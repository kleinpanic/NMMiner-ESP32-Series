#include "boundedTransfer.h"
#include <cassert>
#include <cstdio>
#include <vector>

int main() {
    for (size_t length : {size_t(0), size_t(1), size_t(512), size_t(1360), size_t(5976)}) {
        std::vector<uint8_t> source(length), received;
        for (size_t i = 0; i < length; ++i) source[i] = uint8_t(i);
        size_t offset = 0, yields = 0;
        size_t sent = transferBlocks(length, [&](uint8_t* out, size_t n) {
            assert(n <= 512);
            for (size_t i = 0; i < n; ++i) out[i] = source[offset++];
            return n;
        }, [&](const uint8_t* in, size_t n) {
            received.insert(received.end(), in, in + n); return n;
        }, [] { return true; }, [&] { ++yields; });
        assert(sent == length && received == source && yields == (length + 511) / 512);
    }
    for (int fault = 0; fault < 5; ++fault) {
        size_t reads = 0, writes = 0, yields = 0;
        const size_t sent = transferBlocks(2048, [&](uint8_t*, size_t n) {
            ++reads;
            if (fault == 0) return size_t(0); // storage reports no progress
            if (fault == 1) return n + 1;    // broken storage count
            return n;
        }, [&](const uint8_t*, size_t n) {
            ++writes;
            if (fault == 2) return n - 1;    // short write: never resend
            if (fault == 3) return n + 1;    // broken socket count
            return n;
        }, [&] { return fault != 4 || reads == 0; }, [&] { ++yields; });
        assert(reads == 1);
        assert(writes == size_t(fault >= 2));
        assert(sent == (fault == 2 ? 511u : fault == 4 ? 512u : 0u));
        assert(yields == size_t(fault == 4));
    }
    puts("PASS bounded transfer: exact block bytes, zero/invalid reads, short/invalid writes and disconnect");
}
