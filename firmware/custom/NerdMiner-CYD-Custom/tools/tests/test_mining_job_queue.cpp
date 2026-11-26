#include "miningJobQueue.h"
#include "miningBatches.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

static bool denyAllocation;
void* operator new(size_t n) {
    if (denyAllocation) throw std::bad_alloc();
    if (void* p = std::malloc(n)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }

struct Job { uint32_t id, nonce; uint8_t header[256]; };
int main() {
    MiningJobQueue<Job, 4> queue;
    OwnedMiningJob<Job> current;
    denyAllocation = true;
    bool oldFailed = false;
    try { auto old = std::make_shared<Job>(); (void)old; }
    catch (const std::bad_alloc&) { oldFailed = true; }
    assert(oldFailed); // The failed native allocation, reproduced on the host.
    assert(queue.empty() && !current);
    for (uint32_t i = 0; i < 4; ++i) {
        Job j{}; j.id = 300; j.nonce = i; j.header[0] = uint8_t(i);
        assert(queue.push_back(j));
    }
    assert(!queue.push_back(Job{}) && queue.size() == 4);
    for (uint32_t i = 0; i < 4; ++i) {
        assert(!takeContinuousMiningJob(queue, current, 300));
        assert(current && current->nonce == i && current->header[0] == i);
    }
    assert(takeContinuousMiningJob(queue, current, 300));
    current->header[0] = 99;
    for (uint32_t i = 4; i < 8; ++i) {
        Job j{}; j.id = 556; j.nonce = i;
        assert(queue.push_back(j)); // Wraparound cannot overwrite the retained job.
    }
    assert(current->id == 300 && current->header[0] == 99);
    assert(!takeContinuousMiningJob(queue, current, 556) && current->nonce == 4);
    queue.clear();
    assert(!takeContinuousMiningJob(queue, current, 300) && !current);
    Job j{}; j.id = 556; assert(queue.push_back(j));
    assert(!takeContinuousMiningJob(queue, current, UINT32_MAX) && !current);
    queue.pop_front(); assert(queue.empty());
    denyAllocation = false;
    std::puts("PASS mining job queue: allocation-failure control, bounded FIFO, overflow, wrap, deep ownership, continuation and generation cancellation");
}
