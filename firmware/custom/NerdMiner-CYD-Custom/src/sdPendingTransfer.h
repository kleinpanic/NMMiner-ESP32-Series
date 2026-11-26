#pragma once
#include <stddef.h>
#include <stdint.h>

enum class SdWriteState { Progress, Wait, Failed };
struct SdWriteResult { size_t sent; SdWriteState state; };

// One read and one nonblocking send per pass. Retain partially sent data and
// retry it unchanged after backpressure; never reread or skip those bytes.
class SdPendingTransfer {
    uint8_t data_[512];
    size_t length_ = 0, offset_ = 0;
public:
    void reset() { length_ = offset_ = 0; }
    template<class Read, class Write>
    SdWriteResult advance(size_t remaining, Read read, Write write) {
        if (!remaining) return {0, SdWriteState::Progress};
        if (offset_ == length_) {
            const size_t count = remaining < sizeof(data_) ? remaining : sizeof(data_);
            const int received = read(data_, count);
            if (received < 0 || size_t(received) != count) return {0, SdWriteState::Failed};
            length_ = count;
            offset_ = 0;
        }
        const int sent = write(data_ + offset_, length_ - offset_);
        if (sent < 0 || size_t(sent) > length_ - offset_) return {0, SdWriteState::Failed};
        if (!sent) return {0, SdWriteState::Wait};
        offset_ += size_t(sent);
        return {size_t(sent), SdWriteState::Progress};
    }
};
