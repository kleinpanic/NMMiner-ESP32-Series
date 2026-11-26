#pragma once
#include <stdint.h>

enum class SdTransferStep { Write, Wait, Abort };

// Low memory can be temporary while another API request or TCP ACK completes.
// Keep the retained file positioned at its next unsent byte while waiting.
inline SdTransferStep sdTransferStep(bool socketOpen, uint32_t elapsedMs,
                                     uint32_t freeBytes, uint32_t largestBlock)
{
    if (!socketOpen || elapsedMs >= 180000) return SdTransferStep::Abort;
    // A 512-byte write can retain a full 1436-byte TCP segment in this SDK.
    // A nonreading peer must leave room for another connection's RX buffer,
    // JSON/header copies, TCP output and concurrent authenticated feeds.
    // Native pressure exhausted the old 16 KB / 2 KB guard (628-byte largest
    // block). Stop bulk traffic earlier; 4000 also permits the observed
    // 4084-byte fragmentation plateau which a 4096-byte cutoff rejected.
    if (freeBytes < 24576 || largestBlock < 4000) return SdTransferStep::Wait;
    return SdTransferStep::Write;
}
