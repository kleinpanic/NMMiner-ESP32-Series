#include "egressAudit.h"
#include <cassert>
#include <cstdio>

int main() {
    uint8_t packet[78]{};
    EgressEndpoint e{};
    for (size_t n = 0; n < 34; ++n) assert(!egressEndpoint(packet, n, e));
    packet[12] = 8; packet[14] = 0x45; packet[23] = 6;
    packet[30] = 1; packet[31] = 2; packet[32] = 3; packet[33] = 4;
    packet[36] = 1; packet[37] = 0xbb;
    assert(egressEndpoint(packet, 38, e) && e.address == 0x01020304 && e.port == 443 && e.protocol == 6);
    assert(!egressEndpoint(packet, 37, e));
    packet[23] = 17; packet[36] = 0; packet[37] = 53;
    assert(egressEndpoint(packet, 38, e) && e.port == 53 && e.protocol == 17);
    packet[21] = 1; // non-first IPv4 fragment: these bytes are not ports
    assert(egressEndpoint(packet, 34, e) && e.port == 0);
    packet[21] = 0; packet[14] = 0x46;
    packet[40] = 0; packet[41] = 123;
    assert(egressEndpoint(packet, 42, e) && e.port == 123);
    packet[14] = 0x44; assert(!egressEndpoint(packet, sizeof(packet), e));
    packet[14] = 0x65; assert(!egressEndpoint(packet, sizeof(packet), e));
    packet[14] = 0x45; packet[23] = 1;
    assert(egressEndpoint(packet, 34, e) && e.port == 0 && e.protocol == 1);
    packet[12] = 0x86; packet[13] = 0xdd;
    assert(!egressEndpoint(packet, sizeof(packet), e));
    std::puts("egress headers: truncated, TCP/UDP, options, fragments, ICMP and non-IPv4 PASS");
}
