---
status: investigating
trigger: "Current custom decoded-spec ST7789 firmware boots recovery AP and preserves decoded assets, but touchscreen reports no press signal and the miner is not responsive to touches."
created: 2026-06-18T23:06:00Z
updated: 2026-06-19T17:34:35Z
---

# Debug Session: touch-no-press

## Symptoms

- Expected behavior: Touch input on the ESP32-2432S028R CYD should produce press events so the custom UI can navigate pages, enter config, wake the screen, and satisfy TOUCH-01/02 validation.
- Actual behavior: Device boots the custom decoded-spec firmware to recovery AP, but touch diagnostics repeatedly report no press signal.
- Error messages: No panic or Guru Meditation in the current clean log. Repeated diagnostics show `directPressed=0`, `libPressed=0`, `irq=1`, `miso=0`, direct raw samples around `directX=0`, `directY=4095`, `directZ1=1..4`, `directZ2=4080..4095`, low `directP`.
- Timeline: Started during Phase 7 on-device validation after Phases 3-6 were marked compile/code-review complete. The asset-wipe issue from `TFT_eTouch::init()` has been fixed; touch no-press remains.
- Reproduction: Boot current custom firmware, let it enter recovery AP `nmap-2.4g` / `192.168.4.1`, touch or hold the panel, and observe serial diagnostics.

## Current Focus

- hypothesis: Active firmware path reaches the XPT2046 probe code, but no physical pen/contact signal is reaching the assumed CYD touch interface.
- test: Compare active CYD touch init/poll/log source against serial probe output from prompted tap/hold runs.
- expecting: If the code path is alive but contact is absent, logs will show repeated direct and library no-press values, IRQ idle/high, and no calibrated press edge despite the main loop polling touch.
- next_action: Do not run more calibration, threshold, or host-side-only diagnostics unless the hardware state has changed; they would duplicate existing tap/hold serial evidence. The next useful step is physical measurement/re-identification: visually confirm the touch controller/FPC and probe XPT2046 PENIRQ/GPIO36 during a firm press, then confirm CS=33, CLK=25, MOSI=32, MISO=39 activity at the controller or identify a different/no-touch controller variant.
- reasoning_checkpoint: asset partition preservation is fixed and should not be mixed with the remaining touch fault.
- tdd_checkpoint: hardware-facing; no automated unit test can prove touch until the physical signal exists.

## Evidence

- timestamp: 2026-06-18T22:53:00Z
  source: `.planning/phases/07-on-device-validation/07-VERIFICATION.md`
  observation: Current custom firmware and decoded LittleFS assets verify; touch diagnostics still show no press signal.
- timestamp: 2026-06-18T23:06:00Z
  source: `/tmp/p7-serial-clean-nvs-custom-decoded-20260618-185211.log`
  observation: Clean recovery AP boot plus repeated no-press diagnostics.
- timestamp: 2026-06-18T23:06:00Z
  source: `/tmp/p7-serial-touch-direct-probe.log`, `/tmp/p7-serial-hspi-touch.log`
  observation: Earlier direct-probe and HSPI-touch logs also show no press signal.
- timestamp: 2026-06-18T23:12:14Z
  source: `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp`
  observation: The active CYD setup calls `nm_touch_init()` and the main loop calls `nm_touch_poll_gesture()`. That poll emits idle diagnostics through `nm_touch_log_raw()`, so the serial probe output is from the live touch path, not dead code.
- timestamp: 2026-06-18T23:12:14Z
  source: `/tmp/p7-serial-touch-direct-probe.log`, `/tmp/p7-serial-clean-nvs-custom-decoded-20260618-185211.log`
  observation: Prompted tap/hold runs repeatedly show `irq=1`, `directX=0`, `directY=4095`, `directZ1` near zero, `directZ2` near full-scale, `directP` around 3..18, and both direct/library pressed flags false. This is a no-contact/no-pen signal, not a rotation or pixel-calibration failure.
- timestamp: 2026-06-18T23:23:00Z
  source: `/tmp/p7-serial-after-current-code-review-fix-reflash-20260618-232130.log`
  observation: After the latest code-review app reflash, the firmware still boots the active HSPI touch path and repeatedly logs `directPressed=0` / `libPressed=0` while the recovery portal starts. The latest app fixes did not change the root-cause class.
