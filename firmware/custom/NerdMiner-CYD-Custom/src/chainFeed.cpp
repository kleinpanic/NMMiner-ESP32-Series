// Bitcoin chain data for the PanicMiner Chain page, from mempool.space only.
// A task fetches price, tip height, difficulty, network hashrate, the next
// retarget and fees over one TLS connection, verified against a pinned root
// CA. Reuse the connection between updates: repeating the RSA handshake on
// every update takes CPU time from mining and can exceed the server timeout.
#ifdef PANICMINER_UI

#include <stdlib.h>
// The SDK's C allocator header has no C++ linkage guard. Include it before
// mbedTLS's config pulls it in, preserving the SDK symbols' C linkage.
extern "C" {
#include <esp_mem.h>
}
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <atomic>
#include <mbedtls/platform.h>
#include <mbedtls/ssl_internal.h>

#include "chainFeed.h"
#include "mining.h"
#include "monitor.h"
#include "walletStats.h"
#include "tlsRecordBuffers.h"
#include "tlsScratchBuffer.h"
#include "chainHttp.h"
#include "drivers/storage/storage.h"
extern TSettings Settings;

#define PRICE_API "https://mempool.space/api/v1/prices"
#define PERIOD_MS (45 * 1000)
#define RETRY_MS (10 * 1000)

