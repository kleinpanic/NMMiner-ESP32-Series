#pragma once
#include <stddef.h>
#include <stdint.h>

struct EgressEndpoint {
    uint32_t address;
    uint16_t port;
    uint8_t protocol;
};

// Ethernet/IPv4 headers only. Non-first fragments have no transport header.
inline bool egressEndpoint(const uint8_t* frame, size_t size, EgressEndpoint& out) {
    if (size < 34 || frame[12] != 0x08 || frame[13] != 0x00 || (frame[14] >> 4) != 4) return false;
    const size_t header = (frame[14] & 15) * 4;
    if (header < 20 || size < 14 + header) return false;
    out.address = (uint32_t(frame[30]) << 24) | (uint32_t(frame[31]) << 16) |
                  (uint32_t(frame[32]) << 8) | frame[33];
    out.protocol = frame[23];
    out.port = 0;
    const bool first = !(frame[20] & 0x1f) && frame[21] == 0;
    if (first && (out.protocol == 6 || out.protocol == 17)) {
        if (size < 14 + header + 4) return false;
        out.port = (uint16_t(frame[14 + header + 2]) << 8) | frame[14 + header + 3];
    }
    return true;
}

void egressAuditStart();
void egressAuditDump();
