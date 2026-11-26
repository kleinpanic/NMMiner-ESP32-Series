// PanicMiner UI. Pure LVGL: no Arduino calls, so tools/uisim renders the
// same source on the host. The display driver fills a ui_model_t once per
// second and calls ui_update().
#ifndef PANICMINER_UI_H
#define PANICMINER_UI_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { UI_PAGE_MINE, UI_PAGE_HASH, UI_PAGE_CHAIN, UI_PAGE_CLOCK, UI_PAGE_STATS, UI_PAGE_COUNT } ui_page_t;

typedef enum { UI_LINK_CONNECTING, UI_LINK_MINING, UI_LINK_NO_POOL } ui_link_t;

typedef struct {
    ui_link_t link;
    float hashrate_khs;
    float hashrate_60s_khs, hashrate_session_khs;
    float hashrate_min_khs, hashrate_max_khs; // 1 s samples in the last minute
    double total_mhash; // work since this boot, independent of saved lifetime stats
    uint32_t rejected, sha_checked, sha_mismatched, heap_free;
    uint32_t heap_total, sd_rows;
    float cpu_pct[2], pool_diff;
    bool cpu_valid, sd_ready, wallet_set;
    uint32_t shares;        // accepted shares
    uint32_t blocks;        // blocks found
    uint32_t jobs;          // stratum jobs received
    double best_diff;       // best share difficulty, all time
    uint32_t uptime_s;
    float temp_c;
    int rssi_dbm;
    char pool[40];
    char log_line[48];      // newest mining event, already formatted
    bool best_hash_valid;
    uint8_t best_hash[32];  // best share hash, most significant byte first
    bool live_valid;
    uint8_t live_hash[32];  // newest hash past the 16-bit prefilter, same order

    bool time_valid;
    int hour, minute, second;
    int year, month, day, weekday;  // month 1-12, weekday 0 = Sunday

    bool chain_enabled;     // the owner allows the mempool.space feed
    bool chain_valid;       // the feed has answered at least once
    uint32_t block_height;
    double net_difficulty;
    double net_hashrate;    // hashes per second
    double price_usd;
    float retarget_change_pct;
    uint32_t retarget_blocks;
    float retarget_progress; // 0-100
    int fee_fast, fee_hour, fee_eco;  // sat/vB
} ui_model_t;

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_update(const ui_model_t* m);
void ui_set_page(ui_page_t page);
void ui_next_page(void);
ui_page_t ui_current_page(void);
// The newest hash past the prefilter, most significant byte first, for the
// Hash page's live line. Call it as often as it changes, up to about 8 Hz.
void ui_live_hash(const uint8_t* hash);

void ui_show_boot(void);
void ui_boot_log(const char* line);
void ui_show_setup(const char* ap_name, const char* ap_password, const char* ap_ip);

// Formatting helpers, exposed for tests.
void ui_fmt_si(double v, char* out, int len);
void ui_fmt_commas(uint32_t v, char* out, int len);

#ifdef __cplusplus
}
#endif

#endif
