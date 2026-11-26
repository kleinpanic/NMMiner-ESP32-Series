#pragma once
#include <array>
#include <stddef.h>

// Callers hold the mining mutex. Job payloads are copied into fixed storage;
// queue refills and worker ownership never allocate from the network heap.
template<class T, size_t Capacity>
class MiningJobQueue {
    static_assert(Capacity > 0, "A mining queue needs storage");
    std::array<T, Capacity> slots_{};
    size_t head_ = 0, count_ = 0;
public:
    bool empty() const { return count_ == 0; }
    size_t size() const { return count_; }
    T& front() { return slots_[head_]; }
    bool push_back(const T& value) {
        if (count_ == Capacity) return false;
        slots_[(head_ + count_) % Capacity] = value;
        ++count_;
        return true;
    }
    void pop_front() { if (count_) { head_ = (head_ + 1) % Capacity; --count_; } }
    void clear() { head_ = count_ = 0; }
};

template<class T>
class OwnedMiningJob {
    T value_{};
    bool valid_ = false;
public:
    OwnedMiningJob& operator=(const T& value) { value_ = value; valid_ = true; return *this; }
    explicit operator bool() const { return valid_; }
    T* operator->() { return &value_; }
    const T* operator->() const { return &value_; }
    void reset() { valid_ = false; }
};
