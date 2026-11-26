#pragma once
#include <stdint.h>
class PoolProgressWatchdog {
    uint32_t last_work_ = 0, progress_at_ = 0;
    bool started_ = false;
public:
    void reset() { started_ = false; }
    bool sample(uint32_t now, uint32_t work, uint32_t timeout) {
        if (!started_ || work != last_work_) {
            started_ = true;
            last_work_ = work;
            progress_at_ = now;
        }
        return now - progress_at_ >= timeout;
    }
};
