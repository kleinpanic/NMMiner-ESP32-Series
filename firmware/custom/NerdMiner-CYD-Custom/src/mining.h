
#ifndef MINING_API_H
#define MINING_API_H

// Mining
#define MAX_NONCE_STEP  5000000U
#define MAX_NONCE       25000000U
#define TARGET_NONCE    471136297U
#define DEFAULT_DIFFICULTY  0.00015
#define KEEPALIVE_TIME_ms       30000
#define POOLINACTIVITY_TIME_ms  60000

//#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
#define HARDWARE_SHA265
//#endif

// With the pipelined classic-ESP32 HW loop, core 1 belongs to the HW miner
// alone (see the task creation in NerdMinerV2.ino.cpp); Monitor, Stratum and
// ChainFeed run on APP_TASK_CORE.
#include <sdkconfig.h>
#if defined(PIPELINED_ASM_MINING) && defined(CONFIG_IDF_TARGET_ESP32)
#define HW_MINER_OWNS_CORE1
#define APP_TASK_CORE 0
#else
#define APP_TASK_CORE 1
#endif

#define TARGET_BUFFER_SIZE 64

void runMonitor(void *name);

void runStratumWorker(void *name);
void runMiner(void *name);

void minerWorkerSw(void * task_id);
void minerWorkerHw(void * task_id);
bool minerIsSubscribed(void);
const char* minerPoolHost(void);
int minerPoolPort(void);
bool minerUsingFallback(void);
uint32_t minerRejectedShares(void); // rejected tracked pool submissions this boot
uint32_t minerAcceptedShares(void); // every pool-confirmed share, this boot
double minerBestDifficulty(void); // best verified candidate, includes saved best
float minerPoolDifficulty(void);
uint64_t minerTotalHashes(void); // coherent lifetime work, including current job
bool minerLiveHash(uint8_t out[32]);   // newest verified prefilter hash, big-endian
bool minerBestHash(uint8_t out[32]);   // best verified hash since boot, big-endian
void minerShaStats(uint32_t* checked, uint32_t* mismatched);
void minerShaLockAll(bool on);
bool minerShaLockAllOn(void);
bool minerHwPauseForChain(bool pause); // diagnostic only; acknowledged outside SHA locks

String printLocalTime(void);

void resetStat();
void saveStat();

typedef struct{
  uint8_t bytearray_target[32];
  uint8_t bytearray_pooltarget[32];
  uint8_t merkle_result[32];
  uint8_t bytearray_blockheader[128];
} miner_data;


#endif // UTILS_API_H
