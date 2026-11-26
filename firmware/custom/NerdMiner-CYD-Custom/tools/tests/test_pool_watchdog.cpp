#include "poolWatchdog.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    unsigned cases = 0;
    constexpr uint32_t timeout = 60000;
    {
        PoolProgressWatchdog watchdog;
        assert(!watchdog.sample(0, 0, timeout));
        assert(!watchdog.sample(59999, 0, timeout));
        assert(watchdog.sample(60000, 0, timeout));
        ++cases;
    }
    {
        PoolProgressWatchdog watchdog;
        assert(!watchdog.sample(1000, 100, timeout));
        assert(!watchdog.sample(60000, 101, timeout));
        assert(!watchdog.sample(119999, 101, timeout));
        assert(watchdog.sample(120000, 101, timeout));
        ++cases;
    }
    {
        PoolProgressWatchdog watchdog;
        assert(!watchdog.sample(0, 0, timeout));
        // Sample at varying phases of the monitor's one-second interval.
        // Hash progress is cumulative, so equal per-second work is not a stall.
        for (uint32_t i = 1; i < 5000; i++)
            assert(!watchdog.sample(i * 53, (i * 53 / 1000) * 980, timeout));
        ++cases;
    }
    {
        PoolProgressWatchdog watchdog;
        assert(!watchdog.sample(100, 9000, timeout));
        assert(watchdog.sample(60100, 9000, timeout));
        watchdog.reset();
        assert(!watchdog.sample(200000, 9000, timeout));
        assert(!watchdog.sample(259999, 9000, timeout));
        assert(watchdog.sample(260000, 9000, timeout));
        ++cases;
    }
    {
        PoolProgressWatchdog watchdog;
        constexpr uint32_t before_wrap = UINT32_MAX - 30000;
        assert(!watchdog.sample(before_wrap, 55, timeout));
        assert(!watchdog.sample(before_wrap + 59999, 55, timeout));
        assert(watchdog.sample(before_wrap + 60000, 55, timeout));
        ++cases;
    }
    {
        PoolProgressWatchdog watchdog;
        assert(!watchdog.sample(100, UINT32_MAX - 20, timeout));
        assert(!watchdog.sample(59000, 100, timeout)); // work counter wraps
        assert(!watchdog.sample(118999, 100, timeout));
        assert(watchdog.sample(119000, 100, timeout));
        ++cases;
    }
    {
        PoolProgressWatchdog watchdog;
        assert(!watchdog.sample(100, 10, timeout));
        assert(watchdog.sample(60100, 10, timeout));
        assert(!watchdog.sample(60200, 11, timeout)); // progress recovers immediately
        assert(!watchdog.sample(120199, 11, timeout));
        assert(watchdog.sample(120200, 11, timeout));
        ++cases;
    }
    std::cout << "PASS pool watchdog: " << cases << " progress/stall/reset/wrap scenarios\n";
}
