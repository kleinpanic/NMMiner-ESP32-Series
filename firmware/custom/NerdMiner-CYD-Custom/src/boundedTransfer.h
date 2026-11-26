#pragma once
#include <stddef.h>
#include <stdint.h>

// Read storage in blocks, outside socket writes. SDK write(Stream&) instead
// reads through Stream byte-by-byte and can spin forever on a failed read.
// A failed/short socket write must abort; retrying that block duplicates bytes.
template<class Read, class Write, class Ready, class Yield>
size_t transferBlocks(size_t length, Read read, Write write, Ready ready, Yield yield)
{
    uint8_t buffer[512];
    size_t sent = 0;
    while (sent < length && ready()) {
        const size_t wanted = length - sent < sizeof(buffer) ? length - sent : sizeof(buffer);
        const size_t got = read(buffer, wanted);
        if (!got || got > wanted) break;
        const size_t written = write(buffer, got);
        if (written > got) break;
        sent += written;
        if (written != got) break;
        yield();
    }
    return sent;
}
