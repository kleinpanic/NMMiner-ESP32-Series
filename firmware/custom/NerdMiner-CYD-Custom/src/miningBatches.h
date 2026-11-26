#pragma once
#include <stdint.h>

struct MiningBatchProgress { uint32_t completed; bool continuing; };
struct MiningBatchResult { uint32_t completed; bool valid; };

// Count actual completed work in bounded steps, independently of network-task
// result delivery. Never extrapolate a rate or credit an unprocessed nonce.
template<class Execute, class Record>
MiningBatchResult runMiningBatches(uint32_t nonces, Execute execute, Record record)
{
    uint32_t done = 0;
    while (done < nonces) {
        uint32_t remaining = nonces - done;
        uint32_t requested = remaining < 32768 ? remaining : 32768;
        MiningBatchProgress progress = execute(requested);
        if (progress.completed > requested || (!progress.completed && progress.continuing))
            return {done, false};
        if (progress.completed) record(progress.completed);
        done += progress.completed;
        if (!progress.continuing) break;
    }
    return {done, true};
}
