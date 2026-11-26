#include "walletStats.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    char out[91];
    const char* address = "1BoatSLRHtKNngkdXEeobR76b53LETtpyT";
    assert(walletAddress(address, out, sizeof(out)) && !strcmp(out, address));
    assert(walletAddress("1BoatSLRHtKNngkdXEeobR76b53LETtpyT.worker", out, sizeof(out)));
    assert(!strcmp(out, address));
    assert(!walletAddress("https://example.com/wallet", out, sizeof(out)));
    assert(!walletAddress("1BoatSLRHtKNngkdXEeobR76b53LETtpyT?x", out, sizeof(out)));
    assert(!walletAddress(address, out, 8));
    assert(!walletAddress(nullptr, out, sizeof(out)));
    assert(walletAddress("bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4.worker", out, sizeof(out)));
    int64_t net;
    assert(walletNetSats(0, 0, net) && net==0);
    assert(walletNetSats(200000000, 50000000, net) && net==150000000);
    assert(walletNetSats(0, 500, net) && net==-500);
    assert(walletNetSats(2200000000000000ULL, 2199999999999500ULL, net) && net==500);
    assert(!walletNetSats(UINT64_MAX, 0, net));
    std::cout << "PASS wallet stats: worker stripping, bounded address, path rejection, zero/positive/pending balances and overflow\n";
}
