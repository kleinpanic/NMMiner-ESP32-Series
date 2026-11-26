#pragma once
#include <stdint.h>

enum class SdTransferStep { Write, Wait, Abort };

// Low memory can be temporary while another API request or TCP ACK completes.
// Keep the retained file positioned at its next unsent byte while waiting.
inline SdTransferStep sdTransferStep(bool socketOpen, uint32_t elapsedMs,
                                     uint32_t freeBytes, uint32_t largestBlock)
{
    if (!socketOpen || elapsedMs >= 180000) return SdTransferStep::Abort;
    // Reserve room for Stratum parsing/results and concurrent status replies.
    if (freeBytes < 16384 || largestBlock < 4096) return SdTransferStep::Wait;
    return SdTransferStep::Write;
}
