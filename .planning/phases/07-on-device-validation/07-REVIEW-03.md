---
phase: 07-on-device-validation
reviewed: 2026-06-18T19:24:40Z
depth: deep
files_reviewed: 2
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
status: issues_found
---

# Phase 7: Code Review Report

**Reviewed:** 2026-06-18T19:24:40Z
**Depth:** deep
**Files Reviewed:** 2
**Status:** issues_found

## Summary

Re-reviewed the latest working tree for only the requested follow-up scope:
`wManager.cpp` and `sdstore.cpp`. No firmware source files were edited. No flash,
upload, read_flash, write_flash, or backup commands were run. `git diff --check`
passed for the scoped firmware files.

The three original 07-REVIEW-02 focus items are fixed:

- WiFiManagerParameter lifetime is no longer stack-bound. Parameters now have static
  pointer storage and are allocated once by `ensurePortalParameters()` before both
  boot-time and manual portals use them.
- Portal save paths now copy submitted WiFiManager SSID/password into `Settings`
  before `nvMem.saveConfig()` via `persistPortalSettingsFromParameters(true)`.
- SD API `/sd` path double-normalization is fixed in `SDStore::sanitizePath()`.

One edge in the additional SD import patch still fails the stated contract: imported
SD Wi-Fi credentials are assigned into `Settings` before the `hasUsableWifiConfig()`
check passes.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: SD Wi-Fi credentials still mutate Settings before usable validation passes

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:364`
**Issue:** `tryConfigFromSD()` now holds imported `SSID` and `WifiPW` in local
`String`s and correctly returns before assigning them when `WifiPW` is missing
at lines 349-363. However, it then assigns both values into the live `Settings`
object at lines 364-365 and only afterwards calls `hasUsableWifiConfig(settings)`
at line 366. If an SD config contains both keys but the pair is rejected by
`hasUsableWifiConfig()`, the function returns `false` after mutating
`Settings.WifiSSID` and `Settings.WifiPW`. That does not meet the intended
"assign only after hasUsableWifiConfig() passes" contract and leaves a residual
stale-credential edge in the SD fallback path.
**Fix:** Validate a candidate copy, or validate the local values through a helper,
then assign into `Settings` only after the check passes.

```cpp
TSettings candidate = *settings;
candidate.WifiSSID = importedSsid;
candidate.WifiPW = importedWifiPw;
if (!hasUsableWifiConfig(&candidate)) {
    Serial.println(F("[CFG] SD config imported app settings but has no usable WiFi credentials"));
    return false;
}
settings->WifiSSID = importedSsid;
settings->WifiPW = importedWifiPw;
```

## Resolved Checks

- **Fixed:** WiFiManagerParameter lifetime for manual config portal after
  `init_WifiManager()` returns. Static parameter pointers are declared at
  `wManager.cpp:46`, allocated in `ensurePortalParameters()` at
  `wManager.cpp:480`, registered at `wManager.cpp:512`, and initialized for both
  boot-time and manual portal paths at `wManager.cpp:617` and `wManager.cpp:254`.
- **Fixed:** Portal saves sync submitted WiFiManager SSID/password into
  `Settings` before NVS persistence. `syncWiFiManagerCredentials()` reads
  WiFiManager credentials at `wManager.cpp:426`, `persistPortalSettingsFromParameters(true)`
  saves through that path at `wManager.cpp:472`, and the force/fallback/manual
  save paths call it at `wManager.cpp:630`, `wManager.cpp:670`, and
  `wManager.cpp:762`.
- **Fixed:** SD API `/sd` path double-normalization. `sanitizePath()` now returns
  `/sd` unchanged for the root at `sdstore.cpp:433`, strips an existing `sd/`
  prefix at `sdstore.cpp:436`, and only then prefixes the remaining relative path
  at `sdstore.cpp:441`.

---

_Reviewed: 2026-06-18T19:24:40Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_

## Follow-up Resolution - 2026-06-18T19:27:50Z

The WR-01 warning above is closed in the current working tree. `tryConfigFromSD()`
now validates key presence at `wManager.cpp:368`, then validates
`hasUsableWifiCredentials(importedSsid, importedWifiPw)` at `wManager.cpp:372`
before assigning imported SD Wi-Fi fields into `settings->WifiSSID` and
`settings->WifiPW` at `wManager.cpp:376-377`. The exact stale SD Wi-Fi
assignment edge from 07-REVIEW-03 is resolved.
