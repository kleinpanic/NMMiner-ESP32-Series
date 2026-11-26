#include "tlsRecordBuffers.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>

int main() {
    alignas(std::max_align_t) unsigned char input[512], output[512];
    TlsRecordBuffers buffers;
    assert(!buffers.acquire(1, 512));
    assert(!buffers.init(nullptr, 512, output, 512));
    assert(!buffers.init(input, 512, input, 512));
    assert(buffers.init(input, sizeof(input), output, sizeof(output)));
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
        memset(input, 0xa5, sizeof(input)); memset(output, 0x5a, sizeof(output));
        assert(buffers.acquire(1, 512) == input);
        assert(buffers.acquire(2, 256) == output);
        for (size_t i=0; i<512; ++i) assert(!input[i] && !output[i]);
        assert(!buffers.acquire(1, 512));
        assert(buffers.release(input));
        assert(buffers.acquire(1, 512) == input);
        assert(buffers.release(output));
        assert(buffers.release(input));
    }
    assert(!buffers.acquire(SIZE_MAX, 2));
    assert(!buffers.acquire(0, 512));
    assert(!buffers.acquire(1, 513));
    int other;
    assert(!buffers.release(&other)); assert(!buffers.release(nullptr));
    TlsRecordBuffers asymmetric;
    assert(asymmetric.init(input, 512, output, 256));
    assert(asymmetric.acquire(1, 256) == output);
    assert(asymmetric.acquire(1, 512) == input);
    std::cout << "PASS TLS records: repeated zeroed reuse, concurrent slots, exhaustion, overflow and allocator ownership\n";
}
