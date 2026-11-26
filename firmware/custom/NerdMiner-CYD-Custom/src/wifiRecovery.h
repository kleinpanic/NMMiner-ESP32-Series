#pragma once
#include <stdint.h>

enum class WifiStartupAction { Setup, Join, Run };

// A temporarily unavailable saved network is not a request to reconfigure it.
// Finish startup so the regular retry owner and the display can keep running.
constexpr WifiStartupAction wifiStartupAction(bool explicitSetup, bool hasSsid,
                                              bool connected) {
    return explicitSetup || !hasSsid ? WifiStartupAction::Setup :
           connected ? WifiStartupAction::Run : WifiStartupAction::Join;
}

// Give a join time to finish before restarting it. Arduino reconnect() forces
// a disconnect, so invoking it every five seconds can prevent recovery.
class WifiRecovery {
    uint32_t since_ = 0;
    bool waiting_ = false;
public:
    bool due(uint32_t now, bool connected) {
        if (connected) { waiting_ = false; return false; }
        if (!waiting_) { since_ = now; waiting_ = true; return false; }
        if (uint32_t(now - since_) < 30000) return false;
        since_ = now;
        return true;
    }
};