- timestamp: 2026-06-18T23:31:51Z
  source: `.planning/phases/07-on-device-validation/07-VERIFICATION.md`, `.planning/phases/07-on-device-validation/07-VALIDATION.md`, `firmware/custom/NerdMiner-CYD-Custom/platformio.ini`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp`, `/tmp/p7-serial-touch-direct-probe.log`, `/tmp/p7-serial-hspi-touch.log`, `/tmp/p7-serial-after-current-code-review-fix-reflash-20260618-232130.log`
  observation: Host-side review found no remaining safe firmware-free diagnostic expected to add new signal before physical probing. The current ST7789 CYD env enables `TOUCH_ENABLE` and `NM_TOUCH_NO_VSPI`, initializes HSPI pins `clk=25 mosi=32 miso=39 cs=33 irq=36`, and polls `nm_touch_poll_gesture()` as the sole CYD touch path. Saved prompted tap/hold serial runs already exercise the direct and TFT_eTouch readers and still show idle IRQ/high, MISO low, near-zero pressure, and no press edge.
- timestamp: 2026-06-18T23:39:31Z
  source: `/tmp/p7-verify-current-custom-app-after-user-report-20260618-193918.log`, `/tmp/p7-verify-decoded-littlefs-after-user-report-20260618-193918.log`, `/tmp/p7-serial-after-user-report-verify-20260618-193931.log`
  observation: After the user reported a red LED, an AP join/auth/captive-portal popup, and no response to touch, the live board was non-destructively re-verified. The app at `0x10000` matches the current custom `ESP32-2432S028R` build and the decoded LittleFS image at `0x310000` matches the recovered asset image. Fresh serial after reset again enters WiFiManager recovery mode at `nmap-2.4g` / `192.168.4.1` and repeats `directPressed=0` / `libPressed=0`, confirming the report is still the same no-physical-touch-signal failure class, not an unflashed-stock state.
- timestamp: 2026-06-18T23:59:03Z
  source: `/tmp/p7-verify-live-current-custom-app-20260618-195813.log`, `/tmp/p7-verify-live-current-decoded-littlefs-20260618-195813.log`, `/tmp/p7-serial-live-current-after-verify-20260618-195813.log`
  observation: A second live recheck again verified the custom decoded-spec app at `0x10000` and decoded LittleFS at `0x310000` without any firmware write. Raw serial still reports the same no-contact signature (`irq=1`, `directX=0`, `directY=4095`, `directPressed=0`, `libPressed=0`). The AP/captive-portal popup and red LED are consistent with WiFiManager waiting-config/connecting state; they do not change the touch root-cause class.
- timestamp: 2026-06-19T00:29:04Z
  source: `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`
  observation: After the custom app was rebuilt/flashed to report recovered `v2.0.02`, serial captured a clean boot to WiFiManager AP mode with SD mounted and no panic/boot-loop markers. The same no-contact touch signature persists: `irq=1`, `miso=0`, `directX=0`, `directY=4095`, `directPressed=0`, and `libPressed=0`.
- timestamp: 2026-06-19T00:55:00Z
  source: `pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R -t envdump`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp`
  observation: Rechecked the compiled bus contract after the `v2.0.02` identity flash. `CPPDEFINES` includes `TOUCH_ENABLE` and `NM_TOUCH_NO_VSPI`, and does not include `USE_HSPI_PORT` or `CONFIG_TFT_eSPI_ESPIDF`. The active CYD loop calls only `nm_touch_poll_gesture()`; legacy `TouchHandler` is gated out. Current source uses HSPI/TFT_eTouch plus direct HSPI probes for touch and keeps VSPI unavailable to the legacy touch handler. The older Phase 4 SoftSPI language is historical/superseded, not the current runtime contract.
- timestamp: 2026-06-19T01:16:29Z
  source: `.planning/phases/07-on-device-validation/07-TOUCH-SOFTWARE-REVIEW.md`
  observation: A focused no-flash software review found 0 critical touch findings and 1 warning. The warning is a real ownership defect: legacy `doLedStuff()` still dispatched to the CYD display driver from the monitor task, where it could poll `touch.getXY()` and mutate legacy screen/backlight state. The review judged it insufficient to explain the current idle/no-contact diagnostics because a real press should still pull PENIRQ/GPIO36 low and produce pressure on the direct probe.
- timestamp: 2026-06-19T01:19:29Z
  source: `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp`, `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R`
  observation: Patched `doLedStuff()` to no-op on `ESP32_2432S028R` / `ESP32_2432S028_2USB`, matching the existing CYD no-ops for legacy draw/animate calls. Build passed with RAM 38.0%, flash 89.7%, local unflashed `firmware.bin` size 2,826,672 bytes and SHA-256 `a2452282b428e8fbea9939a84958b4c36b6937896b8bca31526d108e724c92aa`; this artifact is superseded by the 03:04 UTC source-review-fix build.
- timestamp: 2026-06-19T03:04:45Z
  source: `.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-02.md`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`, `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R`
  observation: A follow-up firmware review found a non-touch source blocker in `readCustomAPName()` and a defensive `.ss` upload header-validation warning. A second follow-up found an AP-name serial leak and final-path-before-validation upload gap. These were patched and rebuilt without flashing; a final narrow review found no remaining source blocker. Latest local unflashed `firmware.bin` is 2,827,504 bytes with SHA-256 `32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b`.
  implication: This was true at the review timestamp only. Later fallback-pool, source-blocker, review-05, and webfix2 app flashes superseded the `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1` app. The current live app is the webfix2 custom decoded-spec artifact, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`; the touch root-cause class remains raw no-contact below UI/LVGL/calibration.
