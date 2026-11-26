#pragma once
#include <stdint.h>

// Resistive touch can briefly lose contact while a finger is still down.
// Require a stable press and a full release; never repeat a held press.
class TouchPager {
    bool down_ = false, candidate_ = false;
    uint32_t first_ = 0, seen_ = 0, fired_ = 0;
    int direction_ = 0;
public:
    int sample(uint32_t now, bool pressed, int x, int y) {
        if (!pressed || x < 0 || x >= 320 || y < 0 || y >= 240) {
            candidate_ = false;
            if (now - seen_ >= 300) down_ = false;
            return 0;
        }
        seen_ = now;
        if (down_) return 0;
        int direction = x < 152 ? -1 : x >= 168 ? 1 : 0;
        if (!direction) { candidate_ = false; return 0; }
        if (!candidate_ || direction != direction_) {
            first_ = now;
            direction_ = direction;
            candidate_ = true;
            return 0;
        }
        if (now - first_ < 60 || (fired_ && now - fired_ < 500)) return 0;
        down_ = true;
        candidate_ = false;
        fired_ = now;
        return direction;
    }
};
