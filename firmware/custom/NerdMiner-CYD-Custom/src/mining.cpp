#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>
#include <nvs.h>
//#include "ShaTests/nerdSHA256.h"
#include "ShaTests/nerdSHA256plus.h"
#include "stratum.h"
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "timeconst.h"
#include "drivers/displays/display.h"
#include "drivers/storage/storage.h"
#include <mutex>
#include <atomic>
#include <list>
#include <map>
#include "mbedtls/sha256.h"
#include "i2c_master.h"
#include "speaker.h"
#include "poolWatchdog.h"
#include "miningTarget.h"

//10 Jobs per second
#define NONCE_PER_JOB_SW 4096
// The SW and HW miners must never hash the same nonce of a job (the pool
// refuses the second one as "Duplicate"). The pipelined HW loop searches
// byte-swapped nonces from its own cursor, so native-space chunk ranges alone
// don't keep it apart from the SW chunks: the nonce space is split by the
// low byte of the native nonce instead. SW chunks only use nonces whose low
// byte is below 0x80 (swNonce(): a 4096-nonce chunk spans 8192 natives, all
// allocations stay multiples of 256), the pipelined HW cursor only those
// with 0x80 and above (its top bit is set when it is seeded).
// (Ported from HenrysCat/NerdMiner-Modded, MIT.)
#define SW_NONCE_SPAN (2 * NONCE_PER_JOB_SW)
static inline uint32_t swNonce(uint32_t start, uint32_t n)
{
  return start + ((n >> 7) << 8) + (n & 0x7F);
}
#if defined(PIPELINED_ASM_MINING) && defined(CONFIG_IDF_TARGET_ESP32) && defined(HARDWARE_SHA265)
#define PIPELINED_CLASSIC
#endif

#ifdef PIPELINED_CLASSIC
// Diagnostic fetches pause only between chunks, after every SHA lock has
// been released. The normal mining path pays one atomic load per chunk.
static std::atomic<bool> s_hw_pause_requested{false};
static std::atomic<bool> s_hw_paused{false};
#endif

bool minerHwPauseForChain(bool pause)
{
#ifdef PIPELINED_CLASSIC
  s_hw_pause_requested.store(pause);
  if (!pause) return true;
  uint32_t start = millis();
  while (!s_hw_paused.load()) {
    if (millis() - start >= 2000) {
      s_hw_pause_requested.store(false);
      return false;
    }
    vTaskDelay(1);
  }
  return true;
#else
  return false;
#endif
}

#ifdef PIPELINED_CLASSIC
// One chunk is one call of the pipelined loop between re-entries into C (job
// id check, engine locks, watchdog). At ~1 MH/s, 16K-nonce chunks ran the
// stratum task's 4-deep HW queue dry between its 50 ms refills; 256K is
// ~0.27 s, the size HenrysCat measured best. A job change does not wait for
// the chunk: the stratum task clears s_pl_ctx.run, which the loop reads
// every nonce.
#define NONCE_PER_JOB_HW 256*1024
#else
#define NONCE_PER_JOB_HW 16*1024
#endif

//#define I2C_SLAVE

//#define SHA256_VALIDATE
//#define RANDOM_NONCE
#define RANDOM_NONCE_MASK 0xFFFFC000

#ifdef HARDWARE_SHA265
#include <sha/sha_dma.h>
#include <hal/sha_hal.h>
#include <hal/sha_ll.h>

#if defined(CONFIG_IDF_TARGET_ESP32)
#include <sha/sha_parallel_engine.h>
#endif
#ifdef PIPELINED_CLASSIC
#include <stddef.h>
#include <driver/periph_ctrl.h>
#include <hal/efuse_hal.h>
#endif

#endif

nvs_handle_t stat_handle;
static bool s_stats_ready;

uint32_t templates = 0;
uint32_t hashes = 0;
uint32_t Mhashes = 0;
static std::mutex s_work_mutex;
uint64_t minerTotalHashes(void) {
  std::lock_guard<std::mutex> lock(s_work_mutex);
  return (uint64_t)Mhashes * 1000000ULL + hashes;
}
uint32_t totalKHashes = 0;
uint32_t elapsedKHs = 0;
uint64_t upTime = 0;

volatile uint32_t shares; // increase if blockhash has 32 bits of zeroes
volatile uint32_t valids; // increased if blockhash <= target
volatile uint32_t falseShares; // candidates whose software SHA256d did not confirm them

// The ESP32 hardware-SHA worker can report hashes that are not SHA256d of the
// header. Upstream NerdMiner_v2 #444 measured 99.8 % false shares on this
// board (ESP32-2432S028R, V1.8.3) while TLS handshakes to the pool-stats API
// failed; the pool answers each one with "Difficulty too low". Recompute every
// candidate in software before submit so the pool only sees real shares.
static bool verifyShareSw(const uint8_t* header, uint32_t nonce, uint8_t* hash_out, double* diff_out)
{
  uint8_t hdr[80];
  memcpy(hdr, header, sizeof(hdr));
  memcpy(hdr + 76, &nonce, sizeof(nonce));
  uint32_t mid[8];
  uint32_t bake[16];
  nerd_mids(mid, hdr);
  nerd_sha256_bake(mid, hdr + 64, bake);
  if (!nerd_sha256d_baked(mid, hdr + 64, bake, hash_out))
    return false;   // fewer than 16 leading zero bits: far below any pool difficulty
  *diff_out = diff_from_target(hash_out);
  return true;
}

/* Hashes that pass the hardware worker's 16-bit prefilter (about 5 a second at
 * 345 kH/s) go through this ring to the Monitor task. There each one is
 * recomputed in software: the match count tells whether the SHA peripheral is
 * producing real SHA256d, and the verified hashes feed the display (the newest
 * and the best since boot). One writer (MinerHw-0), one reader (Monitor). */
#define PREFILTER_RING 16
struct PrefilterHit
{
  volatile uint32_t seq;   // publish count + 1 once the slot is complete, 0 while writing
  uint32_t nonce;
  uint8_t header[80];
  uint8_t hash[32];
};
static PrefilterHit s_hits[PREFILTER_RING];
static volatile uint32_t s_hit_seq = 0;
static uint32_t s_hit_read = 0;
// dropped: hits overwritten before the Monitor read them. checked + dropped
// is the true candidate count, the one a hashrate estimate needs.
static uint32_t s_hw_checked = 0, s_hw_mismatched = 0, s_hw_dropped = 0;
static volatile uint32_t s_rejected_submissions;
static volatile uint32_t s_accepted_submissions;
static volatile float s_pool_difficulty;
static volatile bool s_pool_authorized;
uint32_t minerRejectedShares(void) { return s_rejected_submissions; }
uint32_t minerAcceptedShares(void) { return s_accepted_submissions; }
float minerPoolDifficulty(void) { return s_pool_difficulty; }
static uint8_t s_live_hash[32], s_best_hash[32];
static bool s_live_valid = false;
static double s_best_hash_diff = 0.0;

// Hold every SHA engine type for a whole job, not just SHA-256: all types
// share the SHA_TEXT block (IDF sha_parallel_engine.h), so mbedTLS hashing
// SHA-1 or SHA-384 during a TLS handshake overwrites the miner's input and
// the reverse. Off by default until the measurement says otherwise. The
// pipelined worker ignores it and always holds all three (pl_mine_chunk).
#ifndef PIPELINED_CLASSIC
static volatile bool s_sha_lock_all = false;
#endif

static inline void prefilter_publish(const uint8_t* header, uint32_t nonce, const uint8_t* hash)
{
  uint32_t seq = s_hit_seq;
  PrefilterHit& h = s_hits[seq % PREFILTER_RING];
  h.seq = 0;
  __sync_synchronize();
  h.nonce = nonce;
  memcpy(h.header, header, sizeof(h.header));
  memcpy(h.hash, hash, sizeof(h.hash));
  __sync_synchronize();
  h.seq = seq + 1;
  s_hit_seq = seq + 1;
}

static void prefilter_consume(void)
{
  uint32_t end = s_hit_seq;
  if (end - s_hit_read > PREFILTER_RING)
  {
    s_hw_dropped += end - s_hit_read - PREFILTER_RING;
    s_hit_read = end - PREFILTER_RING;
  }
  for (; s_hit_read != end; s_hit_read++)
  {
    PrefilterHit& slot = s_hits[s_hit_read % PREFILTER_RING];
    PrefilterHit e;
    if (slot.seq != s_hit_read + 1) { s_hw_dropped++; continue; }
    memcpy((void*)&e, (const void*)&slot, sizeof(e));
    __sync_synchronize();
    if (slot.seq != s_hit_read + 1) { s_hw_dropped++; continue; }   // overwritten while copying
    uint8_t sw[32];
    double d = 0.0;
    s_hw_checked++;
    if (!verifyShareSw(e.header, e.nonce, sw, &d) || memcmp(sw, e.hash, sizeof(sw)) != 0)
    {
      s_hw_mismatched++;
      continue;
    }
    memcpy(s_live_hash, sw, sizeof(sw));
    s_live_valid = true;
    if (d > s_best_hash_diff)
    {
      s_best_hash_diff = d;
      memcpy(s_best_hash, sw, sizeof(sw));
    }
  }
}

// Hashes are stored least significant byte first; the display wants the
// usual big-endian hex, so reverse on the way out.
static bool copy_display_order(bool valid, const uint8_t* src, uint8_t* out)
{
  if (!valid) return false;
  for (int i = 0; i < 32; i++) out[i] = src[31 - i];
  return true;
}

