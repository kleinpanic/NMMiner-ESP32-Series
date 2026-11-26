#pragma once
#include <stddef.h>

enum class ResponseWriteState { Progress, Wait, Complete, Failed };

// Buffer supplies length(), c_str(), concat(data, size) and null assignment
// that releases its storage (Arduino String's documented null assignment).
// Build the complete small response before sending; send at most one bounded
// slice per pass and preserve its cursor across short writes/backpressure.
template<class Buffer>
class PendingResponse {
    Buffer bytes_;
    size_t offset_ = 0;
public:
    static constexpr size_t maxBytes = 8192;
    bool append(const char* data, size_t size) {
        if (size > maxBytes - bytes_.length()) return false;
        return !size || bytes_.concat(data, size);
    }
    void reset() { bytes_ = static_cast<const char*>(nullptr); offset_ = 0; }
    size_t size() const { return bytes_.length(); }
    size_t sent() const { return offset_; }
    template<class Write>
    ResponseWriteState advance(Write write) {
        const size_t left = size() - offset_;
        if (!left) return ResponseWriteState::Complete;
        const size_t count = left < 512 ? left : 512;
        const int result = write(bytes_.c_str() + offset_, count);
        if (result < 0 || size_t(result) > count) return ResponseWriteState::Failed;
        if (!result) return ResponseWriteState::Wait;
        offset_ += size_t(result);
        return offset_ == size() ? ResponseWriteState::Complete : ResponseWriteState::Progress;
    }
};
