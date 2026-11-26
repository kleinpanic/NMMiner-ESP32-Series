#pragma once
#include <atomic>
#include <stdint.h>

// Only Stratum records failures. The display/API may read the active endpoint.
class PoolFailover {
    std::atomic<bool> fallback_{false};
    uint8_t failures_ = 0;
public:
    bool usingFallback() const { return fallback_.load(std::memory_order_relaxed); }
    void healthy() { failures_ = 0; }
    bool failed(bool configured) {
        if (!configured) { failures_ = 0; return false; }
        if (++failures_ < 3) return false;
        failures_ = 0;
        fallback_.store(!usingFallback(), std::memory_order_relaxed);
        return true;
    }
};