bool minerLiveHash(uint8_t out[32]) { return copy_display_order(s_live_valid, s_live_hash, out); }
bool minerBestHash(uint8_t out[32]) { return copy_display_order(s_best_hash_diff > 0.0, s_best_hash, out); }
void minerShaStats(uint32_t* checked, uint32_t* mismatched) { *checked = s_hw_checked; *mismatched = s_hw_mismatched; }
#ifdef PIPELINED_CLASSIC
void minerShaLockAll(bool on) { (void)on; }
bool minerShaLockAllOn(void) { return true; }
#else
void minerShaLockAll(bool on) { s_sha_lock_all = on; }
bool minerShaLockAllOn(void) { return s_sha_lock_all; }
#endif

// Track best diff
double best_diff = 0.0;
double minerBestDifficulty(void) { return fmax(best_diff, s_best_hash_diff); }

// Variables to hold data from custom textboxes
//Track mining stats in non volatile memory
extern TSettings Settings;

IPAddress serverIP(1, 1, 1, 1); //Temporally save poolIPaddres

//Global work data 
static WiFiClient client;
static miner_data mMiner; //Global miner data (Create a miner class TODO)
mining_subscribe mWorker;
mining_job mJob;
monitor_data mMonitor;
static bool volatile isMinerSuscribed = false;
unsigned long mLastTXtoPool = millis();

int saveIntervals[7] = {5 * 60, 15 * 60, 30 * 60, 1 * 3600, 3 * 3600, 6 * 3600, 12 * 3600};
int saveIntervalsSize = sizeof(saveIntervals)/sizeof(saveIntervals[0]);
int currentIntervalIndex = 0;

bool minerIsSubscribed(void) { return isMinerSuscribed && s_pool_authorized && WiFi.status() == WL_CONNECTED; }

bool checkPoolConnection(void) {
  
  if (client.connected()) {
    return true;
  }
  
  isMinerSuscribed = false;
  s_pool_authorized = false;

  Serial.println("Client not connected, trying to connect..."); 
  
  //Resolve first time pool DNS and save IP
  if(serverIP == IPAddress(1,1,1,1)) {
    WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP);
    Serial.printf("Resolved DNS and save ip (first time) got: %s\n", serverIP.toString());
  }

  //Try connecting pool IP
  if (!client.connect(serverIP, Settings.PoolPort, 5000)) {
    Serial.println("Imposible to connect to : " + Settings.PoolAddress);
    WiFi.hostByName(Settings.PoolAddress.c_str(), serverIP);
    Serial.printf("Resolved DNS got: %s\n", serverIP.toString());
    return false;
  }

  return true;
}

//Implements a socketKeepAlive function and 
//checks if pool is not sending any data to reconnect again.
//Even connection could be alive, pool could stop sending new job NOTIFY
unsigned long mStart0Hashrate = 0;
static PoolProgressWatchdog s_progress_watchdog;
bool checkPoolInactivity(unsigned int keepAliveTime, unsigned long inactivityTime){ 

    unsigned long currentKHashes = (uint32_t)(minerTotalHashes() / 1000);

    uint32_t time_now = millis();

    // If no shares sent to pool
    // send something to pool to hold socket oppened
    if (time_now - mLastTXtoPool >= keepAliveTime)
    {
      mLastTXtoPool = time_now;
      Serial.println("  Sending  : KeepAlive suggest_difficulty");
      //if (client.print("{}\n") == 0) {
      tx_suggest_difficulty(client, DEFAULT_DIFFICULTY);
      /*if(tx_suggest_difficulty(client, DEFAULT_DIFFICULTY)){
        Serial.println("  Sending keepAlive to pool -> Detected client disconnected");
        return true;
      }*/
    }

    return s_progress_watchdog.sample(time_now, currentKHashes, inactivityTime);
}

struct JobRequest
{
  uint32_t id;
  uint32_t nonce_start;
  uint32_t nonce_count;
  double difficulty;
  uint8_t sha_buffer[128];
  uint32_t midstate[8];
  uint32_t bake[16];
};

struct JobResult
{
  uint32_t id;
  uint32_t nonce;
  uint32_t nonce_count;
  double difficulty;
  uint8_t hash[32];
};

static std::mutex s_job_mutex;
std::list<std::shared_ptr<JobRequest>> s_job_request_list_sw;
#ifdef HARDWARE_SHA265
std::list<std::shared_ptr<JobRequest>> s_job_request_list_hw;
#endif
std::list<std::shared_ptr<JobResult>> s_job_result_list;
static volatile uint8_t s_working_current_job_id = 0xFF;

// Queues a miner's result; s_job_mutex held. When the list is full (the
// stratum task busy for a while) a result without a share is folded into the
// last queued one, so its hashes are still counted. Results with a share are
// kept; the hard cap only guards against a stuck stratum task. (Ported from
// HenrysCat/NerdMiner-Modded, MIT.)
static void pushJobResult(const std::shared_ptr<JobResult> &r, size_t limit)
{
  if (s_job_result_list.size() >= limit && !s_job_result_list.empty() &&
      (r->nonce == 0xFFFFFFFF || s_job_result_list.size() >= 4 * limit))
  {
    s_job_result_list.back()->nonce_count += r->nonce_count;
    return;
  }
  s_job_result_list.push_back(r);
}

#ifdef PIPELINED_CLASSIC
// What the pipelined HW loop works from. The asm knows the layout up to chk
// (src/pipelined_hw_sha_classic_v2.S, CTX_*); the fields after it are C only.
struct pl_ctx_t
{
  uint32_t hdr[20];       // header words as the SHA engine wants them (byte-swapped)
  uint32_t nonce;         // in: first nonce to try (swapped space); out: next one
  uint32_t budget;        // in: nonces to try; out: nonces not tried
  volatile uint32_t run;  // the asm reads it every nonce, so clearing it from the
                          // stratum task stops a stale-job chunk at once
  uint32_t pad;           // 0x80000000
  uint32_t chk;           // bench builds only
  uint32_t has_hash;      // the plain loop sets it: hash holds the engine's digest
  uint8_t hash[32];
};
static_assert(offsetof(pl_ctx_t, nonce) == 80 && offsetof(pl_ctx_t, budget) == 84 &&
              offsetof(pl_ctx_t, run) == 88 && offsetof(pl_ctx_t, pad) == 92 &&
              offsetof(pl_ctx_t, chk) == 96, "pl_ctx_t layout is fixed by the asm");
static pl_ctx_t s_pl_ctx;
#endif

static void JobPush(std::list<std::shared_ptr<JobRequest>> &job_list,  uint32_t id, uint32_t nonce_start, uint32_t nonce_count, double difficulty,
                    const uint8_t* sha_buffer, const uint32_t* midstate, const uint32_t* bake)
{
  std::shared_ptr<JobRequest> job = std::make_shared<JobRequest>();
  job->id = id;
  job->nonce_start = nonce_start;
  job->nonce_count = nonce_count;
  job->difficulty = difficulty;
  memcpy(job->sha_buffer, sha_buffer, sizeof(job->sha_buffer));
  memcpy(job->midstate, midstate, sizeof(job->midstate));
  memcpy(job->bake, bake, sizeof(job->bake));
  job_list.push_back(job);
}

struct Submition
{
  double diff;
  bool is32bit;
  bool isValid;
};

static void MiningJobStop(uint32_t &job_pool, std::map<uint32_t, std::shared_ptr<Submition>> & submition_map)
{
  s_pool_authorized = false;
  {
    std::lock_guard<std::mutex> lock(s_job_mutex);
    s_job_result_list.clear();
    s_job_request_list_sw.clear();
    #ifdef HARDWARE_SHA265
    s_job_request_list_hw.clear();
    #endif
  }
  s_working_current_job_id = 0xFF;
  #ifdef PIPELINED_CLASSIC
  s_pl_ctx.run = 0; // after the id change: see the set-then-check in minerWorkerHw
  #endif
  job_pool = 0xFFFFFFFF;
  submition_map.clear();
}

