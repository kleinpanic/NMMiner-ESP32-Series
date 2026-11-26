#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <src/misc/lv_tlsf.h>

// A separate instance of LVGL's existing TLSF allocator, never the UI pool.
// Only ChainFeed allocates/frees here; foreign allocations stay with the SDK.
class TlsScratchBuffer {
    lv_tlsf_t pool_ = nullptr;
    uintptr_t begin_ = 0, end_ = 0;
    size_t used_ = 0, peak_ = 0;
public:
    bool init(void* memory, size_t bytes) {
        uintptr_t begin = reinterpret_cast<uintptr_t>(memory);
        size_t overhead = lv_tlsf_size() + lv_tlsf_pool_overhead();
        if (pool_ || !memory || begin % lv_tlsf_align_size() ||
            bytes < overhead + lv_tlsf_block_size_min() ||
            bytes - overhead > lv_tlsf_block_size_max() || bytes > UINTPTR_MAX - begin) return false;
        pool_ = lv_tlsf_create_with_pool(memory, bytes);
        if (!pool_) return false;
        begin_ = begin; end_ = begin + bytes;
        return true;
    }
    void* calloc(size_t count, size_t size) {
        if (!pool_ || !count || !size || count > SIZE_MAX / size) return nullptr;
        size_t bytes = count * size;
        void* ptr = lv_tlsf_malloc(pool_, bytes);
        if (ptr) {
            memset(ptr, 0, bytes);
            used_ += lv_tlsf_block_size(ptr);
            if (used_ > peak_) peak_ = used_;
        }
        return ptr;
    }
    bool owns(const void* ptr) const {
        uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
        return pool_ && address >= begin_ && address < end_;
    }
    bool release(void* ptr) {
        if (!owns(ptr)) return false;
        used_ -= lv_tlsf_block_size(ptr);
        lv_tlsf_free(pool_, ptr);
        return true;
    }
    size_t used() const { return used_; }
    size_t peak() const { return peak_; }
};
