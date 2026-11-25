---
phase: 07-on-device-validation
reviewed: 2026-06-18T20:59:20Z
depth: deep
files_reviewed: 17
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/storage.h
  - firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
findings:
  critical: 5
  warning: 2
  info: 0
  total: 7
status: issues_found_then_partially_fixed
---

# Phase 7: Codex Code Review Report

## Critical Issues

### CR-01: Manual config portal conflicts with the custom API server on port 80

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:286`

**Issue:** The runtime BOOT-triggered path calls `wm.startConfigPortal()` while the custom API server is already active. `webApiSetup()` starts `apiServer` on port 80 during setup (`NerdMinerV2.ino.cpp:264`, `webapi.cpp:43`, `webapi.cpp:773`), and the main loop continues pumping it (`NerdMinerV2.ino.cpp:459`, `webapi.cpp:779`). WiFiManager also uses port 80 by default and starts its own web server for the portal. That leaves the documented long-press recovery/config path unable to reliably serve the captive portal after normal boot; the existing API server can keep owning the same listener or continue handling port-80 traffic while WiFiManager expects to own it.

**Fix:** Suspend the custom API before starting any WiFiManager portal, then restart it only after the portal is closed, or move one of the services to a different port/interface. A minimal shape is:

```cpp
// webapi.cpp / webapi.h
void webApiStop() {
  if (!apiStarted) return;
  apiServer.stop();
  apiStarted = false;
}

// wManager.cpp
void enter_config_portal() {
  webApiStop();
  refreshPortalParametersFromSettings();
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(0);
  wm.startConfigPortal(apName, configPortalPassword());
}
```

Add the matching restart path when `wm.getConfigPortalActive()` becomes false.

### CR-02: Reused WiFiManager parameters can overwrite newer API settings with stale values

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:508`

**Issue:** `ensurePortalParameters()` initializes portal parameters from `Settings` once, then returns forever when `s_portalParametersAdded` is true (`wManager.cpp:508-512`). Later API calls mutate and persist settings such as network, mining, time, and display preferences (`webapi.cpp:445-513`), but the already-allocated `WiFiManagerParameter` objects are never refreshed. If the user enters the manual portal after changing settings through the API and saves, `persistPortalSettingsFromParameters()` copies the stale parameter values back into `Settings` and NVS/SD (`wManager.cpp:500-505`, `wManager.cpp:785-786`). This can silently roll back pool, credential-adjacent, time, and display settings.

**Fix:** Split allocation from value refresh. Call the refresh before every `startConfigPortal()` and before any portal save path reads parameters.

```cpp
static void refreshPortalParametersFromSettings() {
  ensurePortalParameters();
  s_poolTextBox->setValue(Settings.PoolAddress.c_str(), 80);
  s_portTextBox->setValue(String(Settings.PoolPort).c_str(), 7);
  s_poolPasswordTextBox->setValue(Settings.PoolPassword, 80);
  s_btcAddressTextBox->setValue(Settings.BtcWallet, 80);
  s_timezoneTextBox->setValue(String(Settings.Timezone).c_str(), 3);
  setCheckboxHtml(s_saveStatsCheckboxHtml, sizeof(s_saveStatsCheckboxHtml),
                  Settings.saveStats);
#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  s_brightnessTextBox->setValue(String(Settings.Brightness).c_str(), 3);
  setCheckboxHtml(s_invertColorsCheckboxHtml, sizeof(s_invertColorsCheckboxHtml),
                  Settings.invertColors);
#endif
}
```

Avoid persisting fields from stale portal parameters unless the current portal session actually submitted them.

### CR-03: Mutating device API endpoints have no authentication or config-mode gate

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:748`

**Issue:** The custom API exposes POST routes that change network, mining, time, display, market, weather, SD, screensaver, swarm, and restart behavior without any authentication or config-mode requirement (`webapi.cpp:748-769`). The handlers directly persist sensitive device settings to NVS (`webapi.cpp:445-513`), delete SD files (`webapi.cpp:194-204`), and reboot the device (`webapi.cpp:586-589`). The comment at `webapi.cpp:48-53` relies on a LAN-trust/CORS model, but CORS is not authentication and does not stop same-LAN clients, direct HTTP tools, AP clients, or compromised LAN hosts from changing device state.

**Fix:** Require an admin credential for every mutating route, or restrict mutating routes to an explicit local config mode. Store only a hash or derived secret in NVS, compare a header or bearer token before dispatching POST handlers, and return `401`/`403` before parsing or applying request bodies.

```cpp
static bool requireApiAuth() {
  String token = apiServer.header("Authorization");
  if (!validAdminToken(token)) {
    sendError(401, "unauthorized");
    return false;
  }
  return true;
}

