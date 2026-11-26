#include "poolFailover.h"
#include <cassert>
#include <cstdio>

int main() {
    PoolFailover state;
    assert(!state.usingFallback());
    for (unsigned i = 0; i < 100; ++i) assert(!state.failed(false));
    assert(!state.usingFallback());
    assert(!state.failed(true));
    assert(!state.failed(true));
    state.healthy();
    assert(!state.failed(true));
    assert(!state.failed(true));
    assert(state.failed(true));
    assert(state.usingFallback());
    state.healthy();
    assert(!state.failed(true));
    assert(!state.failed(true));
    assert(state.failed(true));
    assert(!state.usingFallback());
    std::puts("pool failover: disabled, threshold, recovery reset and both directions PASS");
}
