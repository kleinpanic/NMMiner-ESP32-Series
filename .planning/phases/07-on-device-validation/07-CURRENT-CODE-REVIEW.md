---
phase: 07-on-device-validation
reviewed: 2026-06-18T23:10:58Z
depth: deep
files_reviewed: 28
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
  - firmware/custom/NerdMiner-CYD-Custom/include/webapi.h
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/devices/device.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/devices/esp322432s028r.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/storage.h
  - firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
findings:
  critical: 2
  warning: 2
  info: 0
  total: 4
status: issues_found_then_fixed_for_blockers
superseded_by: "Later fallback-pool/source-blocker/review-05/webfix2 app flashes; current live app SHA-256 f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5"
resolution:
  reviewed_build: "ESP32-2432S028R SUCCESS, firmware.bin 2826704 bytes"
  flashed: "2026-06-18T23:20Z app-only reflash after fresh full backup"
  verify_flash: "0x10000 app digest matched; 0x310000 decoded LittleFS digest still matched"
  remaining_warning: "WR-02 confirmed in portal-mode probe; product behavior decision remains open"
---

# Phase 7: Code Review Report

**Supersession:** This review is historical despite the filename. Its reviewed 2,826,704-byte artifact and app-only reflash were superseded by later fallback-pool/source-blocker/review-05/webfix2 app flashes. Current live authority is `07-VALIDATION.md`, `07-VERIFICATION.md`, and `07-CURRENT-FAILURE-STATE.md`; the live app is 2,840,400 bytes with SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

**Reviewed:** 2026-06-18T23:10:58Z
**Depth:** deep
**Files Reviewed:** 28
**Status:** issues_found_then_fixed_for_blockers

## Summary

Reviewed the current Phase 7 custom ESP32-2432S028R ST7789 firmware source against the validation state, requirements, prior review, and forensics context. The known `TFT_eTouch::init()` formatter blocker is fixed in the active touch/display path, and the platform target is narrowed to the ST7789 CYD environment. However, two remaining source behaviors can still block Phase 7 validation: LittleFS can still be formatted from unrelated runtime paths, and the legacy SD owner still initializes globally before the custom `SDStore`.

Context files read but not counted as source files: `AGENTS.md`, `.planning/STATE.md`, `.planning/REQUIREMENTS.md`, Phase 7 validation/verification/review artifacts, and `.planning/forensics/report-20260618-230345.md`.

**Resolution update (2026-06-18T23:22Z):** CR-01, CR-02, and WR-01 are fixed in source, the `ESP32-2432S028R` build passes, and the latest app image was flashed after a fresh full-flash backup. Independent verification passed for the app at `0x10000`; the decoded LittleFS image at `0x310000` still digest-matched after the app reflash. WR-02 remains an intentional product-behavior decision: normal-mode mutating API routes are locked, and Phase 7 must either test them only through a reachable config/admin window or document them as intentionally disabled.

**Probe update (2026-06-18T23:31Z):** WR-02 is confirmed for recovery portal mode. A safe GET-only probe against the current AP returned 200 for `/` and `/info`, but 404 for `/probe`, `/alive`, `/api/status`, `/api/system/info`, settings, SD, and screensaver API paths. Evidence: `/tmp/p7-current-ap-get-probe-20260618-233135.tsv`.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: Runtime LittleFS mount fallback can still erase decoded assets

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp:272`; `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:730`
**Issue:** Phase 7 fixed the `TFT_eTouch::init()` path that formatted SPIFFS, but these runtime paths still call `LittleFS.begin(true)` after a failed `LittleFS.begin(false)`. In ESP32 Arduino, the `true` argument formats on mount failure. This is the same decoded asset partition used for the recovered `.ss` carousel assets, so a transient mount failure, partition mismatch, or corruption while entering the screensaver or starting a screensaver upload can wipe the decoded LittleFS content that Phase 7 is explicitly trying to preserve.
**Fix:**

```cpp
if (!LittleFS.begin(false)) {
  Serial.println("[SS] LittleFS mount failed; preserving asset partition");
  s_black_mode = true;
  nm_display_set_brightness(0);
  return 0;
}
```

For upload handling, fail the upload instead of formatting:

```cpp
if (!LittleFS.begin(false)) {
  s_ss->bad = true;
  return;
}
```

Any format operation should be moved behind an explicit maintenance-only flow with physical/admin intent, not a normal runtime fallback.

**Resolution:** Fixed. `nm_screensaver.cpp` and `webapi.cpp` now call `LittleFS.begin(false)` only. On mount failure they log and fail closed, preserving the decoded asset partition instead of formatting it.

### CR-02: Legacy `SDCard` still auto-mounts SD before the custom SD owner

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:69`; `firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.h:23`; `firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.cpp:20`; `firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.cpp:204`; `firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp:20`
**Issue:** The CYD device header defines `SDSPI_CS`, so `SDCard.h` enables `BUILD_SDSPI`. That makes the global `SDCard SDCrd` in `NerdMinerV2.ino.cpp` construct before `setup()` and call `initSDcard()`, which starts the global Arduino `SD` singleton. Later, `setup()` mounts `SDStore`, which starts a separate `SPIClass vspi(VSPI)` and calls `SD.begin()` on the same singleton again. The previous review removed live `wManager` calls into `SDCrd`, but this global constructor side effect remains and can leave SD/API validation dependent on initialization order rather than the intended single owner.
**Fix:**

