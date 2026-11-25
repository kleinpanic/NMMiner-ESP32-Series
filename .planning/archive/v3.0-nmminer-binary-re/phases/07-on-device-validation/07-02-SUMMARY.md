---
phase: 07-on-device-validation
plan: 02
subsystem: firmware-cyd-recovery
tags: [phase7, gap-closure, wifi-manager, lvgl, sd, touch, display, build-gate]
requires:
  - phase: 07-on-device-validation
    provides: "07-REVIEW.md H1-H5 failure analysis and 07-01 hardware runbook"
provides:
  - "Historical pre-flash H1-H5 software repair set for the failed v3.0 LVGL build"
  - "NVS-first Wi-Fi config, SD fallback import, and no firmware-written WifiPW on SD"
  - "Opaque LVGL Loading/Config roots and LVGL config AP/IP feedback"
  - "Rotation-aware touch/display runtime settings instrumentation"
  - "Build-passed ESP32-2432S028R artifact; later Phase 7 evidence supersedes the original hardware/driver gate"
affects:
  - "Historical 07-01 hardware validation gate only; current next action is physical baseline/touch probe plus remaining Phase 7 runtime gates"
  - "Phase 8 fallback decision depends on the remaining Phase 7 on-device result"
tech-stack:
  added: []
  patterns:
    - "Nonblocking WiFiManager portal loop pumps LVGL on CYD"
    - "Process-lifetime WiFiManagerParameter ownership for manual portal reuse"
    - "SD config is fallback/import-only; NVS wins when usable"
    - "Runtime display settings apply after config load and through the preference API"
key-files:
  created:
    - .planning/phases/07-on-device-validation/07-REVIEW-FIX.md
    - .planning/phases/07-on-device-validation/07-REVIEW-02.md
    - .planning/phases/07-on-device-validation/07-REVIEW-03.md
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
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
    - .planning/phases/07-on-device-validation/07-VALIDATION.md
decisions:
  - "Do not flash during original 07-02; preserve the hardware write gate that existed at the time."
  - "Historical driver note: original 07-02 kept the then-current ILI9341_2_DRIVER flag for software-only repair; later Phase 7 work resolved the live target to ST7789."
  - "Do not write WifiPW to SD; read SD WifiPW only as operator-supplied import data."
patterns-established:
  - "Portal save state must sync WiFiManager credentials before NVS persistence."
  - "LVGL config entry must wake display, stop screensaver/dim, and avoid LastPage persistence."
requirements-completed: []
metrics:
  duration: "multi-pass repair + review"
  completed: 2026-06-18
---

# Phase 7 Plan 02: Pre-Flash Gap Closure Summary

> **Current-state note (2026-06-18T23:48:22Z):** This summary accurately closes the 07-02
> software-only repair pass as it stood at the time. Later Phase 7 work resolved the ST7789
> flash target, reflashed the custom decoded-spec app, verified decoded LittleFS at `0x310000`,
> and rechecked the user-reported AP/touch failure. Use `07-VALIDATION.md` and
> `07-VERIFICATION.md` for live device state; do not use this summary as proof that VALID-01,
> VALID-02, or VALID-03 are complete.

Reviewed and repaired the known Phase 7 LVGL/WiFi/SD/display failures so the next gated hardware attempt would not run against known broken software. This summary closes the 07-02 software plan only. Later Phase 7 work resolved the live board-driver decision to ST7789 and flashed/verified the custom app; VALID-01, VALID-02, and VALID-03 remain open only because the physical/runtime evidence is incomplete.

## Performance

- Duration: multi-pass repair, review, follow-up fix, and final build on 2026-06-18.
- Tasks: 3 of 3 software tasks complete.
- Files modified: 13 firmware source/header files plus 07-VALIDATION.md.
- Review artifacts: 07-REVIEW-FIX.md, 07-REVIEW-02.md, 07-REVIEW-03.md, and read-only subagent recheck notes in this summary.

## Accomplishments

- H1 closed in code: NVS Wi-Fi config wins; SD config is fallback/import-only; firmware no longer writes `WifiPW` to removable SD config; oversized or truncated SD config reads fail closed.
- H1 follow-up closed: WiFiManager portal parameters now have process-lifetime storage, portal saves sync submitted SSID/password into `Settings` before NVS persistence, and `/sd` path normalization is idempotent.
- H1 residual blocker closed: `wManager.cpp` no longer falls through to legacy `SDCrd.loadConfigFile()` / `SD2nvMemory()` and no longer calls `SDCrd.terminate()` after `SDStore` is mounted.
- H5 closed in code: Loading and Config screen roots set explicit black backgrounds with `LV_OPA_COVER`.
- H2 closed in code: WiFiManager config mode routes AP SSID and portal IP through the LVGL Config page without persisting `LastPage`.
- H3 prepared for hardware calibration: touch mapping tracks landscape rotation 1/3, default rotation 3 inverts both base axes, and diagnostics log rotation plus raw/calibrated points.
- H4 closed in code: brightness, invert colors, and supported landscape `RotateScreen` are applied after config load and immediately through the preference API; unsupported rotations are rejected before settings mutation.
- Build gate passed for `ESP32-2432S028R` without running upload, backup, flash, verify, restore, or serial-log capture commands during the original 07-02 pass.
- Historical board-driver note: at the end of the original 07-02 pass, the driver gate was still unresolved. That state is superseded by later Phase 7 evidence: the active build now uses `-DST7789_DRIVER=1`, was flashed, and was verified as the live custom decoded-spec ST7789 app.

