#include <assert.h>
#include <stdio.h>
#include "sdTransferPolicy.h"

int main() {
    assert(sdTransferStep(true, 0, 20480, 4000) == SdTransferStep::Write);
    assert(sdTransferStep(true, 179999, 20480, 4000) == SdTransferStep::Write);
    assert(sdTransferStep(false, 0, 32768, 16384) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 180000, 32768, 16384) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 180000, 0, 0) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 100, 20479, 8192) == SdTransferStep::Wait);
    assert(sdTransferStep(true, 100, 32768, 3999) == SdTransferStep::Wait);
    // Native long-lived sockets left a 4084-byte largest block. Permit that
    // fragmentation shape once aggregate memory has also recovered.
    assert(sdTransferStep(true, 100, 28620, 4084) == SdTransferStep::Write);
    // Native blocked-peer trace: stop at the low-water readings before the
    // former 16 KB reserve permitted further bulk writes. Stdio is now bounded.
    assert(sdTransferStep(true, 100, 20924, 4596) == SdTransferStep::Write);
    assert(sdTransferStep(true, 100, 18756, 4596) == SdTransferStep::Wait);
    assert(sdTransferStep(true, 100, 13064, 628) == SdTransferStep::Wait);
    // Cancellation must win even when low memory would prevent another write.
    assert(sdTransferStep(false, 100, 18804, 6644) == SdTransferStep::Abort);
    // A disconnected or unreading client cannot retain a low-memory file for
    // the entire three-minute deadline. A healthy peer can still pause briefly.
    assert(sdTransferStep(true, 16000, 18804, 6644, 14999) == SdTransferStep::Wait);
    assert(sdTransferStep(true, 16000, 18804, 6644, 15000) == SdTransferStep::Abort);
    assert(sdTransferStep(true, 16000, 28620, 8192, 14999) == SdTransferStep::Write);
    // Native failure: temporary largest block 1908 must not truncate a live
    // download; it can resume after the other request releases its memory.
    assert(sdTransferStep(true, 200, 19208, 1908) == SdTransferStep::Wait);
    assert(sdTransferStep(true, 300, 28832, 6644) == SdTransferStep::Write);
    puts("PASS SD transfer policy: memory wait/resume, exact bounds, deadline and closed socket");
}