#ifdef RANDOM_NONCE
uint64_t s_random_state = 1;
static uint32_t RandomGet()
{
    s_random_state += 0x9E3779B97F4A7C15ull;
    uint64_t z = s_random_state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

#endif

void runStratumWorker(void *name) {

// TEST: https://bitcoin.stackexchange.com/questions/22929/full-example-data-for-scrypt-stratum-client

  Serial.println("");
  Serial.printf("\n[WORKER] Started. Running %s on core %d\n", (char *)name, xPortGetCoreID());

  #ifdef DEBUG_MEMORY
  Serial.printf("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
  #endif

  std::map<uint32_t, std::shared_ptr<Submition>> s_submition_map;

#ifdef I2C_SLAVE
  std::vector<uint8_t> i2c_slave_vector;

  //scan for i2c slaves
  if (i2c_master_start() == 0)
    i2c_slave_vector = i2c_master_scan(0x0, 0x80);
  Serial.printf("Found %d slave workers\n", i2c_slave_vector.size());
  if (!i2c_slave_vector.empty())
  {
    Serial.print("  Workers: ");
    for (size_t n = 0; n < i2c_slave_vector.size(); ++n)
      Serial.printf("0x%02X,", (uint32_t)i2c_slave_vector[n]);
    Serial.println("");
  }
#endif

  // connect to pool  
  double currentPoolDifficulty = DEFAULT_DIFFICULTY;
  uint32_t nonce_pool = 0;
  uint32_t job_pool = 0xFFFFFFFF;
  uint32_t last_job_time = millis();
  bool wifiWasConnected = false;   // edge-trigger: alarm only on a connected->lost transition
  String pending_line;

  // Set on mining.notify and read by every later queue refill, so they live
  // outside the loop: declared inside it, the refills read a dead object
  // that only kept its value by stack-slot luck (and -O3 is free to break it).
  uint32_t hw_midstate[8] = {0};
  uint32_t diget_mid[8] = {0};
  uint32_t bake[16] = {0};
  #if defined(CONFIG_IDF_TARGET_ESP32)
  uint8_t sha_buffer_swap[128] = {0};
  #endif

  while(true) {

    if(WiFi.status() != WL_CONNECTED){
      client.stop();
      isMinerSuscribed = false;
      pending_line = "";
      s_progress_watchdog.reset();
      // WiFi is disconnected, so reconnect now
      mMonitor.NerdStatus = NM_Connecting;
      MiningJobStop(job_pool, s_submition_map);
      if (wifiWasConnected) {        // beep the alarm once per real drop, not every retry loop
#ifndef PANICMINER_UI
        Speaker::playDisconnectAlarm();
#endif
        wifiWasConnected = false;
      }
      WiFi.reconnect();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      continue;
    }
    wifiWasConnected = true;         // WiFi is up — arm the alarm for the next loss

    if(!checkPoolConnection()){
      //If server is not reachable add random delay for connection retries
      //Generate value between 1 and 60 secs
      MiningJobStop(job_pool, s_submition_map);
      vTaskDelay(pdMS_TO_TICKS(5000)); // bounded recovery after a transient failure
      continue;
    }

    if(!isMinerSuscribed)
    {
      pending_line = "";
      //Stop miner current jobs
      mWorker = init_mining_subscribe();

      // STEP 1: Pool server connection (SUBSCRIBE)
      if(!tx_mining_subscribe(client, mWorker)) { 
        client.stop();
        MiningJobStop(job_pool, s_submition_map);
        continue; 
      }
      
      snprintf(mWorker.wName, sizeof(mWorker.wName), "%s", Settings.BtcWallet);
      snprintf(mWorker.wPass, sizeof(mWorker.wPass), "%s", Settings.PoolPassword);
      // STEP 2: Pool authorize work (Block Info)
      s_pool_authorized = false; // confirmed by this request's response below
      tx_mining_auth(client, mWorker.wName, mWorker.wPass);
      //tx_mining_auth2(client, mWorker.wName, mWorker.wPass); //Don't verifies authoritzation, TODO

      // STEP 3: Suggest pool difficulty
      tx_suggest_difficulty(client, currentPoolDifficulty);

      isMinerSuscribed=true;
      uint32_t time_now = millis();
      mLastTXtoPool = time_now;
      last_job_time = time_now;
    }

    //Check if pool is down for almost 5minutes and then restart connection with pool (1min=600000ms)
    if(checkPoolInactivity(KEEPALIVE_TIME_ms, POOLINACTIVITY_TIME_ms)){
      //Restart connection
      Serial.println("  Detected more than 2 min without data form stratum server. Closing socket and reopening...");
      client.stop();
      isMinerSuscribed=false;
      MiningJobStop(job_pool, s_submition_map);
      continue; 
    }

    {
      uint32_t time_now = millis();
      if (time_now < last_job_time) //32bit wrap
        last_job_time = time_now;
      if (time_now >= last_job_time + 10*60*1000)  //10minutes without job
      {
        client.stop();
        isMinerSuscribed=false;
        MiningJobStop(job_pool, s_submition_map);
        continue;
      }
    }

    //Read pending messages from pool
    while(client.connected() && client.available())
    {
      int c = client.read();
      if (c < 0) break;
      if (c == '\r') continue;
      if (c != '\n') {
        if (pending_line.length() >= 8192) {
          Serial.println("[POOL] oversized message; reconnecting");
          client.stop();
          isMinerSuscribed = false;
          MiningJobStop(job_pool, s_submition_map);
          pending_line = "";
          break;
        }
        pending_line += (char)c;
        continue;
      }
      String line = std::move(pending_line);
      //Serial.println("  Received message from pool");      
      stratum_method result = parse_mining_method(line);
      switch (result)
      {
          case MINING_NOTIFY:         if(parse_mining_notify(line, mJob))
                                      {
                                          {
                                            std::lock_guard<std::mutex> lock(s_job_mutex);
                                            s_job_request_list_sw.clear();
                                            #ifdef HARDWARE_SHA265
                                            s_job_request_list_hw.clear();
                                            #endif
                                          }
                                          //Increse templates readed
                                          templates++;
                                          job_pool++;
                                          s_working_current_job_id = job_pool & 0xFF; //Terminate current job in thread
                                          #ifdef PIPELINED_CLASSIC
                                          s_pl_ctx.run = 0;                           //...and stop the HW loop mid-chunk now
                                          #endif

                                          last_job_time = millis();
                                          mLastTXtoPool = last_job_time;

                                          {
                                            std::lock_guard<std::mutex> lock(s_work_mutex);
                                            uint32_t mh = hashes/1000000;
                                            Mhashes += mh;
                                            hashes -= mh*1000000;
                                          }

                                          //Prepare data for new jobs
                                          mMiner=calculateMiningData(mWorker, mJob);

                                          memset(mMiner.bytearray_blockheader+80, 0, 128-80);
                                          mMiner.bytearray_blockheader[80] = 0x80;
                                          mMiner.bytearray_blockheader[126] = 0x02;
                                          mMiner.bytearray_blockheader[127] = 0x80;

                                          nerd_mids(diget_mid, mMiner.bytearray_blockheader);
                                          nerd_sha256_bake(diget_mid, mMiner.bytearray_blockheader+64, bake);

                                          #ifdef HARDWARE_SHA265
                                          #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
                                            esp_sha_acquire_hardware();
                                            sha_hal_hash_block(SHA2_256,  mMiner.bytearray_blockheader, 64/4, true);
                                            sha_hal_read_digest(SHA2_256, hw_midstate);
                                            esp_sha_release_hardware();
                                          #endif
                                          #endif

                                          #if defined(CONFIG_IDF_TARGET_ESP32)
                                          for (int i = 0; i < 32; ++i)
                                            ((uint32_t*)sha_buffer_swap)[i] = __builtin_bswap32(((const uint32_t*)(mMiner.bytearray_blockheader))[i]);
                                          #endif

                                          #ifdef RANDOM_NONCE
                                          nonce_pool = RandomGet() & RANDOM_NONCE_MASK;
                                          #else
                                            #ifdef I2C_SLAVE
                                            if (!i2c_slave_vector.empty())
                                              nonce_pool = 0x10000000;
                                            else
                                            #endif
                                              nonce_pool = 0xDA54E700;  //nonce 0x00000000 is not possible, start from some random nonce
                                          #endif
                                          

                                          {
                                            std::lock_guard<std::mutex> lock(s_job_mutex);
                                            for (int i = 0; i < 4; ++ i)
                                            {
                                              #if 1
                                              JobPush( s_job_request_list_sw, job_pool, nonce_pool, NONCE_PER_JOB_SW, currentPoolDifficulty, mMiner.bytearray_blockheader, diget_mid, bake);
                                              #ifdef RANDOM_NONCE
                                              nonce_pool = RandomGet() & RANDOM_NONCE_MASK;
                                              #else
                                              nonce_pool += SW_NONCE_SPAN;
                                              #endif
                                              #endif
                                              #ifdef HARDWARE_SHA265
                                                #if defined(CONFIG_IDF_TARGET_ESP32)
                                                  JobPush( s_job_request_list_hw, job_pool, nonce_pool, NONCE_PER_JOB_HW, currentPoolDifficulty, sha_buffer_swap, hw_midstate, bake);
                                                #else
                                                  JobPush( s_job_request_list_hw, job_pool, nonce_pool, NONCE_PER_JOB_HW, currentPoolDifficulty, mMiner.bytearray_blockheader, hw_midstate, bake);
                                                #endif
                                              #ifdef RANDOM_NONCE
                                              nonce_pool = RandomGet() & RANDOM_NONCE_MASK;
                                              #else
                                              nonce_pool += NONCE_PER_JOB_HW;
                                              #endif
                                              #endif
                                            }
                                          }
                                          #ifdef I2C_SLAVE
                                          //Nonce for nonce_pool starts from 0x10000000
                                          //For i2c slave we give nonces from 0x20000000, that is 0x10000000 nonces per slave
                                          i2c_feed_slaves(i2c_slave_vector, job_pool & 0xFF, 0x20, currentPoolDifficulty, mMiner.bytearray_blockheader);
                                          #endif
                                      } else
                                      {
                                        Serial.println("Parsing error, need restart");
                                        client.stop();
                                        isMinerSuscribed=false;
                                        MiningJobStop(job_pool, s_submition_map);
                                      }
                                      break;
          case MINING_SET_DIFFICULTY: if (parse_mining_set_difficulty(line, currentPoolDifficulty))
                                        s_pool_difficulty = currentPoolDifficulty;
                                      break;
          case STRATUM_SUCCESS:       {
                                        unsigned long id = parse_extract_id(line);
                                        if (id == stratumAuthRequestId()) {
                                          s_pool_authorized = true;
                                          Serial.println("[POOL] wallet worker authorized by pool");
                                        }
                                        auto itt = s_submition_map.find(id);
                                        if (itt != s_submition_map.end())
                                        {
                                          s_accepted_submissions++;
                                          Speaker::playShareBeep();
                                          Serial.printf("[POOL] share accepted, diff %.6g; accepted %lu\n",
                                              itt->second->diff, (unsigned long)s_accepted_submissions);
                                          if (itt->second->diff > best_diff)
                                            best_diff = itt->second->diff;
                                          if (itt->second->is32bit) {
                                            shares++;
                                          }
                                          if (itt->second->isValid)
                                          {
                                            Serial.println("CONGRATULATIONS! Valid block found");
                                            valids++;
                                            Speaker::playBlockChime();
                                          }
                                          s_submition_map.erase(itt);
                                        }
                                      }
                                      break;
          case STRATUM_PARSE_ERROR:   {
                                        unsigned long id = parse_extract_id(line);
                                        if (id == stratumAuthRequestId()) {
                                          Serial.println("[POOL] authorization rejected; check wallet configuration");
                                          client.stop();
                                          isMinerSuscribed = false;
                                          MiningJobStop(job_pool, s_submition_map);
                                        }
                                        auto itt = s_submition_map.find(id);
                                        if (itt != s_submition_map.end())
                                        {
                                          Serial.printf("Refuse submition %d\n", id);
                                          s_rejected_submissions++;
                                          s_submition_map.erase(itt);
                                        }
                                      }
                                      break;
          default:                    Serial.println("  Parsed JSON: unknown"); break;

      }
    }

    std::list<std::shared_ptr<JobResult>> job_result_list;
    #ifdef I2C_SLAVE
    if (i2c_slave_vector.empty() || job_pool == 0xFFFFFFFF)
    {
      vTaskDelay(50 / portTICK_PERIOD_MS); //Small delay
    } else
    {
      uint32_t time_start = millis();
      i2c_hit_slaves(i2c_slave_vector);
      vTaskDelay(5 / portTICK_PERIOD_MS);
      uint32_t nonces_done = 0;
      std::vector<uint32_t> nonce_vector = i2c_harvest_slaves(i2c_slave_vector, job_pool & 0xFF, nonces_done);
      {
        std::lock_guard<std::mutex> lock(s_work_mutex);
        hashes += nonces_done;
      }
      for (size_t n = 0; n < nonce_vector.size(); ++n)
      {
        std::shared_ptr<JobResult> result = std::make_shared<JobResult>();
        ((uint32_t*)(mMiner.bytearray_blockheader+64+12))[0] = nonce_vector[n];
        if (nerd_sha256d_baked(diget_mid, mMiner.bytearray_blockheader+64, bake, result->hash))
        {
          result->id = job_pool;
          result->nonce = nonce_vector[n];
          result->nonce_count = 0;
          result->difficulty = diff_from_target(result->hash);
          job_result_list.push_back(result);
        }
      }
      uint32_t time_end = millis();
      //if (nonces_done > 16384)
        //Serial.printf("Harvest slaves in %dms hashes=%d\n", time_end - time_start, nonces_done);
      if (time_end > time_start)
      {
        uint32_t elapsed = time_end - time_start;
        if (elapsed < 50)
          vTaskDelay((50 - elapsed) / portTICK_PERIOD_MS);
      } else
        vTaskDelay(40 / portTICK_PERIOD_MS);
    }
    #else
    vTaskDelay(50 / portTICK_PERIOD_MS); //Small delay
    #endif

    
    if (job_pool != 0xFFFFFFFF)
    {
      std::lock_guard<std::mutex> lock(s_job_mutex);
      job_result_list.insert(job_result_list.end(), s_job_result_list.begin(), s_job_result_list.end());
      s_job_result_list.clear();

#if 1
      while (s_job_request_list_sw.size() < 4)
      {
        JobPush( s_job_request_list_sw, job_pool, nonce_pool, NONCE_PER_JOB_SW, currentPoolDifficulty, mMiner.bytearray_blockheader, diget_mid, bake);
        #ifdef RANDOM_NONCE
        nonce_pool = RandomGet() & RANDOM_NONCE_MASK;
        #else
        nonce_pool += SW_NONCE_SPAN;
        #endif
      }
#endif

      #ifdef HARDWARE_SHA265
      while (s_job_request_list_hw.size() < 4)
      {
        #if defined(CONFIG_IDF_TARGET_ESP32)
          JobPush( s_job_request_list_hw, job_pool, nonce_pool, NONCE_PER_JOB_HW, currentPoolDifficulty, sha_buffer_swap, hw_midstate, bake);
        #else
          JobPush( s_job_request_list_hw, job_pool, nonce_pool, NONCE_PER_JOB_HW, currentPoolDifficulty, mMiner.bytearray_blockheader, hw_midstate, bake);
        #endif
        #ifdef RANDOM_NONCE
        nonce_pool = RandomGet() & RANDOM_NONCE_MASK;
        #else
        nonce_pool += NONCE_PER_JOB_HW;
        #endif
      }
      #endif
    }

    while (!job_result_list.empty())
    {
      std::shared_ptr<JobResult> res = job_result_list.front();
      job_result_list.pop_front();

      {
        std::lock_guard<std::mutex> lock(s_work_mutex);
        hashes += res->nonce_count;
      }
      if (res->difficulty > currentPoolDifficulty && job_pool == res->id && res->nonce != 0xFFFFFFFF)
      {
        if (!client.connected())
          break;
        double sw_diff = 0.0;
        if (!verifyShareSw(mMiner.bytearray_blockheader, res->nonce, res->hash, &sw_diff)
            || sw_diff <= currentPoolDifficulty)
        {
          falseShares++;
          Serial.printf("[VERIFY] dropped false share: reported %.6f, software %.3g (%lu dropped)\n",
                        res->difficulty, sw_diff, (unsigned long)falseShares);
          continue;
        }
        res->difficulty = sw_diff;
        unsigned long sumbit_id = 0;
        tx_mining_submit(client, mWorker, mJob, res->nonce, sumbit_id);
        Serial.print("   - Current diff share: "); Serial.println(res->difficulty,12);
        Serial.print("   - Current pool diff : "); Serial.println(currentPoolDifficulty,12);
        Serial.print("   - TX SHARE: ");
        for (size_t i = 0; i < 32; i++)
            Serial.printf("%02x", res->hash[i]);
        Serial.println("");
        mLastTXtoPool = millis();

        std::shared_ptr<Submition> submition = std::make_shared<Submition>();
        submition->diff = res->difficulty;
        submition->is32bit = (res->hash[29] == 0 && res->hash[28] == 0);
        if (submition->is32bit)
        {
          submition->isValid = checkValid(res->hash, mMiner.bytearray_target);
        } else
          submition->isValid = false;

        s_submition_map.insert(std::make_pair(sumbit_id, submition));
        if (s_submition_map.size() > 32)
          s_submition_map.erase(s_submition_map.begin());
      }
    }
  }
}

//////////////////THREAD CALLS///////////////////

void minerWorkerSw(void * task_id)
{
  unsigned int miner_id = (uint32_t)task_id;
  Serial.printf("[MINER] %d Started minerWorkerSw Task!\n", miner_id);

  std::shared_ptr<JobRequest> job;
  std::shared_ptr<JobResult> result;
  uint8_t hash[32];
  uint32_t wdt_counter = 0;
  while (1)
  {
    {
      std::lock_guard<std::mutex> lock(s_job_mutex);
      if (result)
      {
        if (s_job_result_list.size() < 16)
          s_job_result_list.push_back(result);
        result.reset();
      }
      if (!s_job_request_list_sw.empty())
      {
        job = s_job_request_list_sw.front();
        s_job_request_list_sw.pop_front();
      } else
        job.reset();
    }
    if (job)
    {
      result = std::make_shared<JobResult>();
      result->difficulty = job->difficulty;
      result->nonce = 0xFFFFFFFF;
      result->id = job->id;
      result->nonce_count = job->nonce_count;
      uint8_t job_in_work = job->id & 0xFF;
      for (uint32_t n = 0; n < job->nonce_count; ++n)
      {
        ((uint32_t*)(job->sha_buffer+64+12))[0] = swNonce(job->nonce_start, n);
        if (nerd_sha256d_baked(job->midstate, job->sha_buffer+64, job->bake, hash))
        {
          double diff_hash = diff_from_target(hash);
          if (diff_hash > result->difficulty)
          {
            result->difficulty = diff_hash;
            result->nonce = swNonce(job->nonce_start, n);
            memcpy(result->hash, hash, 32);
          }
        }

        if ( (uint16_t)(n & 0xFF) == 0 &&s_working_current_job_id != job_in_work)
        {
          result->nonce_count = n+1;
          break;
        }
      }
    } else
      vTaskDelay(2 / portTICK_PERIOD_MS);

    wdt_counter++;
    if (wdt_counter >= 8)
    {
      wdt_counter = 0;
      esp_task_wdt_reset();
    }
  }
}

#ifdef HARDWARE_SHA265

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)

static inline void nerd_sha_ll_fill_text_block_sha256(const void *input_text, uint32_t nonce)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    REG_WRITE(&reg_addr_buf[0], data_words[0]);
    REG_WRITE(&reg_addr_buf[1], data_words[1]);
    REG_WRITE(&reg_addr_buf[2], data_words[2]);
#if 0
    REG_WRITE(&reg_addr_buf[3], nonce);
    //REG_WRITE(&reg_addr_buf[3], data_words[3]);    
    REG_WRITE(&reg_addr_buf[4], data_words[4]);
    REG_WRITE(&reg_addr_buf[5], data_words[5]);
    REG_WRITE(&reg_addr_buf[6], data_words[6]);
    REG_WRITE(&reg_addr_buf[7], data_words[7]);
    REG_WRITE(&reg_addr_buf[8], data_words[8]);
    REG_WRITE(&reg_addr_buf[9], data_words[9]);
    REG_WRITE(&reg_addr_buf[10], data_words[10]);
    REG_WRITE(&reg_addr_buf[11], data_words[11]);
    REG_WRITE(&reg_addr_buf[12], data_words[12]);
    REG_WRITE(&reg_addr_buf[13], data_words[13]);
    REG_WRITE(&reg_addr_buf[14], data_words[14]);
    REG_WRITE(&reg_addr_buf[15], data_words[15]);
#else
    REG_WRITE(&reg_addr_buf[3], nonce);
    REG_WRITE(&reg_addr_buf[4], 0x00000080);
    REG_WRITE(&reg_addr_buf[5], 0x00000000);
    REG_WRITE(&reg_addr_buf[6], 0x00000000);
    REG_WRITE(&reg_addr_buf[7], 0x00000000);
    REG_WRITE(&reg_addr_buf[8], 0x00000000);
    REG_WRITE(&reg_addr_buf[9], 0x00000000);
    REG_WRITE(&reg_addr_buf[10], 0x00000000);
    REG_WRITE(&reg_addr_buf[11], 0x00000000);
    REG_WRITE(&reg_addr_buf[12], 0x00000000);
    REG_WRITE(&reg_addr_buf[13], 0x00000000);
    REG_WRITE(&reg_addr_buf[14], 0x00000000);
    REG_WRITE(&reg_addr_buf[15], 0x80020000);
#endif
}