// mempool.space chains to USERTrust RSA Certification Authority (expires
// 2038-01-18, SHA-256 E7:93:C9:B0:...:CB:D2) through Sectigo Root R46.
static const char ROOT_CA[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n"
    "iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
    "cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
    "BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n"
    "MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n"
    "BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n"
    "aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n"
    "dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n"
    "AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n"
    "3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n"
    "tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n"
    "Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n"
    "VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n"
    "79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n"
    "c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n"
    "Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n"
    "c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n"
    "UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n"
    "Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n"
    "BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n"
    "A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n"
    "Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n"
    "VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n"
    "ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n"
    "8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n"
    "iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n"
    "Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n"
    "XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n"
    "qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n"
    "VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n"
    "L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n"
    "jjxDah2nGN59PRbxYvnKkKj9\n"
    "-----END CERTIFICATE-----\n"
    ;

static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_task;
static chain_snapshot_t s_snap;
static std::atomic<bool> s_pause_test{false};
static TlsRecordBuffers s_records;
static bool s_prepared;
static bool s_records_ready;
static uint32_t s_record_uses;
static TlsScratchBuffer s_scratch;
static uint32_t s_scratch_fallbacks;

static void* feed_calloc(size_t count, size_t size)
{
    if (size && count > SIZE_MAX / size) return nullptr;
    size_t bytes = count * size;
    if (chainFeedIsCurrentTask()) {
        if (s_records_ready &&
            (bytes == MBEDTLS_SSL_IN_BUFFER_LEN || bytes == MBEDTLS_SSL_OUT_BUFFER_LEN)) {
            void* buffer = s_records.acquire(count, size);
            if (buffer) ++s_record_uses;
            return buffer;
        }
        if (void* buffer = s_scratch.calloc(count, size)) return buffer;
        ++s_scratch_fallbacks;
    }
    void* buffer = esp_mbedtls_mem_calloc(count, size);
    if (!buffer && chainFeedIsCurrentTask())
        Serial.printf("[CHAIN] allocation failed: %u bytes\n", (unsigned)bytes);
    return buffer;
}

static void feed_free(void* ptr)
{
    if (s_records_ready && s_records.release(ptr)) return;
    if (s_scratch.release(ptr)) return;
    esp_mbedtls_mem_free(ptr);
}

void chainFeedPrepare(void)
{
    if (s_prepared) return;
    s_prepared = true;
    void* input = esp_mbedtls_mem_calloc(1, MBEDTLS_SSL_IN_BUFFER_LEN);
    void* output = esp_mbedtls_mem_calloc(1, MBEDTLS_SSL_OUT_BUFFER_LEN);
    s_records_ready = s_records.init(input, MBEDTLS_SSL_IN_BUFFER_LEN,
                                    output, MBEDTLS_SSL_OUT_BUFFER_LEN);
    if (!s_records_ready) {
        esp_mbedtls_mem_free(input);
        esp_mbedtls_mem_free(output);
        Serial.println("[CHAIN] record buffer reservation failed");
        return;
    }
    // RSA/certificate scratch allocations need coalescing space too. Keep
    // their allocator separate from long-lived SD/network allocations.
    constexpr size_t scratch_bytes = 24 * 1024;
    void* scratch = esp_mbedtls_mem_calloc(1, scratch_bytes);
    if (!s_scratch.init(scratch, scratch_bytes)) {
        esp_mbedtls_mem_free(scratch);
        Serial.println("[CHAIN] scratch reservation failed; using SDK fallback");
    }
    // Supported mbedTLS allocator hook. Other tasks retain the SDK's
    // allocator; certificate checks are unchanged.
    mbedtls_platform_set_calloc_free(feed_calloc, feed_free);
    Serial.printf("[CHAIN] reserved record buffers %u + %u bytes\n",
                  (unsigned)MBEDTLS_SSL_IN_BUFFER_LEN, (unsigned)MBEDTLS_SSL_OUT_BUFFER_LEN);
}

static bool get_body(HTTPClient& http, WiFiClientSecure& tls, const char* url, String& body,
                     const char* label = nullptr)
{
    return chainGetBody(http, tls, url, body, [&](int code, unsigned attempt) {
        // HTTPClient reports DNS, TCP, TLS and certificate failures all as -1;
        // mbedTLS's own error and the heap tell them apart.
        char err[100] = "";
        tls.lastError(err, sizeof(err));
        Serial.printf("[CHAIN] %s attempt %u -> %d %s; heap %u free, largest block %u\n", label ? label : url, attempt, code, err,
                      (unsigned)ESP.getFreeHeap(), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    }, [] { vTaskDelay(pdMS_TO_TICKS(250)); });
}

static void fetch_all(HTTPClient& http, WiFiClientSecure& tls)
{
    chain_snapshot_t n;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    n = s_snap;
    xSemaphoreGive(s_mutex);

    uint32_t t0 = millis();
    StaticJsonDocument<512> doc;
    String body;
    int ok = 0;

    if (get_body(http, tls, PRICE_API, body) && !deserializeJson(doc, body)) {
        n.price_usd = doc["USD"] | n.price_usd;
        ok++;
    }
    if (get_body(http, tls, getHeightAPI, body)) {
        uint32_t h = strtoul(body.c_str(), NULL, 10);
        if (h) { n.height = h; ok++; }
    }
    StaticJsonDocument<192> filter;
    filter["currentHashrate"] = true;
    filter["currentDifficulty"] = true;
    if (get_body(http, tls, getGlobalHash, body)
        && !deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
        n.hashrate = doc["currentHashrate"] | n.hashrate;
        n.difficulty = doc["currentDifficulty"] | n.difficulty;
        ok++;
    }
    if (get_body(http, tls, getDifficulty, body) && !deserializeJson(doc, body)) {
        n.retarget_progress = doc["progressPercent"] | n.retarget_progress;
        n.retarget_change_pct = doc["difficultyChange"] | n.retarget_change_pct;
        n.retarget_blocks = doc["remainingBlocks"] | n.retarget_blocks;
        ok++;
    }
    if (get_body(http, tls, getFees, body) && !deserializeJson(doc, body)) {
        n.fee_fast = doc["fastestFee"] | n.fee_fast;
        n.fee_hour = doc["hourFee"] | n.fee_hour;
        n.fee_eco = doc["economyFee"] | n.fee_eco;
        ok++;
    }
    char address[91];
    if (walletAddress(Settings.BtcWallet, address, sizeof(address))) {
        char url[140];
        snprintf(url, sizeof(url), "https://mempool.space/api/address/%s", address);
        doc.clear();
        filter.clear();
        filter["chain_stats"]["funded_txo_sum"] = true;
        filter["chain_stats"]["spent_txo_sum"] = true;
        filter["mempool_stats"]["funded_txo_sum"] = true;
        filter["mempool_stats"]["spent_txo_sum"] = true;
        n.wallet_attempted = true;
        if (get_body(http, tls, url, body, "wallet balance") &&
            !deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
            JsonObject chain = doc["chain_stats"], pending = doc["mempool_stats"];
            int64_t balance, delta;
            if (chain["funded_txo_sum"].is<uint64_t>() && chain["spent_txo_sum"].is<uint64_t>() &&
                pending["funded_txo_sum"].is<uint64_t>() && pending["spent_txo_sum"].is<uint64_t>() &&
                walletNetSats(chain["funded_txo_sum"], chain["spent_txo_sum"], balance) && balance >= 0 &&
                walletNetSats(pending["funded_txo_sum"], pending["spent_txo_sum"], delta)) {
                n.wallet_valid = true;
                n.wallet_sats = balance;
                n.wallet_pending_sats = delta;
                n.wallet_updated_ms = millis();
            }
        }
    }
    n.valid = n.height && n.price_usd > 0 && n.difficulty > 0;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_snap = n;
    xSemaphoreGive(s_mutex);
    Serial.printf("[CHAIN] %d/5 ok in %lu ms: block %lu, $%.0f, difficulty %.4g, fees %d/%d/%d\n",
                  ok, (unsigned long)(millis() - t0), (unsigned long)n.height, n.price_usd,
                  n.difficulty, n.fee_fast, n.fee_hour, n.fee_eco);
    Serial.printf("[CHAIN] stack minimum free %u bytes; wallet %s\n",
                  (unsigned)uxTaskGetStackHighWaterMark(NULL), n.wallet_valid ? "loaded" : "unavailable");
    Serial.printf("[CHAIN] reserved record uses %lu\n", (unsigned long)s_record_uses);
    Serial.printf("[CHAIN] scratch used %u, peak %u, SDK fallbacks %lu\n",
                  (unsigned)s_scratch.used(), (unsigned)s_scratch.peak(),
                  (unsigned long)s_scratch_fallbacks);
}

static void feed_loop(void*)
{
    WiFiClientSecure tls;
    tls.setCACert(ROOT_CA);
    HTTPClient http;
    http.setReuse(true);
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            bool test = s_pause_test.exchange(false);
            bool paused = test && minerHwPauseForChain(true);
            if (test) Serial.printf("[CHAIN] controlled test: HW miner %s\n",
                                    paused ? "paused outside SHA locks" : "pause FAILED; test skipped");
            if (!test || paused) fetch_all(http, tls);
            if (paused) {
                minerHwPauseForChain(false);
                Serial.println("[CHAIN] controlled test: HW miner resumed");
            }
        } else {
            // A disconnected network cannot retain a usable TLS session.
            http.end();
            tls.stop();
        }
        // A PMFEED command wakes the task early.
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(s_snap.valid ? PERIOD_MS : RETRY_MS));
    }
}

void chainFeedStart(void)
{
    if (s_task) return;
    chainFeedPrepare();
    s_mutex = xSemaphoreCreateMutex();
    // TLS hashes with the SHA-1 and SHA-512 engines, which share SHA_TEXT with
    // the miner's SHA-256 engine. With every engine held per job, mbedTLS finds
    // them busy and hashes in software instead (see minerShaLockAll).
    minerShaLockAll(true);
    // Finish the initial handshake before the server's deadline. Equal
    // priority with the software miner doubles its duration and the peer
    // resets the connection. Later updates reuse the authenticated session.
    xTaskCreatePinnedToCore(feed_loop, "ChainFeed", 10240, NULL, 2, &s_task, APP_TASK_CORE);
}

void chainFeedPoke(void)
{
    if (s_task) xTaskNotifyGive(s_task);
}

void chainFeedPokePaused(void)
{
    s_pause_test.store(true);
    chainFeedPoke();
}

bool chainFeedGet(chain_snapshot_t* out)
{
    if (!s_task) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_snap;
    xSemaphoreGive(s_mutex);
    return out->valid;
}

bool chainFeedIsCurrentTask(void)
{
    return s_task && xTaskGetCurrentTaskHandle() == s_task;
}

#endif
