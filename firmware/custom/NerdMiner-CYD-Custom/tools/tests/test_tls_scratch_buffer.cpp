#include "tlsScratchBuffer.h"
#include <assert.h>
#include <stdio.h>
#include <vector>

int main() {
    alignas(16) unsigned char storage[24 * 1024];
    unsigned char foreign[32];
    TlsScratchBuffer pool;
    assert(!pool.calloc(1, 1));
    assert(!pool.init(nullptr, sizeof(storage)));
    assert(!pool.init(storage + 1, sizeof(storage) - 1));
    assert(!pool.init(storage, 16));
    assert(pool.init(storage, sizeof(storage)));
    assert(!pool.init(storage, sizeof(storage)));
    assert(!pool.calloc(0, 1) && !pool.calloc(1, 0));
    assert(!pool.calloc(SIZE_MAX, 2));
    assert(!pool.release(nullptr) && !pool.release(foreign));
    for (int cycle = 0; cycle < 1000; ++cycle) {
        std::vector<void*> pointers;
        for (size_t size : {128u, 256u, 512u, 1024u, 4096u, 2048u, 512u, 64u}) {
            auto p = static_cast<unsigned char*>(pool.calloc(1, size));
            assert(p && pool.owns(p));
            for (size_t i = 0; i < size; ++i) assert(p[i] == 0);
            memset(p, 0xa5, size);
            pointers.push_back(p);
        }
        for (size_t i = 0; i < pointers.size(); i += 2) assert(pool.release(pointers[i]));
        for (size_t i = 1; i < pointers.size(); i += 2) assert(pool.release(pointers[i]));
        assert(pool.used() == 0);
        assert(lv_tlsf_check_pool(lv_tlsf_get_pool(storage)) == 0);
    }
    auto large = pool.calloc(1, 16 * 1024);
    assert(large && !pool.calloc(1, 24 * 1024));
    assert(pool.release(large));
    assert(pool.used() == 0 && pool.peak() >= 16 * 1024);
    puts("TLS scratch allocator: 1000 fragmented/coalesced cycles, zeroing and boundary checks PASS");
}