static inline void nerd_sha_ll_fill_text_block_sha256_inter()
{
  uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

  DPORT_INTERRUPT_DISABLE();
  REG_WRITE(&reg_addr_buf[0], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4));
  REG_WRITE(&reg_addr_buf[1], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4));
  REG_WRITE(&reg_addr_buf[2], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4));
  REG_WRITE(&reg_addr_buf[3], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4));
  REG_WRITE(&reg_addr_buf[4], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4));
  REG_WRITE(&reg_addr_buf[5], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4));
  REG_WRITE(&reg_addr_buf[6], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4));
  REG_WRITE(&reg_addr_buf[7], DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4));
  DPORT_INTERRUPT_RESTORE();

  REG_WRITE(&reg_addr_buf[8], 0x00000080);
  REG_WRITE(&reg_addr_buf[9], 0x00000000);
  REG_WRITE(&reg_addr_buf[10], 0x00000000);
  REG_WRITE(&reg_addr_buf[11], 0x00000000);
  REG_WRITE(&reg_addr_buf[12], 0x00000000);
  REG_WRITE(&reg_addr_buf[13], 0x00000000);
  REG_WRITE(&reg_addr_buf[14], 0x00000000);
  REG_WRITE(&reg_addr_buf[15], 0x00010000);
}

