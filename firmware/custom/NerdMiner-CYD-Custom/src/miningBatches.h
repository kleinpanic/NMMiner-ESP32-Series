#pragma once
#include <stdint.h>

struct MiningBatchProgress { uint32_t completed; bool continuing; };
struct MiningBatchResult { uint32_t completed; bool valid; };

// The classic worker owns a continuous nonce cursor. A network-task queue
// refill is not needed to continue the same valid header. Invalidate by the
// full generation, including an explicit stopped state, before reusing work.
template<class Queue, class Job>
bool takeContinuousMiningJob(Queue& pending, Job& current, uint32_t active)
{
    bool reused = pending.empty() && bool(current);
    if (!pending.empty()) {
        current = pending.front();
        pending.pop_front();
    }
    if (current && (active == UINT32_MAX || current->id != active)) current.reset();
    return reused && bool(current);
}

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
