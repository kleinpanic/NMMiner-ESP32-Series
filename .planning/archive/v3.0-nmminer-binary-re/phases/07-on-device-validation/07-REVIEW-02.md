---
phase: 07-on-device-validation
reviewed: 2026-06-18T19:14:17Z
depth: deep
files_reviewed: 13
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
findings:
  critical: 2
  warning: 1
  info: 0
  total: 3
status: issues_found
---

# Phase 7: Code Review Report

**Reviewed:** 2026-06-18T19:14:17Z
**Depth:** deep
**Files Reviewed:** 13
**Status:** issues_found

## Summary

Re-reviewed only the requested Phase 7 07-02 gap-closure files. No flash, upload,
read_flash, write_flash, backup, or firmware-edit commands were run. `git diff --check`
passed for the scoped firmware files.

The six prior findings are directly addressed in the latest patch: CYD blocking portal
loops now pump `wm.process()` and `nm_uimanager_tick()`; runtime display apply invalidates
and refreshes LVGL after rotation/brightness; the new SD fallback requires `WifiPW` before
returning usable WiFi config; config entry wakes the display and stops an active screensaver;
the config countdown is tied to the 180s timeout constant; and brightness 0 is normalized to
the default instead of being applied as black.

Two blocking config-path defects remain, plus one SD API path bug.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: Manual config portal reuses dead WiFiManagerParameter pointers

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:452`
**Issue:** `init_WifiManager()` creates all custom `WiFiManagerParameter` objects as stack locals at lines 452-505 and registers their addresses with the global `wm` at lines 484-505. After `init_WifiManager()` returns, the long-press/manual path can call `enter_config_portal()` at lines 229-238, causing WiFiManager to render/save using dangling pointers. That can corrupt the portal page, lose parameter values, or crash during the documented recovery path.
**Fix:** Give registered parameters storage that outlives every portal session, or rebuild/own them in a portal context before each `startConfigPortal()` and clear them afterward. Also route manual portal save completion through the same NVS/SD save code used by the boot-time portal.

### CR-02: Portal saves persist stale WiFi credentials instead of submitted credentials

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:532`
**Issue:** When a portal save occurs, the force-config save block writes `Settings` to NVS at line 532, and the fallback save block does the same at line 586, but neither block copies the SSID/password submitted to WiFiManager into `Settings.WifiSSID`/`Settings.WifiPW` first. This can save default, stale, or SD-mirrored SSID values into the app config. The SD fallback direct promotion is fixed, but `tryConfigFromSD()` still assigns a mirrored `SSID` before returning false for missing `WifiPW` at lines 329-342; a later portal save can persist that stale SSID because the real WiFiManager credentials are never synchronized.
**Fix:**
```cpp
Settings.WifiSSID = wm.getWiFiSSID(true);
Settings.WifiPW = wm.getWiFiPass(true);
if (!hasUsableWifiConfig(&Settings)) {
    // keep portal active or fail the save instead of persisting stale credentials
    return;
}
nvMem.saveConfig(&Settings);
writeConfigToSD(&Settings);
```
Apply this before every portal-driven `nvMem.saveConfig(&Settings)` path, and avoid assigning SD `SSID` into the live WiFi fields unless the SD JSON also has usable WiFi credentials.

## Warnings

### WR-01: SD API paths are prefixed with `/sd` twice

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:162`
**Issue:** `apiSafePath()` returns the transformed result from `SDStore::sanitizePath()`, then `handleSdList()`, `handleSdDownload()`, and `handleSdDelete()` pass that already-prefixed path into `SDStore` methods that sanitize again. `SDStore::sanitizePath()` strips leading slashes and unconditionally prefixes `/sd/` at lines 412-434, so `/sd` becomes `/sd/sd`, and a download path such as `/sd/mining_stats.csv` becomes `/sd/sd/...` or `/sd/sd/sd/...`. `handleSdLog()` also passes `SD_CSV_PATH` directly to `readFile()` at line 210, which sanitizes it to `/sd/sd/mining_stats.csv`, so the log endpoint cannot read the file written by `appendCSVLog()`.
**Fix:** Make path normalization single-owner. Either have API handlers pass raw relative paths to `SDStore`, or make `SDStore::sanitizePath()` recognize already-rooted `/sd` paths without adding another prefix.

---

_Reviewed: 2026-06-18T19:14:17Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_