static inline void nerd_sha_ll_read_digest(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4);  
  ((uint32_t*)ptr)[7] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4);
  DPORT_INTERRUPT_RESTORE();
}


static inline bool nerd_sha_ll_read_digest_if(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  uint32_t last = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 7 * 4);
  #if 1
  if ( (uint16_t)(last >> 16) != 0)
  {
    DPORT_INTERRUPT_RESTORE();
    return false;
  }
  #endif

  ((uint32_t*)ptr)[7] = last;
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_H_BASE + 6 * 4);  
  DPORT_INTERRUPT_RESTORE();
  return true;
}

static inline void nerd_sha_ll_write_digest(void *digest_state)
{
    uint32_t *digest_state_words = (uint32_t *)digest_state;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_H_BASE);

    REG_WRITE(&reg_addr_buf[0], digest_state_words[0]);
    REG_WRITE(&reg_addr_buf[1], digest_state_words[1]);
    REG_WRITE(&reg_addr_buf[2], digest_state_words[2]);
    REG_WRITE(&reg_addr_buf[3], digest_state_words[3]);
    REG_WRITE(&reg_addr_buf[4], digest_state_words[4]);
    REG_WRITE(&reg_addr_buf[5], digest_state_words[5]);
    REG_WRITE(&reg_addr_buf[6], digest_state_words[6]);
    REG_WRITE(&reg_addr_buf[7], digest_state_words[7]);
}

static inline void nerd_sha_hal_wait_idle()
{
    while (REG_READ(SHA_BUSY_REG))
    {}
}

//#define VALIDATION
void minerWorkerHw(void * task_id)
{
  unsigned int miner_id = (uint32_t)task_id;
  Serial.printf("[MINER] %d Started minerWorkerHw Task!\n", miner_id);

  std::shared_ptr<JobRequest> job;
  std::shared_ptr<JobResult> result;
  uint8_t interResult[64];
  uint8_t hash[32];
  uint8_t digest_mid[32];
  uint8_t sha_buffer[64];
  uint32_t wdt_counter = 0;

#ifdef VALIDATION
  uint8_t doubleHash[32];
  uint32_t diget_mid[8];
  uint32_t bake[16];
#endif

  while (1)
  {
    {
      std::lock_guard<std::mutex> lock(s_job_mutex);
      if (result)
      {
        if (s_job_result_list.size() < 16)
          s_job_result_list.push_back(result);
        result.reset();
      }
      if (!s_job_request_list_hw.empty())
      {
        job = s_job_request_list_hw.front();
        s_job_request_list_hw.pop_front();
      } else
        job.reset();
    }
    if (job)
    {
      result = std::make_shared<JobResult>();
      result->id = job->id;
      result->nonce = 0xFFFFFFFF;
      result->nonce_count = job->nonce_count;
      result->difficulty = job->difficulty;
      uint8_t job_in_work = job->id & 0xFF;
      memcpy(digest_mid, job->midstate, sizeof(digest_mid));
      memcpy(sha_buffer, job->sha_buffer+64, sizeof(sha_buffer));
#ifdef VALIDATION
      nerd_mids(diget_mid, job->sha_buffer);
      nerd_sha256_bake(diget_mid, job->sha_buffer+64, bake);
#endif

      esp_sha_acquire_hardware();
      REG_WRITE(SHA_MODE_REG, SHA2_256);
      uint32_t nend = job->nonce_start + job->nonce_count;
      for (uint32_t n = job->nonce_start; n < nend; ++n)
      {
        //nerd_sha_hal_wait_idle();
        nerd_sha_ll_write_digest(digest_mid);
        //nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256(sha_buffer, n);
        //sha_ll_continue_block(SHA2_256);
        REG_WRITE(SHA_CONTINUE_REG, 1);
        
        sha_ll_load(SHA2_256);
        nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256_inter();
        //sha_ll_start_block(SHA2_256);
        REG_WRITE(SHA_START_REG, 1);
        sha_ll_load(SHA2_256);
        nerd_sha_hal_wait_idle();
        if (nerd_sha_ll_read_digest_if(hash))
        {
          //Serial.printf("Hw 16bit Share, nonce=0x%X\n", n);
#ifdef VALIDATION
          //Validation
          ((uint32_t*)(job->sha_buffer+64+12))[0] = n;
          nerd_sha256d_baked(diget_mid, job->sha_buffer+64, bake, doubleHash);
          for (int i = 0; i < 32; ++i)
          {
            if (hash[i] != doubleHash[i])
            {
              Serial.println("***HW sha256 esp32s3 bug detected***");
              break;
            }
          }
#endif
          //~5 per second
          double diff_hash = diff_from_target(hash);
          if (diff_hash > result->difficulty)
          {
            if (isSha256Valid(hash))
            {
              result->difficulty = diff_hash;
              result->nonce = n;
              memcpy(result->hash, hash, sizeof(hash));
            }
          }
        }
        if (
             (uint8_t)(n & 0xFF) == 0 &&
             s_working_current_job_id != job_in_work)
        {
          result->nonce_count = n-job->nonce_start+1;
          break;
        }
      }
      esp_sha_release_hardware();
    } else
      vTaskDelay(2 / portTICK_PERIOD_MS);

    wdt_counter++;
    if (wdt_counter >= 8)
    {
      wdt_counter = 0;
      esp_task_wdt_reset();
    }
  }
}

#endif  //#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)

#if defined(CONFIG_IDF_TARGET_ESP32)

static inline bool nerd_sha_ll_read_digest_swap_if(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  uint32_t fin = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4);
  if ( (uint32_t)(fin & 0xFFFF) != 0)
  {
    DPORT_INTERRUPT_RESTORE();
    return false;
  }
  ((uint32_t*)ptr)[7] = __builtin_bswap32(fin);
  ((uint32_t*)ptr)[0] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4));
  ((uint32_t*)ptr)[1] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4));
  ((uint32_t*)ptr)[2] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4));
  ((uint32_t*)ptr)[3] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4));
  ((uint32_t*)ptr)[4] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4));
  ((uint32_t*)ptr)[5] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4));
  ((uint32_t*)ptr)[6] = __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4));
  DPORT_INTERRUPT_RESTORE();
  return true;
}

static inline void nerd_sha_ll_read_digest(void* ptr)
{
  DPORT_INTERRUPT_DISABLE();
  ((uint32_t*)ptr)[0] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4);
  ((uint32_t*)ptr)[1] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4);
  ((uint32_t*)ptr)[2] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4);
  ((uint32_t*)ptr)[3] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4);
  ((uint32_t*)ptr)[4] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4);
  ((uint32_t*)ptr)[5] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4);
  ((uint32_t*)ptr)[6] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4);
  ((uint32_t*)ptr)[7] = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4);
  DPORT_INTERRUPT_RESTORE();
}

static inline void nerd_sha_hal_wait_idle()
{
    while (DPORT_REG_READ(SHA_256_BUSY_REG))
    {}
}

static inline void nerd_sha_ll_fill_text_block_sha256(const void *input_text)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = data_words[0];
    reg_addr_buf[1]  = data_words[1];
    reg_addr_buf[2]  = data_words[2];
    reg_addr_buf[3]  = data_words[3];
    reg_addr_buf[4]  = data_words[4];
    reg_addr_buf[5]  = data_words[5];
    reg_addr_buf[6]  = data_words[6];
    reg_addr_buf[7]  = data_words[7];
    reg_addr_buf[8]  = data_words[8];
    reg_addr_buf[9]  = data_words[9];
    reg_addr_buf[10] = data_words[10];
    reg_addr_buf[11] = data_words[11];
    reg_addr_buf[12] = data_words[12];
    reg_addr_buf[13] = data_words[13];
    reg_addr_buf[14] = data_words[14];
    reg_addr_buf[15] = data_words[15];
}

