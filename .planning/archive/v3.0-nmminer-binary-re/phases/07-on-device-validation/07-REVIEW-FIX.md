# Phase 7 — 07-02 Review Fix Notes

**Date:** 2026-06-18
**Status:** SOFTWARE REPAIRS BUILD-PASSED; HARDWARE VALIDATION AND DRIVER DECISION STILL GATED
**Plan:** 07-02 gap closure
**Source reviews:** 07-REVIEW.md H1-H5, 07-REVIEW-02.md, 07-REVIEW-03.md

## Summary

07-02 closes the known pre-flash software defects from the failed Phase 7 run without touching hardware. No backup, serial capture, `read_flash`, `write_flash`, `verify_flash`, `pio upload`, or restore command was run.

## H1 — SD fallback, NVS authority, no Wi-Fi password writes

Files:

- `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp`

Status: closed for pre-flash software repair.

Evidence:

- `init_WifiManager()` loads NVS first and only calls `tryConfigFromSD()` when NVS has no usable SSID.
- `writeConfigToSD()` no longer serializes `WifiPW`.
- `tryConfigFromSD()` imports app settings from SD without promoting SD Wi-Fi fields until candidate SSID/password values pass `hasUsableWifiCredentials()`.
- WiFiManager portal parameters now have process-lifetime storage and are shared by boot-time and manual portal entry.
- Portal saves copy WiFiManager's persisted SSID/password into `Settings` before `nvMem.saveConfig()`.
- `SDStore::sanitizePath()` handles already-rooted `/sd` paths without double-prefixing.
- SD config parsing uses a 1024-byte buffer/document.
- `SDStore::readConfig()` rejects zero-length or oversized config files before reading, preventing partial JSON from being parsed as valid config.
- `wManager.cpp` no longer calls legacy `SDCrd.loadConfigFile()` / `SD2nvMemory()`, so `/config.json` cannot bypass `tryConfigFromSD()` validation.
- `wManager.cpp` no longer calls `SDCrd.terminate()` after `sdStore.mount()`, so Wi-Fi init cannot tear down the shared SD singleton behind `SDStore`.

## H5 — Opaque Loading and Config screens

Files:

- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp`

Status: closed for pre-flash software repair.

Evidence:

- Both LVGL screen roots set explicit black background color and `LV_OPA_COVER`.

## H2 — Config-mode on-screen feedback

Files:

- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h`
- `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp`

Status: closed for pre-flash software repair.

Evidence:

- `nm_screen_config_set_portal_info()` updates visible AP/IP labels.
- `nm_uimanager_show_config()` loads the Config page without persisting `LastPage` and pumps enough LVGL passes for a full frame.
- `configModeCallback()`, `enter_config_portal()`, and forced portal entry route through the LVGL config page.
- Manual recovery path remains: long-press `BOOT`, join `nmap-2.4g`, browse to `192.168.4.1`.

## H3 — Rotation-aware touch mapping

Files:

- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp`

Status: ready for next hardware calibration pass; final calibration still requires on-device corner samples.

Evidence:

- Touch no longer presents rotation-1 as the only active mapping.
- `nm_touch_set_display_rotation()` tracks landscape display rotation 1 or 3.
- Rotation 3 maps by inverting both base landscape axes.
- `nm_touch_log_raw()` records active rotation plus raw and calibrated coordinates for the first on-device presses.

## H4 — Runtime display settings

Files:

- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h`
- `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`

Status: closed for pre-flash software repair.

Evidence:

- `nm_display_apply_runtime_settings()` applies `Settings.invertColors`, `Settings.Brightness`, supported landscape `RotateScreen`, and touch rotation.
- `setup()` calls it after `init_WifiManager()` returns, so loaded/promoted config is reflected on the LVGL path.
- `/api/setting/preference` applies brightness and `RotateScreen` changes immediately through the same display path.
- `/api/setting/preference` rejects unsupported `RotateScreen` values before mutating settings and normalizes supported values to the landscape rotations the display driver applies.

## Build Evidence

Command:

```bash
.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
```

Result: SUCCESS in 00:00:23.101.

- RAM: 38.0% (124,376 / 327,680 bytes)
- Flash: 90.6% (2,849,041 / 3,145,728 bytes)
- Firmware payload: 2,855,344 bytes at 0x010000
- Factory image: 2,924,544 bytes

Observed warnings in the final build were environment/build warnings: SD SPI-mode support enabled and missing `boot_app0.bin` in the merged-firmware post-build step.

## Review Follow-up

`07-REVIEW-02.md` found two blockers and one warning after the first repair pass:

- manual config portal reused stack-local WiFiManager parameters after `init_WifiManager()` returned;
- portal saves persisted `Settings` without syncing submitted WiFiManager SSID/password;
- SD API paths could be normalized from `/sd/...` to `/sd/sd/...`.

Those findings are fixed in the final tree. `07-REVIEW-03.md` re-reviewed the fixes, found no blockers, and its follow-up resolution note confirms the remaining SD import warning is closed.

Later read-only subagent review found three additional pre-flash issues:

- legacy `SDCard` fallback could still bypass H1 validation after `tryConfigFromSD()` failed;
- `SDCrd.terminate()` could tear down the shared SD state after `SDStore` mounted;
- `/api/setting/preference` could persist unsupported `RotateScreen` values.

The two SD issues and the `RotateScreen` warning are fixed in the current tree and were rechecked by the subagent. The remaining item is the board-driver gate below.

## Board Driver Decision

No panel-driver flag was changed in 07-02.

Historical evidence conflict:

- Repo instructions identify the observed unit as `esp32-2432s028r-st7789` and warn against flashing an `ili9341` build unless hardware is re-identified.
- During the original 07-02 software-only pass, the fork still contained stale ILI9341 driver evidence.

Superseded decision: later Phase 7 work resolved this to the decoded-spec `ESP32-2432S028R` ST7789 path, flashed that custom app, and verified the live image hash. Current driver status is governed by `07-CURRENT-FAILURE-STATE.md`, `07-VALIDATION.md`, and `07-VERIFICATION.md`, not this historical pre-flash review note.

## Gate Status

No forbidden artifacts were staged or committed. No actual Wi-Fi passwords, wallet values, device licenses, serial logs, flash backups, or flash-backup hashes were added to tracked source or docs.

The original 07-02 pass ran no hardware mutation. Current open hardware gates are physical touch/probe evidence, operator-confirmed hard power-cycle proof, 2.4 GHz Wi-Fi/pool/mining evidence, API/dashboard evidence, and restore proof.