static void handleSettingMiningPost() {
  if (!requireApiAuth()) return;
  ...
}
```

**Superseded first attempt (2026-06-18T21:42Z):** First attempted in the working tree with a physical-presence mutation gate. `webapi.cpp` rejected mutating routes with `403 physical unlock required` unless `webApiAuthorizeMutationsFor()` opened a short write window. Build gate after this attempt passed: `/tmp/p7-build-cr03-mutation-gate.log`, RAM 38.0%, flash 90.7%, firmware payload 2,857,984 bytes. This attempt was not flashed and was rejected by the follow-up review below.

**Reopened by subagent review (2026-06-18T21:52Z):** Still BLOCKER. The write window is too broad because ordinary page navigation/taps can open all mutating LAN APIs, and long-press config stops the API server while the portal is active. The fix must remove unlock calls from casual short tap/page navigation and require either a per-request admin token or a strict config-mode flag set only on explicit config entry and cleared on portal exit.

**Second resolution update (2026-06-18T22:00Z):** Addressed in code/build; at that timestamp it was not yet flashed. The custom fork now uses `webApiSetConfigMode(bool)` instead of a timed unlock; casual touch/page navigation no longer opens mutating APIs; WiFiManager portal lifecycle enables the config-mode gate only while entering local config and locks it before the API listener resumes. Build passed: `/tmp/p7-build-config-gate-secret-redaction.log`, RAM 38.0%, flash 90.6%, firmware payload 2,857,856 bytes. The later 22:53Z no-touch-SPIFFS-format build includes this fix and was flashed/verified.

### CR-04: Sensitive settings still leak through serial, SD, and read APIs

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`

**Issue:** Subagent review found remaining wallet/pool-password exposure paths in serial logging, `/sd/config.json` mirroring, and unauthenticated read APIs. This conflicts with the repo rule to avoid committing or preserving secrets, Wi-Fi passwords, wallet values, device licenses, serial logs, or flash backups.

**Fix:** Remove or mask wallet/pool-password serial prints, do not mirror wallet or pool passwords to removable SD unless explicitly encrypted/opted in, and redact or gate wallet-bearing API fields behind the same strict auth/config-mode control used for mutating endpoints.

**Resolution update (2026-06-18T22:00Z):** Addressed in code/build; at that timestamp it was not yet flashed. `writeConfigToSD()` no longer writes `BtcWallet` or `PoolPassword`; serial prints for pool password and wallet are redacted; legacy `SDCard.cpp` wallet logging is redacted; `/api/system/info` and `/api/setting/mining` redact wallet-bearing read fields while exposing configured booleans. Static grep found no old timed unlock or SoftSPI flag, and `git diff --check` passed. The later 22:53Z no-touch-SPIFFS-format build includes this fix and was flashed/verified.

### CR-07: TFT_eTouch SPIFFS formatter wipes the decoded LittleFS asset partition

**Classification:** BLOCKER
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp`

**Issue:** Runtime serial evidence showed `TFT_eTouchBase::init()` mounting and formatting SPIFFS on boot. On the `huge_app.csv` layout, the `spiffs` data partition at `0x310000` is the same region the custom firmware mounts as LittleFS for decoded stock `.ss` screensaver assets. That meant the touch bring-up could erase the recovered LittleFS image before `nm_screensaver.cpp` tried to enumerate `/ss`, causing `Corrupted dir pair` and missing asset failures.

**Resolution update (2026-06-18T22:53Z):** Addressed in code, built, flashed, and verified on-device. The CYD touch bring-up now initializes HSPI/CS directly and does not call `TFT_eTouch::init()`. The stock display-driver fallback call site was patched the same way. Build passed, the patched custom app stack verified with `verify_flash`, the decoded LittleFS image was reflashed at `0x310000`, and that region still verified byte-for-byte after runtime boots and after NVS clear. Touch still reports no press signal, but it no longer formats the decoded asset partition.

### CR-05: Non-target CYD build targets remain too easy to select

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/platformio.ini`