- timestamp: 2026-06-19T02:06:22Z
  source: `/tmp/p7-serial-readonly-continuation-20260619-020602.log`
  observation: A DTR/RTS-low read-only serial listen captured a fresh clean boot without flash/write/erase/settings mutation. The log is 4,607 bytes, 55 lines, SHA-256 `ae240cbed25eb7588dfe05f69615401cc83524c992632ffbab0d48cd03a3a045`; it shows `POWERON_RESET`, recovery AP `nmap-2.4g` / `192.168.4.1`, no `Guru Meditation` / `PANIC`, and the same repeated no-contact touch signature: `irq=1`, `miso=0`, `directX=0`, `directY=4095`, `directPressed=0`, `libPressed=0`.
- timestamp: 2026-06-19T06:18:00Z
  source: no-write reset touch window, sanitized console counts only
  observation: Reset-only 45s touch window read 49 serial lines with 31 touch diagnostics, 0 pressed observations, 0 IRQ-low observations, and 0 panic lines. The init line confirmed the current HSPI touch pins as `clk=25 mosi=32 miso=39 cs=33 irq=36`. No raw serial text was written into tracked files.
- timestamp: 2026-06-19T17:33:44Z
  source: `/tmp/p7-serial-passive-inline-20260619-173344.log`
  observation: Passive no-write serial listen captured 5,177 bytes / 51 lines with SHA-256 `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`. The recovery AP was still active at `192.168.4.1`; the sample contained 23 touch diagnostics, 0 pressed observations, 0 IRQ-low observations, and no panic evidence beyond the expected power-on reset marker. Representative diagnostics remained `irq=1`, `miso=0`, `directX=0`, `directY=4095`, `directPressed=0`, and `libPressed=0`.

## Eliminated

- hypothesis: Decoded LittleFS asset corruption causes the touch failure.
  evidence: Decoded LittleFS verifies after runtime boots and after NVS clear; touch no-press persists separately.
- hypothesis: Seeing `v2.0.02` means the official stock binary is live.
  evidence: Current verification states the live firmware is the custom decoded-spec ST7789 fork, rebuilt to report recovered `v2.0.02` for parity; stock is historical baseline only.
- hypothesis: Touch failure is caused by LVGL/UI gesture routing, tap debounce, rotation mapping, or calibration constants.
  evidence: The failure occurs below those layers: `nm_touch_log_raw()` shows no raw press/contact signal and no calibrated point is produced.
- hypothesis: Legacy VSPI `TouchHandler` is consuming or conflicting with the CYD touch path.
  evidence: The CYD build defines `NM_TOUCH_NO_VSPI`, which gates out `TouchHandler`; the main loop calls only `nm_touch_poll_gesture()` for the CYD path.
- hypothesis: The remaining touch failure is explained by the stale Phase 4 SoftSPI documentation mismatch.
  evidence: Current envdump/source prove the live contract is `NM_TOUCH_NO_VSPI` with HSPI/TFT_eTouch plus direct probes, and serial diagnostics from that active path still show no contact. This mismatch needed cleanup, but it does not by itself create a press signal.
- hypothesis: A second legacy `doLedStuff()` touch reader is the current cause of no press.
  evidence: It was a real warning-level software ownership defect and is now patched compile-only, but the live direct probe repeatedly sees idle IRQ/high and low pressure before UI routing. The defect could contaminate future diagnostics once physical contact exists; it does not explain the present no-contact signature by itself.

## Resolution

- root_cause: The current evidence identifies the root-cause class as absent physical XPT2046 pen/contact signal on the assumed CYD touch interface, not a UI/calibration bug. The exact physical reason remains to be distinguished between wrong touch variant/pin map, disconnected or damaged resistive touch panel/FPC, missing/unpowered XPT2046 path, or controller-level hardware fault.
- fix: The warning-level legacy `doLedStuff()` second-reader path and the later non-touch firmware-review blockers have been patched in source and compile-verified, but not flashed. No touch "fix" patch is justified without physical measurement. The next safe action is electrical/visual verification of the touch controller and pins; a future diagnostic-only firmware patch may be safe after a fresh backup.
- verification: Live custom firmware boots; `nm_touch_init()` and `nm_touch_poll_gesture()` are active; repeated prompted/read-only/passive serial probes through 2026-06-19T17:33Z show idle IRQ/high and raw no-contact pressure on both direct HSPI and TFT_eTouch library paths.
- files_changed: `.planning/debug/touch-no-press.md`, `.planning/phases/07-on-device-validation/07-TOUCH-SOFTWARE-REVIEW.md`, `.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-02.md`, `.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-03.md`, `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`; stale live-source/docs comments were cleaned up separately so current Phase 7 evidence no longer implies SoftSPI is active.
