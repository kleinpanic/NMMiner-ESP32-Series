#pragma once
#include <stdint.h>

enum class SdTransferStep { Write, Wait, Abort };

// Low memory can be temporary while another API request or TCP ACK completes.
// Keep the retained file positioned at its next unsent byte while waiting.
inline SdTransferStep sdTransferStep(bool socketOpen, uint32_t elapsedMs,
                                     uint32_t freeBytes, uint32_t largestBlock)
{
    if (!socketOpen || elapsedMs >= 180000) return SdTransferStep::Abort;
    // Keep aggregate headroom for concurrent requests. Requiring 4 KB in one
    // block can permanently stall on the native 4084-byte fragmentation
    // plateau even with >21 KB free. A single send is bounded to 512 bytes;
    // retain the 2 KB contiguous floor used for small request allocations.
    if (freeBytes < 16384 || largestBlock < 2048) return SdTransferStep::Wait;
    return SdTransferStep::Write;
}