**Issue:** The workspace `default_envs` included many boards, including ILI9341 CYD-like variants. This made a wrong-artifact build easy before a reflash.

**Resolution update (2026-06-18T22:00Z):** Addressed in code/build; at that timestamp it was not yet flashed. `default_envs` is narrowed to `ESP32-2432S028R`, the target env remains `-DST7789_DRIVER=1`, and the `ESP32-2432S024` env no longer defines the `ESP32_2432S028R` board macro. The later 22:53Z no-touch-SPIFFS-format build includes this fix and was flashed/verified.

### CR-06: Touch build flag described the wrong bus contract

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/platformio.ini`, `src/ui_nm/nm_touch.*`, `src/TouchHandler.*`, `src/NerdMinerV2.ino.cpp`

**Issue:** The target env still used `NM_TOUCH_SOFTSPI` comments/guards even though the current active path uses HSPI/TFT_eTouch plus direct probes. This made verification falsely look like a SoftSPI requirement was satisfied.

**Resolution update (2026-06-18T22:00Z):** Addressed in code/build; at that timestamp it was not yet flashed. The flag is now `NM_TOUCH_NO_VSPI`, comments describe the HSPI/no-VSPI contract, and legacy VSPI `TouchHandler` remains gated out for the CYD target. The later 22:53Z no-touch-SPIFFS-format build includes this fix and was flashed/verified.

## Warnings

### WR-01: Touch driver swapped XPT2046 X/Y read commands

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp:40`

**Issue:** The earlier touch path defined `XPT_CMD_X` as `0x90` and `XPT_CMD_Y` as `0xD0`, then mapped those reads directly to raw X and raw Y. The bundled TFT_eSPI XPT2046 implementation reads screen X using `0xD0` and screen Y using `0x90` (`lib/TFT_eSPI/Extensions/Touch.cpp:65-92`). Current gestures mostly use pressure, so taps still work, but corner diagnostics and any future coordinate-based UI would report/transmit swapped axes and rotation calibration would be tuned against the wrong raw channel.

**Resolution update (2026-06-19T00:55Z):** Addressed in current source. `nm_touch.cpp` now defines screen X as `0xD0` and screen Y as `0x90`, matching TFT_eSPI. The remaining touch failure is not this warning; the active diagnostics still show no raw contact at all.

```cpp
#define XPT_CMD_X  0xD0
#define XPT_CMD_Y  0x90
```

### WR-02: Manual portal display changes are saved but not applied until reboot

**Classification:** WARNING
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:785`

**Issue:** Boot-time portal changes get applied because setup calls `nm_display_apply_runtime_settings()` immediately after `init_WifiManager()` (`NerdMinerV2.ino.cpp:186-189`). The manual portal save path only calls `persistPortalSettingsFromParameters(true)` from `wifiManagerProcess()` (`wManager.cpp:785-786`), which saves NVS/SD but never calls the runtime display apply function. In contrast, the API preference route calls `nm_display_apply_runtime_settings()` after brightness/rotation changes (`webapi.cpp:511-513`). A user changing brightness or inversion through long-press config mode will see the saved value fail to take effect until a restart, making recovery-mode visual validation misleading.

**Fix:** Track whether portal-saved display fields changed and call `nm_display_apply_runtime_settings()` after persisting them on CYD builds.

```cpp
if (shouldSaveConfig) {
  bool displayChanged = persistPortalSettingsFromParameters(true);
#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  if (displayChanged) nm_display_apply_runtime_settings();
#endif
}
```

## Notes

Reviewed with Phase 7 planning and prior review artifacts as context. I did not include raw serial logs, wallet values, MACs, flash-backup hashes, or device secrets in this report.

_Reviewed: 2026-06-18T20:59:20Z_
_Reviewer: Codex gsd-code-reviewer_
_Depth: deep_
