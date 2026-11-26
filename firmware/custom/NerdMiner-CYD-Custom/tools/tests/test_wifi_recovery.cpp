#include "wifiRecovery.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    WifiRecovery recovery;
    assert(!recovery.due(1000, true));
    assert(!recovery.due(2000, false));
    for (uint32_t t=7000; t<32000; t+=5000) assert(!recovery.due(t, false));
    assert(recovery.due(32000, false));
    assert(!recovery.due(32000, false));
    assert(!recovery.due(61999, false));
    assert(recovery.due(62000, false));
    assert(!recovery.due(62001, true));
    assert(!recovery.due(120000, true));
    assert(!recovery.due(180000, false));
    assert(!recovery.due(209999, false));
    assert(recovery.due(210000, false));
    assert(!recovery.due(UINT32_MAX-20000, true));
    assert(!recovery.due(UINT32_MAX-10000, false));
    assert(!recovery.due(19998, false));
    assert(recovery.due(19999, false));
    std::cout << "PASS Wi-Fi recovery: grace period, no five-second interruptions, retries, recovery and timer wrap\n";
}
