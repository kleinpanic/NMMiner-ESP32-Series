#pragma once
#include <stdint.h>
#include <string.h>
inline bool decodeCompactTarget(uint32_t compact, uint8_t target[32]) {
    memset(target, 0, 32);
    unsigned size = compact >> 24;
    uint32_t word = compact & 0x007fffff;
    if (!word || (compact & 0x00800000)) return false;
    if (size <= 3) {
        word >>= 8 * (3 - size);
        for (unsigned i = 0; i < 3; i++) target[i] = uint8_t(word >> (8 * i));
        return word != 0;
    }
    unsigned start = size - 3;
    for (unsigned i = 0; i < 3; i++) {
        uint8_t byte = uint8_t(word >> (8 * i));
        if (start + i >= 32) { if (byte) return false; }
        else target[start + i] = byte;
    }
    return true;
}
// Both SHA256d and the decoded compact network target are little-endian.
inline bool hashMeetsTarget(const uint8_t hash[32], const uint8_t target[32]) {
    for (int i = 31; i >= 0; i--) {
        if (hash[i] < target[i]) return true;
        if (hash[i] > target[i]) return false;
    }
    return true;
}
