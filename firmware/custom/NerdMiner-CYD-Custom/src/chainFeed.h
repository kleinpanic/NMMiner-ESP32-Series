#ifndef CHAIN_FEED_H
#define CHAIN_FEED_H

#include <stdint.h>

typedef struct {
    bool valid;               // price, height and difficulty have all arrived
    uint32_t height;
    double price_usd;
    double difficulty;
    double hashrate;          // hashes per second
    float retarget_progress;  // 0-100
    float retarget_change_pct;
    uint32_t retarget_blocks;
    int fee_fast, fee_hour, fee_eco;  // sat/vB
    bool wallet_valid, wallet_attempted;
    int64_t wallet_sats, wallet_pending_sats;
    uint32_t wallet_updated_ms;
} chain_snapshot_t;

void chainFeedStart(void);    // idempotent; call once Wi-Fi is up
void chainFeedPoke(void);     // fetch now instead of at the next period
void chainFeedPokePaused(void); // diagnostic fetch with HW mining paused, then auto-resume
bool chainFeedIsCurrentTask(void);
bool chainFeedGet(chain_snapshot_t* out);

#endif
