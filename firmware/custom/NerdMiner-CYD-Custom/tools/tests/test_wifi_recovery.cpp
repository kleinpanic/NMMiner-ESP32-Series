#include "wifiRecovery.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    for (bool setup : {false, true}) {
        for (bool hasSsid : {false, true}) {
            for (bool connected : {false, true}) {
                const auto action = wifiStartupAction(setup, hasSsid, connected);
                if (setup || !hasSsid) assert(action == WifiStartupAction::Setup);
                else if (connected) assert(action == WifiStartupAction::Run);
                else assert(action == WifiStartupAction::Join);
            }
        }
    }
    // A failed initial association follows the same retry schedule as a drop;
    // it must not require an earlier successful connection to arm recovery.
    WifiRecovery failedBoot;
    assert(wifiStartupAction(false, true, false) == WifiStartupAction::Join);
    assert(!failedBoot.due(80000, false));
    assert(!failedBoot.due(109999, false));
    assert(failedBoot.due(110000, false));
    assert(!failedBoot.due(110001, true));
    assert(wifiStartupAction(false, true, true) == WifiStartupAction::Run);
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
    std::cout << "PASS Wi-Fi recovery: saved-network boot, explicit/missing-config Setup, grace period, retries and timer wrap\n";
}