```cpp
#if !defined(ESP32_2432S028R) && !defined(ESP32_2432S028_2USB)
SDCard SDCrd;
#endif
```

Alternatively, add a target build flag such as `NM_DISABLE_LEGACY_SDCARD` and guard both the global instance and the legacy SD implementation. For the Phase 7 CYD target, `SDStore::mount()` should be the only code path that owns `SD.begin()`/`SD.end()`.

**Resolution:** Fixed for the CYD target. `NerdMinerV2.ino.cpp` defines `NM_USE_LEGACY_SDCARD` only outside `ESP32_2432S028R` / `ESP32_2432S028_2USB`, guards the global `SDCard SDCrd`, and guards the legacy `SDCrd.initSDcard()` call. The current boot log shows the custom `SDStore` mount path, not the legacy global constructor path.

## Warnings

### WR-01: Compiled legacy SD config loader can still print secrets to serial

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.cpp:106`
**Issue:** `SDCard::loadConfigFile()` serializes the full parsed config document to `Serial` before copying Wi-Fi, pool, and wallet fields into settings. Current `wManager` source no longer calls this path, but it is still compiled for the CYD target because `BUILD_SDSPI` is active. Any future or accidental call would leak sensitive configuration into serial logs, violating the project rule not to commit or preserve secrets and serial-log sensitive values.
**Fix:** Prefer removing this legacy SD path from the CYD build as part of CR-02. If it must remain compiled, delete the raw `serializeJsonPretty(json, Serial)` call or print only a redacted copy:

```cpp
StaticJsonDocument<512> redacted;
redacted.set(json);
redacted[JSON_KEY_PASW] = "[redacted]";
redacted[JSON_KEY_POOLPASS] = "[redacted]";
redacted[JSON_KEY_WALLETID] = "[redacted]";
serializeJsonPretty(redacted, Serial);
```

**Resolution:** Fixed. The raw parsed JSON dump and wallet print in `SDCard.cpp` were replaced with redacted status lines.

### WR-02: API mutation gate has no reachable HTTP mutation window

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:107`; `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:150`; `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:175`; `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:81`; `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:459`; `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:756`
**Issue:** `suspendWebApiForPortal()` enables config-mode mutations and then immediately stops the API server. `resumeWebApiAfterPortal()` clears config mode before restarting the server. Protected POST routes such as settings, restart, speaker actions, swarm find, SD delete, and screensaver upload therefore have no normal HTTP window where `requireMutationUnlock()` can succeed. This is safe from an exposure perspective, but it makes the stock-parity API mutation surface effectively untestable during normal Phase 7 LAN/API validation.
**Fix:** Decide and encode the intended product behavior. If API mutations are not supported in normal mode, document them as intentionally disabled and remove them from Phase 7 API validation. If they are required, create an explicit reachable config/admin window, such as keeping the API running during a short physical-button-enabled config mode or binding a separate config-mode server, while keeping the physical/admin intent requirement.

**Resolution:** Open warning, confirmed in recovery portal mode. Serial shows the config-mode mutation gate being enabled while the WiFiManager portal starts, but the custom JSON API routes return 404 on port 80 during that state. The intended reachable HTTP mutation/API window is still a product decision. Do not count protected POST/API mutation validation complete until this behavior is deliberately specified and tested.

---

_Reviewed: 2026-06-18T23:10:58Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_
