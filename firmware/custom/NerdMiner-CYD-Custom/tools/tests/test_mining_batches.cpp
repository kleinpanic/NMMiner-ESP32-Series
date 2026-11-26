#include "miningBatches.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    unsigned cases = 0;
    for (uint32_t requested : {0u, 1u, 32767u, 32768u, 32769u, 262144u, 1000000u, 0xffffffffu}) {
        uint64_t recorded = 0; unsigned calls = 0;
        auto result = runMiningBatches(requested, [&](uint32_t budget) {
            assert(budget > 0 && budget <= 32768);
            // Prior work is already visible before the next batch executes.
            assert(recorded == (uint64_t)calls * 32768);
            ++calls;
            return MiningBatchProgress{budget, true};
        }, [&](uint32_t done) { recorded += done; });
        assert(result.valid && result.completed == requested && recorded == requested);
        assert(calls == ((uint64_t)requested + 32767) / 32768);
        ++cases;
    }
    {
        // Candidates return early; count their actual work once and resume.
        uint32_t recorded = 0; unsigned calls = 0;
        auto result = runMiningBatches(262144, [&](uint32_t budget) {
            ++calls;
            return MiningBatchProgress{budget > 4096 ? 4096 : budget, true};
        }, [&](uint32_t done) { recorded += done; });
        assert(result.valid && result.completed == 262144 && recorded == 262144 && calls == 64);
        ++cases;
    }
    for (unsigned stop_after : {0u, 1u, 7u}) {
        uint32_t recorded = 0; unsigned calls = 0;
        auto result = runMiningBatches(262144, [&](uint32_t budget) {
            bool stop = calls++ == stop_after;
            return MiningBatchProgress{stop ? 123u : budget, !stop};
        }, [&](uint32_t done) { recorded += done; });
        assert(result.valid && result.completed == stop_after * 32768 + 123 && recorded == result.completed);
        ++cases;
    }
    for (MiningBatchProgress bad : {MiningBatchProgress{32769, true}, {0, true}, {0xffffffffu, false}}) {
        uint32_t recorded = 0;
        auto result = runMiningBatches(262144, [&](uint32_t) { return bad; },
            [&](uint32_t done) { recorded += done; });
        assert(!result.valid && result.completed == 0 && recorded == 0);
        ++cases;
    }
    {
        auto result = runMiningBatches(262144, [](uint32_t) { return MiningBatchProgress{0, false}; },
            [](uint32_t) { assert(false); });
        assert(result.valid && result.completed == 0);
        ++cases;
    }
    std::cout << "PASS mining batches: " << cases << " exact counts, bounded publication, early candidates, cancellation and invalid progress scenarios\n";
}