## Files Created/Modified

- Created planning/review artifacts: `07-REVIEW-FIX.md`, `07-REVIEW-02.md`, `07-REVIEW-03.md`, and this `07-02-SUMMARY.md`.
- Updated validation artifact: `07-VALIDATION.md`.
- Firmware repairs live in `wManager.cpp`, `sdstore.cpp`, `NerdMinerV2.ino.cpp`, `webapi.cpp`, and the `src/ui_nm/` display, touch, screen, and UI manager files listed in frontmatter.

## Decisions Made

- 07-02 remained a code/build/documentation repair plan only. Later Phase 7 work superseded that pre-flash gate: the custom ST7789 app was flashed and verified after preserving a fresh full backup.
- Current open hardware actions are the physical touch/probe path, operator-confirmed hard power-cycle proof, 2.4 GHz Wi-Fi/pool/mining evidence, API/dashboard evidence, and restore proof; see `07-VERIFICATION.md`.
- The board-driver conflict was deliberately left explicit during the original 07-02 pass instead of silently changed. Later Phase 7 work resolved it to the repo-observed `esp32-2432s028r-st7789` / ST7789 path before the current custom flash.
- `WifiPW` is not firmware-written to SD. If present on SD, it is treated only as operator-supplied one-time import data when NVS has no usable SSID.

## Deviations from Plan

- The first repair pass needed a second review/fix cycle. `07-REVIEW-02.md` found WiFiManager parameter lifetime, portal credential sync, and SD path normalization issues; all were fixed.
- `07-REVIEW-03.md` found one residual SD import warning where candidate credentials could mutate `Settings` before validation. That follow-up warning is closed in the final working tree.
- A later read-only subagent review found legacy `SDCard` fallback/teardown blockers and unsupported `RotateScreen` persistence. Those code issues are now fixed, build-passed, and rechecked.
- No hardware validation was attempted. That is intentional under the Phase 7 hardware gate and means this plan does not complete VALID-01/02/03 by itself.

## Verification Results

- Build: `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R` -> SUCCESS in 00:00:23.101.
- RAM: 38.0% (124,376 / 327,680 bytes).
- Flash: 90.6% (2,849,041 / 3,145,728 bytes).
- Firmware payload: 2,855,344 bytes at 0x010000.
- Factory image: 2,924,544 bytes.
- Final review state: `07-REVIEW-03.md` plus the later subagent recheck have no remaining H1/H4 code blockers; the board-driver conflict remains a hardware/target gate.
- Parent repo `git diff --check`: passed.
- Nested firmware repo `git diff --check`: passed.
- `writeConfigToSD()` static check: no `WifiPW` or `JSON_KEY_PASW` reference in the function body.
- Staged artifact guard: no staged backup or `.bin` artifact.
- GSD roadmap query after this summary: Phase 7 is partial with 2 plans / 1 summary; milestone total is 23 plans / 22 summaries.
- Safety state: no backup, flash, upload, verify, restore, or serial-log capture command was run during 07-02; no backup binary is part of this plan's tracked artifacts.

## Next Phase Readiness

The original 07-02 readiness notes are historical. Later Phase 7 work superseded them by backing up, flashing, and verifying the custom decoded-spec ST7789 app and decoded LittleFS assets. Phase 8 is still not next; Phase 7 remains open because physical runtime proof is missing.

Current required sequence:

1. Capture operator-confirmed hard USB unplug/replug proof, then confirm the same clean recovery AP boot.
2. Run the physical touch probe path in `07-TOUCH-PROBE-RUNBOOK.md`: inspect/probe XPT2046 or touch-controller variant, FPC, PENIRQ/GPIO36, and CS/CLK/MOSI/MISO.
3. Keep ST7789 as the active board-driver decision unless new physical re-identification evidence disproves it.
4. Join a 2.4 GHz Wi-Fi network, confirm LAN/API reachability, pool connection, nonzero hashrate, and at least one accepted share.
5. Verify speaker, LED, SD logging/API, and dashboard panels under real mining/runtime conditions.
6. Prove restore reversibility with an opt-in round trip, or record an explicit accepted deferral while preserving backup integrity.

## Self-Check: PASSED FOR 07-02 SOFTWARE REPAIR ONLY

- H1-H5 software repairs are present in the working tree.
- The CYD firmware build passed.
- Follow-up code review blockers are closed.
- The original hardware gate and unresolved board-driver state are preserved as historical context; current gates are the physical/runtime items listed above.
- VALID-01, VALID-02, and VALID-03 remain pending until hardware evidence exists.
