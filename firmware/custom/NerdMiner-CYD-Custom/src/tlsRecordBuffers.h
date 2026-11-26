#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Two long-lived TLS record buffers. Only the feed task acquires/releases
// these slots; other mbedTLS allocations keep Espressif's normal allocator.
class TlsRecordBuffers {
    struct Slot { void* data = nullptr; size_t size = 0; bool used = false; } slots_[2];
public:
    bool init(void* first, size_t firstSize, void* second, size_t secondSize) {
        if (!first || !second || first == second || !firstSize || !secondSize) return false;
        slots_[0].data = first; slots_[0].size = firstSize; slots_[0].used = false;
        slots_[1].data = second; slots_[1].size = secondSize; slots_[1].used = false;
        return true;
    }
    void* acquire(size_t count, size_t size) {
        if (!count || !size || count > SIZE_MAX / size) return nullptr;
        size_t bytes = count * size;
        // Prefer an exact slot so a smaller output buffer cannot consume
        // the input slot in SDK builds with asymmetric record sizes.
        for (int pass = 0; pass < 2; ++pass) for (auto& slot : slots_) {
            if (slot.data && !slot.used && (pass ? slot.size > bytes : slot.size == bytes)) {
                slot.used = true;
                memset(slot.data, 0, bytes);
                return slot.data;
            }
        }
        return nullptr;
    }
    bool release(void* ptr) {
        if (!ptr) return false;
        for (auto& slot : slots_) if (ptr == slot.data) {
            slot.used = false;
            return true;
        }
        return false;
    }
};