static inline void nerd_sha_ll_fill_text_block_sha256_upper(const void *input_text, uint32_t nonce)
{
    uint32_t *data_words = (uint32_t *)input_text;
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

    reg_addr_buf[0]  = data_words[0];
    reg_addr_buf[1]  = data_words[1];
    reg_addr_buf[2]  = data_words[2];
    reg_addr_buf[3]  = __builtin_bswap32(nonce);
#if 1
    reg_addr_buf[4]  = 0x80000000;
    reg_addr_buf[5]  = 0x00000000;
    reg_addr_buf[6]  = 0x00000000;
    reg_addr_buf[7]  = 0x00000000;
    reg_addr_buf[8]  = 0x00000000;
    reg_addr_buf[9]  = 0x00000000;
    reg_addr_buf[10] = 0x00000000;
    reg_addr_buf[11] = 0x00000000;
    reg_addr_buf[12] = 0x00000000;
    reg_addr_buf[13] = 0x00000000;
    reg_addr_buf[14] = 0x00000000;
    reg_addr_buf[15] = 0x00000280;
#else
    reg_addr_buf[4]  = data_words[4];
    reg_addr_buf[5]  = data_words[5];
    reg_addr_buf[6]  = data_words[6];
    reg_addr_buf[7]  = data_words[7];
    reg_addr_buf[8]  = data_words[8];
    reg_addr_buf[9]  = data_words[9];
    reg_addr_buf[10] = data_words[10];
    reg_addr_buf[11] = data_words[11];
    reg_addr_buf[12] = data_words[12];
    reg_addr_buf[13] = data_words[13];
    reg_addr_buf[14] = data_words[14];
    reg_addr_buf[15] = data_words[15];
#endif
}

static inline void nerd_sha_ll_fill_text_block_sha256_double()
{
    uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

#if 0
    //No change
    reg_addr_buf[0]  = data_words[0];
    reg_addr_buf[1]  = data_words[1];
    reg_addr_buf[2]  = data_words[2];
    reg_addr_buf[3]  = data_words[3];
    reg_addr_buf[4]  = data_words[4];
    reg_addr_buf[5]  = data_words[5];
    reg_addr_buf[6]  = data_words[6];
    reg_addr_buf[7]  = data_words[7];
#endif
    reg_addr_buf[8]  = 0x80000000;
    reg_addr_buf[9]  = 0x00000000;
    reg_addr_buf[10] = 0x00000000;
    reg_addr_buf[11] = 0x00000000;
    reg_addr_buf[12] = 0x00000000;
    reg_addr_buf[13] = 0x00000000;
    reg_addr_buf[14] = 0x00000000;
    reg_addr_buf[15] = 0x00000100;
}

// Engine lock order is fixed (SHA1, SHA2_256, SHA2_512) so two lockers cannot
// deadlock. SHA2_384 shares SHA2_512's engine and lock.
static bool sha_lock_job(bool all)
{
  if (!all)
    return esp_sha_try_lock_engine(SHA2_256);
  if (!esp_sha_try_lock_engine(SHA1))
    return false;
  if (!esp_sha_try_lock_engine(SHA2_256))
  {
    esp_sha_unlock_engine(SHA1);
    return false;
  }
  if (!esp_sha_try_lock_engine(SHA2_512))
  {
    esp_sha_unlock_engine(SHA2_256);
    esp_sha_unlock_engine(SHA1);
    return false;
  }
  return true;
}

static void sha_unlock_job(bool all)
{
  if (all) esp_sha_unlock_engine(SHA2_512);
  esp_sha_unlock_engine(SHA2_256);
  if (all) esp_sha_unlock_engine(SHA1);
}

// try-locks, so a TLS handshake holding an engine delays the job instead of
// starving the task watchdog
static void sha_lock_job_wait(bool all)
{
  while (!sha_lock_job(all))
  {
    esp_task_wdt_reset();
    vTaskDelay(1);
  }
}

#ifdef PIPELINED_CLASSIC
// Pipelined HW loops (ported from HenrysCat/NerdMiner-Modded, MIT): they
// overlap the CPU's register traffic with the SHA engine's busy time instead
// of idle-spinning through it. All share one calling convention -- 1 =
// candidate at ctx->nonce - 1, 0 = budget spent or ctx->run cleared -- so the
// worker can swap one for another.
//
// They write SHA_TEXT with no lock of their own, so the worker holds all
// three engine locks (SHA1, SHA2_256, SHA2_512) for every call, the self-test
// included: mbedTLS then finds every engine busy and hashes in software
// instead of writing SHA_TEXT under the miner. IDF itself guards SHA_TEXT
// with a spinlock held for one block (esp_sha_block()); a 0.27 s chunk can't
// sit in that critical section, so the engine locks are what keep TLS out.
typedef uint32_t (*pl_mine_fn)(volatile uint32_t *sha_base, pl_ctx_t *ctx);

// src/pipelined_hw_sha_classic_v2.S: counted delays, no status polling.
extern "C" uint32_t pipelined_hw_mine_classic_v2(volatile uint32_t *sha_base, pl_ctx_t *ctx);
extern "C" uint32_t pipelined_hw_mine_classic_v2_margin(volatile uint32_t *sha_base, pl_ctx_t *ctx);
extern "C" uint32_t pipelined_hw_mine_classic_v2_wide(volatile uint32_t *sha_base, pl_ctx_t *ctx);

// The non-pipelined loop (same sequence as the #else branch in minerWorkerHw):
// less than half the speed, but every read of a SHA (DPORT) register goes
// through the SDK's DPORT workaround. The loops above read them raw, which
// only chip revision 3 and later tolerate -- see pl_select_loop. It keeps
// the engine's digest of a candidate, so the Monitor can check it.
static uint32_t pl_mine_plain(volatile uint32_t *sha_base, pl_ctx_t *ctx)
{
  while (ctx->run && ctx->budget > 0)
  {
    nerd_sha_ll_fill_text_block_sha256(ctx->hdr);
    sha_ll_start_block(SHA2_256);

    nerd_sha_hal_wait_idle();
    // ctx->nonce is already in the engine's byte order; the fill swaps its argument.
    nerd_sha_ll_fill_text_block_sha256_upper(ctx->hdr + 16, __builtin_bswap32(ctx->nonce));
    sha_ll_continue_block(SHA2_256);

    nerd_sha_hal_wait_idle();
    sha_ll_load(SHA2_256);

    nerd_sha_hal_wait_idle();
    nerd_sha_ll_fill_text_block_sha256_double();
    sha_ll_start_block(SHA2_256);

    nerd_sha_hal_wait_idle();
    sha_ll_load(SHA2_256);

    ctx->nonce++;
    ctx->budget--;
    if (nerd_sha_ll_read_digest_swap_if(ctx->hash))
    {
      ctx->has_hash = 1;
      return 1;
    }
  }
  return 0;
}

// Fastest first. HenrysCat's BUSY-polling loop is left out on purpose: its
// own notes measure about one wrong hash in a few thousand with core 0 busy.
static const struct { pl_mine_fn fn; const char *name; } s_pl_loops[] = {
  {pipelined_hw_mine_classic_v2,        "fast"},
  {pipelined_hw_mine_classic_v2_margin, "fast, +2 margins"},
  {pipelined_hw_mine_classic_v2_wide,   "fast, wide margins"},
  {pl_mine_plain,                       "plain"},
};
#define PL_LOOP_COUNT (sizeof(s_pl_loops) / sizeof(s_pl_loops[0]))
#define PL_LOOP_PLAIN (PL_LOOP_COUNT - 1)
static volatile unsigned s_pl_loop = PL_LOOP_PLAIN;

// Known-answer test: for the header hdr[i] = 0x9E3779B9 * (i + 1), these are
// the only candidates among the 262144 nonces from 0x1000 up. A loop passes
// if it reports exactly these, in order -- one wrong hash among the 262144
// that happens to be a candidate, or hides one, fails it.
static bool pl_selftest(pl_mine_fn fn, size_t *found_out)
{
  static const uint32_t expected[] = {0xa94c, 0xe496, 0x1f476, 0x302f0, 0x35af9, 0x3cf5c};
  const uint32_t first = 0x1000;
  static pl_ctx_t c; // not s_pl_ctx: the stratum task may clear that one's run flag at any time
  for (uint32_t i = 0; i < 19; ++i)
    c.hdr[i] = 0x9E3779B9u * (i + 1);
  c.nonce = first;
  c.budget = 262144;
  c.pad = 0x80000000u;
  c.run = 1;
  size_t found = 0;
  bool ok = true;
  while (c.budget > 0 && fn((volatile uint32_t *)SHA_TEXT_BASE, &c))
  {
    if (found >= sizeof(expected) / sizeof(expected[0]) || c.nonce - 1 - first != expected[found])
    {
      ok = false;
      break;
    }
    found++;
  }
  *found_out = found;
  return ok && found == sizeof(expected) / sizeof(expected[0]) && c.budget == 0;
}

// Picks the fastest loop this chip runs correctly. The fast loops rely on
// the engine's timing as measured on ESP32-D0WD-V3; anything that behaves
// differently fails the known-answer test and drops to the next one. One
// failed run is enough to drop a loop.
//
// Chips before revision 3 are not offered the pipelined loops at all. Seen
// upstream on an ESP32-D0WDQ6 rev 1.1: the fast loop passes the self-test and
// hashes correctly, but within seconds core 0 stops. That matches the silicon
// bug fixed in revision 3, where a raw DPORT read on one core corrupts APB
// reads on the other, so the self-test cannot see it.
static void pl_select_loop()
{
  sha_lock_job_wait(true);
  periph_module_reset(PERIPH_SHA_MODULE);
  unsigned n = 0;
  const unsigned chip_rev = efuse_hal_get_major_chip_version();
  if (chip_rev < 3)
  {
    Serial.printf("[MINER] chip revision %u: the pipelined HW SHA loops need revision 3; self-test skipped\n", chip_rev);
    n = PL_LOOP_PLAIN;
  }
  for (; n < PL_LOOP_PLAIN; ++n)
  {
    size_t found = 0;
    uint32_t t0 = millis();
    const bool pass = pl_selftest(s_pl_loops[n].fn, &found);
    Serial.printf("[MINER] HW SHA self-test '%s': %s (%u of 6 known candidates in 262144 nonces, %lu ms)\n",
                  s_pl_loops[n].name, pass ? "PASS" : "FAIL", (unsigned)found, (unsigned long)(millis() - t0));
    if (pass)
      break;
  }
  s_pl_loop = n;
  sha_unlock_job(true);
  Serial.printf("[MINER] HW SHA loop: %s (%s)\n", s_pl_loops[n].name,
                n < PL_LOOP_PLAIN ? "pipelined" : "not pipelined");
}

