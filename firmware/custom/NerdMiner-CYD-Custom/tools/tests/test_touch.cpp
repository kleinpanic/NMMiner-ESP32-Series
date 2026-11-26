#include "touchPager.h"
#include <cassert>
#include <cstdint>
#include <iostream>

static unsigned cases;
static int press(TouchPager& pager, uint32_t now, int x = 250, int y = 100) {
    return pager.sample(now, true, x, y);
}
static void release(TouchPager& pager, uint32_t now) {
    assert(pager.sample(now, false, -1, -1) == 0);
}

int main() {
    {
        TouchPager pager;
        assert(press(pager, 100) == 0);
        assert(press(pager, 140) == 0);
        assert(press(pager, 160) == 1);
        ++cases;
        for (uint32_t t = 200; t < 4200; t += 40)
            assert(press(pager, t) == 0);
        ++cases;
    }
    {
        TouchPager pager;
        assert(press(pager, 100, 50) == 0);
        assert(press(pager, 160, 50) == -1);
        assert(press(pager, 200, 290) == 0); // drag across the center
        assert(press(pager, 300, 20) == 0);
        ++cases;
    }
    {
        TouchPager pager;
        assert(press(pager, 100) == 0);
        assert(press(pager, 160) == 1);
        for (uint32_t t = 200; t < 5000; t += 290) {
            release(pager, t + 60);
            release(pager, t + 210);
            assert(press(pager, t + 250) == 0);
        }
        ++cases; // repeated short contact losses never repeat a held press
    }
    {
        TouchPager pager;
        assert(press(pager, 100) == 0);
        assert(press(pager, 160) == 1);
        release(pager, 460); // actual release
        assert(press(pager, 470, 40) == 0);
        assert(press(pager, 530, 40) == 0); // cooldown still active
        assert(press(pager, 659, 40) == 0);
        assert(press(pager, 660, 40) == -1);
        assert(press(pager, 1000, 40) == 0);
        ++cases;
    }
    {
        TouchPager pager;
        for (uint32_t t = 100; t < 2000; t += 40)
            assert(press(pager, t, 160) == 0); // neutral band
        assert(press(pager, 2100, 151) == 0);
        assert(press(pager, 2160, 151) == -1);
        release(pager, 2500);
        assert(press(pager, 2700, 168) == 0);
        assert(press(pager, 2760, 168) == 1);
        ++cases;
    }
    {
        TouchPager pager;
        const int bad[][2] = {{-1,100},{320,100},{100,-1},{100,240},{32767,32767}};
        uint32_t t = 100;
        for (const auto& xy : bad) {
            assert(press(pager, t, xy[0], xy[1]) == 0);
            assert(press(pager, t + 80, xy[0], xy[1]) == 0);
            t += 100;
        }
        assert(press(pager, 700, 0, 0) == 0);
        assert(press(pager, 760, 0, 0) == -1);
        ++cases;
    }
    {
        TouchPager pager;
        assert(press(pager, 100) == 0);
        release(pager, 120);
        assert(press(pager, 140) == 0);
        assert(press(pager, 180) == 0);
        assert(press(pager, 200) == 1); // noisy initial contact must stabilize
        ++cases;
    }
    {
        TouchPager pager;
        constexpr uint32_t start = UINT32_MAX - 30;
        assert(press(pager, start) == 0);
        assert(press(pager, start + 40) == 0);
        assert(press(pager, start + 60) == 1);
        release(pager, start + 360);
        assert(press(pager, start + 380, 50) == 0);
        assert(press(pager, start + 440, 50) == 0);
        assert(press(pager, start + 560, 50) == -1);
        ++cases;
    }
    std::cout << "PASS touch: " << cases << " regression scenarios\n";
}
