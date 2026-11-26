#pragma once
#include <stdint.h>

struct StatusLedColor { bool red, green, blue; };
class StatusLed {
    uint32_t accepted_ = 0, share_at_ = 0;
    bool initialized_ = false, flashing_ = false;
public:
    StatusLedColor sample(uint32_t now, bool wifi, bool authorized, uint32_t accepted) {
        if (!initialized_) { accepted_ = accepted; initialized_ = true; }
        else if (accepted > accepted_) { share_at_ = now; flashing_ = true; }
        accepted_ = accepted;
        if (flashing_ && uint32_t(now - share_at_) >= 800) flashing_ = false;
        if (!wifi) return {bool((now / 1000) % 2), false, false};
        if (!authorized) return {true, true, false};
        if (flashing_) return {false, true, true};
        return {false, true, false};
    }
};