// Every candidate is recomputed in software before it is used, which makes
// the share of candidates software does NOT confirm a running estimate of how
// many hashes the loop gets wrong. It is 0 when healthy; if it ever isn't,
// stop trusting the loop and use the next, more conservative one.
static void pl_health(bool confirmed)
{
  static uint32_t hits = 0, unconfirmed = 0;
  hits++;
  if (!confirmed)
    unconfirmed++;
  if (unconfirmed >= 2 && s_pl_loop + 1 < PL_LOOP_COUNT)
  {
    s_pl_loop = s_pl_loop + 1;
    Serial.printf("[MINER] %u of %u HW candidates not confirmed by software; HW SHA loop now: %s\n",
                  (unsigned)unconfirmed, (unsigned)hits, s_pl_loops[s_pl_loop].name);
    hits = unconfirmed = 0;
  }
  if (hits >= 256)
    hits = unconfirmed = 0;
}

// pl_health only sees the candidates a loop reports, so a loop that stopped
// reporting them would look healthy while every share went missing. A healthy
// loop yields one confirmed candidate per 65536 nonces on average; 2^23
// nonces (about 9 s at 1 MH/s, 128 expected) without one has probability
// e^-128, so it means a broken loop, not bad luck. Counting whole chunks
// makes this fire up to one chunk (2^18 nonces) early, which changes nothing.
#define PL_DRY_LIMIT (1u << 23)
static uint32_t s_pl_dry = 0; // nonces hashed since the last confirmed candidate

static void pl_dry_check(uint32_t hashed)
{
  s_pl_dry += hashed;
  if (s_pl_dry < PL_DRY_LIMIT)
    return;
  if (s_pl_loop + 1 < PL_LOOP_COUNT)
  {
    s_pl_loop = s_pl_loop + 1;
    Serial.printf("[MINER] no confirmed HW candidate in %lu nonces; HW SHA loop now: %s\n",
                  (unsigned long)s_pl_dry, s_pl_loops[s_pl_loop].name);
  }
  else
    Serial.printf("[MINER] no confirmed HW candidate in %lu nonces on the last HW SHA loop\n",
                  (unsigned long)s_pl_dry);
  s_pl_dry = 0;
}

// Per stratum job: the native header and the software midstate used to
// recheck candidates, and the cursor in the engine's (swapped) nonce space.
static uint8_t s_pl_native_hdr[80];
static uint32_t s_pl_mid[8];
static uint32_t s_pl_bake[16];

// Runs one HW chunk on the pipelined loops. Every candidate goes to the
// prefilter ring and, if it beats the job's difficulty, straight to the
// stratum task as a share (a 256K chunk at low pool difficulty often holds
// two, and they should not wait for the chunk to end).
static uint32_t pl_mine_chunk(const std::shared_ptr<JobRequest> &job, uint32_t &cursor)
{
  // Key on the header, never the job id. MiningJobStop() restarts ids, so
  // after a pool reconnect a new header can carry an old id, and a stale
  // midstate would fail every candidate. And every notify gets a new id, so
  // a pool re-sending an identical job would send the cursor back over
  // nonces already hashed, and their candidates out again as duplicates.
  if (memcmp(s_pl_ctx.hdr, job->sha_buffer, 76) != 0)
  {
    // New stratum job: reseed from its native nonce_start with the top bit
    // set, i.e. native nonces with a low byte of 0x80 and above, the half
    // the SW miner never uses (see SW_NONCE_SPAN). Within one header the
    // cursor can't run into the other half: that takes 2^31 nonces (~36 min
    // at 1 MH/s), and pools replace the header (new ntime or transactions)
    // within minutes.
    cursor =__builtin_bswap32(job->nonce_start) | 0x80000000u;
    for (int i = 0; i < 20; ++i)
      ((uint32_t *)s_pl_native_hdr)[i] = __builtin_bswap32(((const uint32_t *)job->sha_buffer)[i]);
    nerd_mids(s_pl_mid, s_pl_native_hdr);
    nerd_sha256_bake(s_pl_mid, s_pl_native_hdr + 64, s_pl_bake);
  }

  memcpy(s_pl_ctx.hdr, job->sha_buffer, sizeof(s_pl_ctx.hdr));
  s_pl_ctx.nonce = cursor;
  s_pl_ctx.budget = job->nonce_count;
  s_pl_ctx.pad = 0x80000000u;
  // Set, then look at the job id: whichever way this interleaves with the
  // stratum task switching jobs (it changes the id first and clears run
  // second), a stale chunk ends up with run == 0.
  s_pl_ctx.run = 1;
  if (s_working_current_job_id != (job->id & 0xFF))
    s_pl_ctx.run = 0;

  sha_lock_job_wait(true);
  // Whatever used the engine last (TLS before we held every engine, the
  // previous chunk's abandoned block) is finished; start from a clean one.
  periph_module_reset(PERIPH_SHA_MODULE);

  while (s_pl_ctx.run && s_pl_ctx.budget > 0)
  {
    s_pl_ctx.has_hash = 0;
    if (!s_pl_loops[s_pl_loop].fn((volatile uint32_t *)SHA_TEXT_BASE, &s_pl_ctx))
      break;

    // The fast loops only look at 16 bits of the digest and don't keep it;
    // recompute the candidate (the nonce before the one it stopped on) in
    // software. All of them are published, confirmed or not: the Monitor's
    // mismatched count is then the number of false HW candidates.
    const uint32_t cand_native = __builtin_bswap32(s_pl_ctx.nonce - 1);
    memcpy(s_pl_native_hdr + 76, &cand_native, sizeof(cand_native));
    uint8_t sw_hash[32];
    const bool confirmed = nerd_sha256d_baked(s_pl_mid, s_pl_native_hdr + 64, s_pl_bake, sw_hash);
    if (!confirmed)
      memset(sw_hash, 0, sizeof(sw_hash));
    prefilter_publish(s_pl_native_hdr, cand_native, s_pl_ctx.has_hash ? s_pl_ctx.hash : sw_hash);
    pl_health(confirmed);
    if (confirmed)
      s_pl_dry = 0;

    const double cand_diff = confirmed && isSha256Valid(sw_hash) ? diff_from_target(sw_hash) : 0.0;
    if (cand_diff > job->difficulty)
    {
      std::shared_ptr<JobResult> share = std::make_shared<JobResult>();
      share->id = job->id;
      share->nonce = cand_native;
      share->nonce_count = 0;
      share->difficulty = cand_diff;
      memcpy(share->hash, sw_hash, sizeof(sw_hash));
      std::lock_guard<std::mutex> lock(s_job_mutex);
      pushJobResult(share, 64);
    }
  }
  sha_unlock_job(true);

  // The next chunk of this job continues from here instead of re-deriving a
  // (possibly overlapping) start from its own native nonce_start.
  cursor = s_pl_ctx.nonce;
  const uint32_t hashed = job->nonce_count - s_pl_ctx.budget;
  pl_dry_check(hashed);
  return hashed;
}
#endif // PIPELINED_CLASSIC

void minerWorkerHw(void * task_id)
{
  unsigned int miner_id = (uint32_t)task_id;
  Serial.printf("[MINER] %d Started minerWorkerHwEsp32D Task!\n", miner_id);

  std::shared_ptr<JobRequest> job;
  std::shared_ptr<JobResult> result;
  #ifndef PIPELINED_CLASSIC
  uint8_t hash[32];
  uint8_t sha_buffer[128];
  uint8_t native_header[80];
  #else
  uint32_t pl_cursor = 0;
  pl_select_loop();
  #endif

  while (1)
  {
#ifdef PIPELINED_CLASSIC
    if (s_hw_pause_requested.load()) {
      s_hw_paused.store(true);
      while (s_hw_pause_requested.load()) {
        esp_task_wdt_reset();
        vTaskDelay(1);
      }
      s_hw_paused.store(false);
    }
#endif
    {
      std::lock_guard<std::mutex> lock(s_job_mutex);
      if (result)
      {
        pushJobResult(result, 16);
        result.reset();
      }
      if (!s_job_request_list_hw.empty())
      {
        job = s_job_request_list_hw.front();
        s_job_request_list_hw.pop_front();
      } else
        job.reset();
    }
    if (job)
    {
      result = std::make_shared<JobResult>();
      result->id = job->id;
      result->nonce = 0xFFFFFFFF;
      result->nonce_count = job->nonce_count;
      result->difficulty = job->difficulty;
#ifdef PIPELINED_CLASSIC
      result->nonce_count = pl_mine_chunk(job, pl_cursor);
#else
      uint8_t job_in_work = job->id & 0xFF;
      memcpy(sha_buffer, job->sha_buffer, 80);
      // job->sha_buffer is word-swapped for the engine; verifyShareSw() (the
      // Monitor's check of every published hit) hashes the header as it goes
      // on the wire.
      for (int i = 0; i < 20; ++i)
        ((uint32_t*)native_header)[i] = __builtin_bswap32(((const uint32_t*)sha_buffer)[i]);

      bool lock_all = s_sha_lock_all;
      sha_lock_job_wait(lock_all);
      for (uint32_t n = 0; n < job->nonce_count; ++n)
      {
        //((uint32_t*)(sha_buffer+64+12))[0] = __builtin_bswap32(job->nonce_start+n);

        //sha_hal_hash_block(SHA2_256, s_test_buffer, 64/4, true);
        //nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256(sha_buffer);
        sha_ll_start_block(SHA2_256);

        //sha_hal_hash_block(SHA2_256, s_test_buffer+64, 64/4, false);
        nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256_upper(sha_buffer+64, job->nonce_start+n);
        sha_ll_continue_block(SHA2_256);

        nerd_sha_hal_wait_idle();
        sha_ll_load(SHA2_256);

        //sha_hal_hash_block(SHA2_256, interResult, 64/4, true);
        nerd_sha_hal_wait_idle();
        nerd_sha_ll_fill_text_block_sha256_double();
        sha_ll_start_block(SHA2_256);

        nerd_sha_hal_wait_idle();
        sha_ll_load(SHA2_256);
        if (nerd_sha_ll_read_digest_swap_if(hash))
        {
          //~5 per second
          prefilter_publish(native_header, job->nonce_start + n, hash);
          double diff_hash = diff_from_target(hash);
          if (diff_hash > result->difficulty)
          {
            if (isSha256Valid(hash))
            {
              result->difficulty = diff_hash;
              result->nonce = job->nonce_start+n;
              memcpy(result->hash, hash, sizeof(hash));
            }
          }
        }
        if (
             (uint8_t)(n & 0xFF) == 0 &&
             s_working_current_job_id != job_in_work)
        {
          result->nonce_count = n+1;
          break;
        }
      }
      sha_unlock_job(lock_all);
#endif // PIPELINED_CLASSIC
    } else
      vTaskDelay(2 / portTICK_PERIOD_MS);

    esp_task_wdt_reset();
  }
}

