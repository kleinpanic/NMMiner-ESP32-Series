#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Pool workers use ADDRESS.worker; send only the address to the explorer.
inline bool walletAddress(const char* worker, char* out, size_t size) {
    if (!worker || !out || !size) return false;
    out[0] = 0;
    size_t n = strcspn(worker, ".");
    if (n < 26 || n > 90 || n >= size) return false;
    bool bech = strncmp(worker, "bc1", 3) == 0 || strncmp(worker, "BC1", 3) == 0;
    if (!bech && worker[0] != '1' && worker[0] != '3') return false;
    const char* alphabet = bech ? "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                : "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    for (size_t i=0; i<n; ++i) if (!strchr(alphabet, worker[i])) return false;
    memcpy(out, worker, n); out[n] = 0;
    return true;
}

inline bool walletNetSats(uint64_t funded, uint64_t spent, int64_t& net) {
    constexpr uint64_t max_money = 21000000ULL * 100000000ULL;
    if (funded > INT64_MAX || spent > INT64_MAX) return false;
    net = int64_t(funded) - int64_t(spent);
    return net >= -int64_t(max_money) && net <= int64_t(max_money);
}
