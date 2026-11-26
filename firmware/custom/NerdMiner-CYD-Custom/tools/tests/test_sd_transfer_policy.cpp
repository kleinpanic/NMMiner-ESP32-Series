#include <assert.h>
#include <stdio.h>
#include "sdTransferPolicy.h"

int main() {
    assert(sdTransferStep(true, 0, 8192, 2048) == SdTransferStep::Write);
    assert(sdTransferStep(true, 179999, 8192, 2048) == SdTransferStep::Write);
    assert(sdTransferStep(false, 0, 32768, 16384) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 180000, 32768, 16384) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 180000, 0, 0) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 100, 8191, 4096) == SdTransferStep::Wait);
    assert(sdTransferStep(true, 100, 20000, 2047) == SdTransferStep::Wait);
    // Native failure: temporary largest block 1908 must not truncate a live
    // download; it can resume after the other request releases its memory.
    assert(sdTransferStep(true, 200, 19208, 1908) == SdTransferStep::Wait);
    assert(sdTransferStep(true, 300, 21876, 4340) == SdTransferStep::Write);
    puts("PASS SD transfer policy: memory wait/resume, exact bounds, deadline and closed socket");
}