#endif  //CONFIG_IDF_TARGET_ESP32

#endif  //HARDWARE_SHA265


#ifdef PANICMINER_UI
#define DELAY 40   // LVGL frame period: 25 fps for the one animation
#else
#define DELAY 100
#endif
#define REDRAW_EVERY 10

void restoreStat() {
  if(!Settings.saveStats) return;
  esp_err_t ret = nvs_flash_init();
  if (ret != ESP_OK) { Serial.println("[STATS] NVS unavailable; keeping configuration"); return; }
  ret = nvs_open("state", NVS_READWRITE, &stat_handle);
  if (ret != ESP_OK) return;
  s_stats_ready = true;

  size_t required_size = sizeof(double);
  nvs_get_blob(stat_handle, "best_diff", &best_diff, &required_size);
  uint32_t restored_mhashes = 0;
  nvs_get_u32(stat_handle, "Mhashes", &restored_mhashes);
  uint32_t nv_shares = 0, nv_valids = 0;
  nvs_get_u32(stat_handle, "shares", &nv_shares);
  nvs_get_u32(stat_handle, "valids", &nv_valids);
  shares = nv_shares;
  valids = nv_valids;
  nvs_get_u32(stat_handle, "templates", &templates);
  nvs_get_u64(stat_handle, "upTime", &upTime);

  uint32_t crc = crc32_reset();
  crc = crc32_add(crc, &best_diff, sizeof(best_diff));
  crc = crc32_add(crc, &restored_mhashes, sizeof(restored_mhashes));
  crc = crc32_add(crc, &nv_shares, sizeof(nv_shares));
  crc = crc32_add(crc, &nv_valids, sizeof(nv_valids));
  crc = crc32_add(crc, &templates, sizeof(templates));
  crc = crc32_add(crc, &upTime, sizeof(upTime));
  crc = crc32_finish(crc);

  uint32_t nv_crc = 0;
  esp_err_t crc_result = nvs_get_u32(stat_handle, "crc32", &nv_crc);
  if (crc_result != ESP_OK || nv_crc != crc || !isfinite(best_diff) || best_diff < 0)
  {
    best_diff = 0.0;
    restored_mhashes = 0;
    shares = 0;
    valids = 0;
    templates = 0;
    upTime = 0;
  }
  {
    std::lock_guard<std::mutex> lock(s_work_mutex);
    Mhashes = restored_mhashes;
  }
  uint32_t accepted = 0, rejected = 0;
  nvs_get_u32(stat_handle, "accepted", &accepted);
  nvs_get_u32(stat_handle, "rejected", &rejected);
  s_accepted_submissions = accepted;
  s_rejected_submissions = rejected;
  Serial.printf("[STATS] restored best %.6g, accepted %lu, rejected %lu\n",
      best_diff, (unsigned long)accepted, (unsigned long)rejected);
}

void saveStat() {
  if(!Settings.saveStats || !s_stats_ready) return;
  best_diff = minerBestDifficulty();
  const double saved_best = best_diff;
  const uint32_t saved_mhashes = minerTotalHashes() / 1000000, saved_templates = templates;
  const uint64_t saved_uptime = upTime;
  const uint32_t nv_shares = shares, nv_valids = valids;
  esp_err_t result = ESP_OK;
  auto keep = [&](esp_err_t r) { if (r != ESP_OK) result = r; };
  Serial.printf("[MONITOR] Saving stats\n");
  keep(nvs_set_blob(stat_handle, "best_diff", &saved_best, sizeof(saved_best)));
  keep(nvs_set_u32(stat_handle, "Mhashes", saved_mhashes));
  keep(nvs_set_u32(stat_handle, "shares", nv_shares));
  keep(nvs_set_u32(stat_handle, "valids", nv_valids));
  keep(nvs_set_u32(stat_handle, "templates", saved_templates));
  keep(nvs_set_u64(stat_handle, "upTime", saved_uptime));
  keep(nvs_set_u32(stat_handle, "accepted", s_accepted_submissions));
  keep(nvs_set_u32(stat_handle, "rejected", s_rejected_submissions));

  uint32_t crc = crc32_reset();
  crc = crc32_add(crc, &saved_best, sizeof(saved_best));
  crc = crc32_add(crc, &saved_mhashes, sizeof(saved_mhashes));
  crc = crc32_add(crc, &nv_shares, sizeof(nv_shares));
  crc = crc32_add(crc, &nv_valids, sizeof(nv_valids));
  crc = crc32_add(crc, &saved_templates, sizeof(saved_templates));
  crc = crc32_add(crc, &saved_uptime, sizeof(saved_uptime));
  crc = crc32_finish(crc);
  keep(nvs_set_u32(stat_handle, "crc32", crc));
  if (result == ESP_OK) result = nvs_commit(stat_handle);
  Serial.printf("[STATS] checkpoint %s: best %.6g\n", esp_err_to_name(result), saved_best);
}

void resetStat() {
    Serial.printf("[MONITOR] Resetting NVS stats\n");
    {
      std::lock_guard<std::mutex> lock(s_work_mutex);
      hashes = Mhashes = 0;
    }
    templates = totalKHashes = elapsedKHs = upTime = shares = valids = 0;
    best_diff = 0.0;
    s_best_hash_diff = 0.0;
    s_accepted_submissions = 0;
    s_rejected_submissions = 0;
    saveStat();
}

void runMonitor(void *name)
{

  Serial.println("[MONITOR] started");
  restoreStat();

  unsigned long mLastCheck = millis();

  resetToFirstScreen();

  unsigned long frame = 0;

  uint32_t seconds_elapsed = 0;

  totalKHashes = (uint32_t)(minerTotalHashes() / 1000);
  uint32_t last_update_millis = millis();
  uint32_t uptime_frac = 0;

  while (1)
  {
    // Every frame, not every second: at ~1 MH/s about 15 candidates a second
    // reach the 16-slot ring, and a once-a-second reader would lose some.
    prefilter_consume();
    uint32_t now_millis = millis();
    if (now_millis < last_update_millis)
      now_millis = last_update_millis;
    
    uint32_t mElapsed = now_millis - mLastCheck;
    if (mElapsed >= 1000)
    { 
      mLastCheck = now_millis;
      last_update_millis = now_millis;
      unsigned long currentKHashes = (uint32_t)(minerTotalHashes() / 1000);
      elapsedKHs = currentKHashes - totalKHashes;
      totalKHashes = currentKHashes;

      uptime_frac += mElapsed;
      while (uptime_frac >= 1000)
      {
        uptime_frac -= 1000;
        upTime ++;
      }

      if (upTime % 10 == 0)
        Serial.printf("[SHA] prefilter hits checked %lu, mismatched %lu, lock %s, dropped %lu\n",
                      (unsigned long)s_hw_checked, (unsigned long)s_hw_mismatched,
                      minerShaLockAllOn() ? "all" : "sha256", (unsigned long)s_hw_dropped);
      drawCurrentScreen(mElapsed);

      // Monitor state when hashrate is 0.0
      if (elapsedKHs == 0)
      {
        Serial.printf(">>> [i] Miner: newJob>%s / inRun>%s) - Client: connected>%s / subscribed>%s / wificonnected>%s\n",
            "true",//(1) ? "true" : "false",
            isMinerSuscribed ? "true" : "false",
            client.connected() ? "true" : "false", isMinerSuscribed ? "true" : "false", WiFi.status() == WL_CONNECTED ? "true" : "false");
      }

      #ifdef DEBUG_MEMORY
      Serial.printf("### [Total Heap / Free heap / Min free heap]: %d / %d / %d \n", ESP.getHeapSize(), ESP.getFreeHeap(), ESP.getMinFreeHeap());
      Serial.printf("### Max stack usage: %d\n", uxTaskGetStackHighWaterMark(NULL));
      #endif

      seconds_elapsed++;

#ifdef PANICMINER_UI
      if(seconds_elapsed >= 300){
#else
      if(seconds_elapsed % (saveIntervals[currentIntervalIndex]) == 0){
#endif
        saveStat();
        seconds_elapsed = 0;
        if(currentIntervalIndex < saveIntervalsSize - 1)
          currentIntervalIndex++;
      }    
    }
    animateCurrentScreen(frame);
    doLedStuff(frame);

    vTaskDelay(DELAY / portTICK_PERIOD_MS);
    frame++;
  }
}
