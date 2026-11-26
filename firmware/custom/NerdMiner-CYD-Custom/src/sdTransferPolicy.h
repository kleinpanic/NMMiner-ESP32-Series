#pragma once
#include <stdint.h>

enum class SdTransferStep { Write, Wait, Abort };

// Low memory can be temporary while another API request or TCP ACK completes.
// Keep the retained file positioned at its next unsent byte while waiting.
inline SdTransferStep sdTransferStep(bool socketOpen, uint32_t elapsedMs,
                                     uint32_t freeBytes, uint32_t largestBlock,
                                     uint32_t idleMs = 0)
{
    if (!socketOpen || elapsedMs >= 180000 || idleMs >= 15000) return SdTransferStep::Abort;
    // A 512-byte write can retain a full 1436-byte TCP segment in this SDK.
    // A nonreading peer must leave room for another connection's RX buffer,
    // JSON/header copies, TCP output and concurrent authenticated feeds.
    // Native pressure exhausted both the 2 KB (628 remaining) and 4 KB
    // (2036 remaining) guards. Leave room for RX, JSON and a second TCP send,
    // rather than just the next bulk packet. Stdio is separately bounded to
    // avoid retaining the SDK's additional 4 KB file buffer.
    // File stdio is separately bounded to 512 bytes before this check.
    if (freeBytes < 20480 || largestBlock < 6144) return SdTransferStep::Wait;
    return SdTransferStep::Write;
}
