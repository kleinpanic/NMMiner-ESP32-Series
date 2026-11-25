---
phase: 07-on-device-validation
verified: 2026-06-19T05:45:21Z
status: gaps_found
score: 0/3 VALID requirements fully verified; current app flash/data preservation/recovery web UI proven and clipping fixed, physical panel visual parity failed by user observation, touch/mining proof pending
requirements: [VALID-01, VALID-02, VALID-03]
blocking_gate: "physical UI failure classification + physical touch probe + hard USB power-cycle + Wi-Fi/pool/mining proof + restore proof"
gaps:
  - id: hard-power-cycle
    requirement: VALID-01
    status: open
    evidence_needed: "Operator physically unplugs/replugs USB and the same clean serial/visual recovery state returns"
  - id: physical-panel-led-touch
    requirement: VALID-02
    status: failed_unclassified
    evidence_needed: "No-secret physical photo/video or checklist classifying the user-observed wrong CYD panel content, colors, rotation, clipping/overlap, LED state, and touch behavior"
  - id: touch-physical-probe
    requirement: VALID-02
    status: open
    evidence_needed: "XPT2046/FPC/PENIRQ probe or board re-identification proving whether raw touch contact reaches GPIO36/CS/CLK/MOSI/MISO"
  - id: wifi-pool-mining
    requirement: VALID-02
    status: open
    evidence_needed: "2.4 GHz Wi-Fi join, LAN/API reachability, pool connection, nonzero hashrate, accepted shares, speaker/LED observations"
  - id: custom-api-exposure
    requirement: VALID-02
    status: open
    evidence_needed: "Decision and proof for custom JSON API exposure model; current recovery portal mode returns 404 for those routes"
  - id: restore-round-trip
    requirement: VALID-03
    status: open
    evidence_needed: "Full restore round-trip run by opt-in, or explicit deferral while preserving latest backup integrity and command"
re_verification:
  previous_status: "official stock ST7789 v2.0.02 was restored and verified as a recovery baseline; the live device has since been reflashed with the custom decoded-spec ST7789 fork plus decoded LittleFS assets, latest code-review app fixes, recovered v2.0.02 identity, fallback-pool support, post-forensics source-blocker fixes, and review-05 mining/config hardening"
overrides: []
human_verification:
  - test: "Physical display and LED confirmation"
    expected: "Operator classifies the current wrong physical CYD screen: visible screen/text, colors, rotation, clipping/overlap, stale regions/tearing, and LED state"
    why_human: "No camera/screen capture path is available from serial alone"
  - test: "Hard USB power-cycle"
    expected: "Operator unplugs/replugs USB and the same serial/visual portal behavior returns"
    why_human: "Soft RTS reset is not equivalent to the required physical power-cycle"
  - test: "Wi-Fi/pool/mining proof"
    expected: "Device joins 2.4 GHz Wi-Fi, exposes LAN API, hashrate is nonzero, accepted shares increase, speaker/SD checks pass"
    why_human: "Requires Wi-Fi credentials and physical audio/LED observation"
  - test: "Restore proof"
    expected: "Restore command is either run by opt-in or explicitly deferred, with backup integrity preserved"
    why_human: "Round-trip restore rewrites the device and should stay opt-in"
---

# Phase 7: On-Device Validation — Verification Report

**Phase Goal:** Prove the rebuilt fork firmware on the real device: fresh backup before flash, verified flash and hard power-cycle, serial boot, visual parity, active mining, and documented reversible restore.
**Verified:** 2026-06-19
**Status:** custom_decoded_spec_current_app_verified_recovery_web_pass_clipping_fixed / physical_panel_visual_parity_failed / not complete
**Re-verification:** Yes. This supersedes the earlier stock recovery-baseline report. The official stock `v2.0.02` ST7789 image was verified as a fallback, but it is no longer the live firmware. The current device has been backed up, reflashed with the patched custom `ESP32-2432S028R` ST7789 fork, reflashed with the decoded LittleFS `.ss` asset image at `0x310000`, app-reflashed with recovered `v2.0.02` identity, app-reflashed again with fallback-pool support, app-reflashed again with post-forensics source-blocker fixes, app-reflashed again with review-05 mining/config hardening, and app-reflashed again with the WiFiManager recovery portal CSS fix. The current app readback matches the built app at `0x10000`, serial reaches recovery/config mode, and the current recovery web portal returns HTTP 200 for root/Info/WiFiManager pages without the prior right-edge clipping.
**Latest recovery portal CSS app reflash:** On 2026-06-19T05:36Z, after a fresh 4 MB backup, the rebuilt custom app was flashed at `0x10000` and independently read back. The artifact is 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, contains the prior custom ST7789/recovery/fallback-pool/review-05 work plus the WiFiManager recovery portal CSS fix, and booted cleanly under a sanitized serial smoke with 0 panic lines. Decoded LittleFS at `0x310000` was not intentionally rewritten.
**Latest no-device build recheck:** A later `uvx platformio run -e ESP32-2432S028R` passed without upload/flash/erase/write commands. The rebuilt app remained 2,840,400 bytes with SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, matching the already-flashed webfix2 app artifact.
**Latest recovery web recheck:** The host joined `nmap-2.4g`. `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200; `/api/status`, `/api/system/info`, and `/sd/list` returned HTTP 404 in recovery portal mode. Current screenshots were captured under `/tmp/p7-webfix2-ui/`: root at 1440x900, 390x844, and 320x568, Configure WiFi at 390x844 and 320x568, and Info at 390x844. The prior 320 px root and 390 px Configure WiFi horizontal clipping is fixed in the current flashed app. These screenshots prove browser recovery UI reachability only, not the physical CYD panel/touch layer.
**Post-reflash cleanup/fallback builds:** Comments/docs were corrected to remove stale Phase 4 SoftSPI wording and an `envdump` recheck confirmed the current `ESP32-2432S028R` compile uses `TOUCH_ENABLE` + `NM_TOUCH_NO_VSPI` with no `USE_HSPI_PORT` or `CONFIG_TFT_eSPI_ESPIDF`. Later reviews fixed the legacy `doLedStuff()` second-reader path, bounded/redacted `readCustomAPName()`, redacted the config portal AP serial log, disabled WiFiManager debug output, hardened `.ss` upload header/staging validation, added fallback-pool persistence/API/portal/mining failover support, fixed post-forensics source blockers, fixed review-05 mining/config blockers, and fixed WiFiManager recovery portal narrow-layout clipping before the latest app flash. The live app is now SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.
**Latest physical UI observation:** On 2026-06-19 the user physically observed the live miner display and reported that it looks wrong and nothing like expected. `07-UI-REVIEW.md` now treats physical ST7789/LVGL visual parity as a current failed gate and scores Phase 7 UI at 8/24. No photo, text sample, color/orientation detail, or screen classification has been captured yet; this proves failure, not root cause.
**Latest serial recheck:** A no-write reset-only 45s touch window after the webfix2 app flash read 49 lines, found 0 panic lines, and counted 31 touch diagnostics with 0 pressed observations and 0 IRQ-low observations. A later passive no-write 25s serial window at 2026-06-19T17:33Z wrote only to `/tmp/p7-serial-passive-inline-20260619-173344.log`, captured 51 lines, reached recovery AP at `192.168.4.1`, and counted 23 touch diagnostics with 0 pressed observations and 0 IRQ-low observations. Raw serial output was not stored in tracked files. This does not close the hard physical power-cycle gate because no operator-confirmed USB unplug/replug evidence was collected.
**Previous user-report recheck:** After the user reported a red LED, AP join/auth/captive-portal popup, and no touch response, the board was checked without writing firmware. `/tmp/p7-verify-current-custom-app-after-user-report-20260618-193918.log` verified the app at `0x10000`, `/tmp/p7-verify-decoded-littlefs-after-user-report-20260618-193918.log` verified decoded LittleFS at `0x310000`, and `/tmp/p7-serial-after-user-report-verify-20260618-193931.log` again reached WiFiManager recovery AP mode while reporting no raw touch press. A second 19:58 EDT live recheck reconfirmed the same state in `/tmp/p7-verify-live-current-custom-app-20260618-195813.log`, `/tmp/p7-verify-live-current-decoded-littlefs-20260618-195813.log`, and `/tmp/p7-serial-live-current-after-verify-20260618-195813.log`: app and decoded LittleFS digest-matched; touch stayed `directPressed=0` / `libPressed=0`.

## Verdict

Phase 7 is **not complete**. The current device firmware is the custom decoded-spec ST7789 fork reporting recovered `v2.0.02` identity, not the official stock binary. The stock image remains a verified recovery baseline only.

The current custom app and decoded LittleFS asset partition are proven byte-for-byte. The old decoded-asset blocker was `TFT_eTouch::init()` formatting the SPIFFS/data partition; the source now also avoids the later runtime `LittleFS.begin(true)` format fallbacks, and the decoded LittleFS region still verifies after the latest app reflash.

The remaining gap is direct physical and runtime proof, now including a user-observed physical panel visual failure: physical screen/LED/touch classification, hard USB power-cycle, Wi-Fi/pool setup, mining-active evidence, speaker/SD/API checks, dashboard checks, and restore proof. Touch is specifically still unresolved: serial diagnostics continue to show no touch press signal. Phase 8 must not start from the current evidence.

## Requirement Verdicts

| Requirement | Verdict | Evidence Needed | Current Evidence |
|-------------|---------|-----------------|------------------|
| VALID-01 | PARTIAL | Fresh 4 MB backup before write, readback evidence, hard USB power-cycle | Latest backup `cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin` is 4,194,304 bytes with SHA-256 `b6c6c58ca755b84bcc27e5cc00c1fca0a6ab9411ae5f378ac3a28f192277f2ed`; current app readback matches built app; hard physical power-cycle pending |
| VALID-02 | PARTIAL / FAILING UI | Repaired firmware flashed, serial-confirmed boot, visual parity checklist, mining active | Current custom decoded-spec firmware with fallback-pool, source-blocker, review-05 mining/config hardening fixes, and recovery portal CSS fix boots to config mode; current recovery web UI returns HTTP 200 for root/Info/WiFiManager pages and no longer shows the prior narrow clipping; decoded LittleFS was preserved by app-only write; custom JSON API returns 404 in portal mode; physical panel visual parity is failed by user observation but unclassified; touch and mining evidence pending |
| VALID-03 | PARTIAL | Restore path documented and backup integrity proven for the fresh backup | Latest full backup size and checksum are preserved; restore command is known; full backup restore round-trip not run |

**Score:** 0/3 VALID requirements fully verified; 3/3 have partial evidence, but all still require remaining Phase 7 gates before closure.

## What Is Proven

| Item | Status | Evidence |
|------|--------|----------|
| 07-02 software repair plan | PASS | `07-02-SUMMARY.md` records H1-H5 plus follow-up SDCard/RotateScreen repairs |
| Current code-review blocker fixes | PASS | `07-CURRENT-CODE-REVIEW.md`: CR-01/CR-02/WR-01 fixed in source; `ESP32-2432S028R` build succeeded |
| Current firmware review 02/03/04/05 fixes | PASS/LIVE | `07-CURRENT-FIRMWARE-REVIEW-02.md` through `07-CURRENT-FIRMWARE-REVIEW-05.md`: CR-01 raw flash read/log leak fixed, config portal AP serial log redacted, WiFiManager debug disabled, `.ss` header/staging validation hardened, mining target validation fixed, pool endpoint parsing made strict/transactional, mining-stat NVS handling committed/checked, and `ESP32-2432S028R` build succeeded. These fixes are included in the latest webfix2 app flash/readback. |
| v2.0.02 identity source patch | PASS | `src/version.h` now defines `CURRENT_VERSION` as `v2.0.02`; artifact strings include `v2.0.02` and no `V1.8.3` |
| v2.0.02 identity build | PASS | `pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R` succeeded; app SHA-256 `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1` |
| v2.0.02 identity app reflash | PASS | Fresh backup `hardware/backups/cyd-backup-20260619-001919-pre-v2002-identity-app-reflash.bin`; app-only `write_flash 0x10000` reported hash verified |
| v2.0.02 identity app verify | PASS | 2026-06-19T00:20Z `verify_flash --diff no 0x10000 firmware.bin` passed with digest matched |
| Decoded LittleFS after identity app flash | PASS | 2026-06-19T00:20Z `verify_flash --diff no 0x310000 /tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` passed with digest matched |
| Post-identity serial boot/runtime | PASS/PARTIAL | `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`: clean boot, SD mounted, recovery AP starts; no panic/boot-loop markers |
| Post-identity touch recheck | FAIL/PARTIAL | `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`: repeated no-contact diagnostics, `directPressed=0` / `libPressed=0` |
| No-flash touch software review | PASS/WARNING | `07-TOUCH-SOFTWARE-REVIEW.md`: 0 critical findings, 1 warning-level legacy `doLedStuff()` second-reader defect; source patched and rebuilt, but unflashed |
| Cleanup build before fallback-pool work | PASS/SUPERSEDED | `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R`: SUCCESS, local firmware.bin 2,827,504 bytes, SHA-256 `32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b`; superseded first by the fallback-pool app flash/readback, then by review-05, and finally by the current webfix2 app flash/readback |
| Fallback-pool source/build | PASS | `ESP32-2432S028R` build succeeded after adding secondary-pool persistence, portal/serial/SD/API fields, failover, and <=10s primary switch-back probing; app SHA-256 `49d318889eaef3f8e9d80a37b3e4a256ede586252475ff35277979521b0bc3a9` |
| Fallback-pool app reflash | PASS | Fresh backup `hardware/backups/cyd-backup-20260618-235938-pre-fallback-pool-flash.bin`; app-only `write_flash 0x10000` reported hash verified |
| Fallback-pool app readback | PASS | `/tmp/nmminer-app-readback-20260619-000220.bin` SHA-256 `49d318889eaef3f8e9d80a37b3e4a256ede586252475ff35277979521b0bc3a9` matched the built app |
| Post-fallback serial boot/runtime | PASS/PARTIAL | Filtered 22s serial check after the app flash read 46 lines, found 0 panic/abort lines, mounted SD, imported SD app settings without usable Wi-Fi credentials, and entered config mode; raw serial was not stored in tracked files |
| Post-forensics source-blocker source/build | PASS/SUPERSEDED | `ESP32-2432S028R` build succeeded after fixing merkle-root termination, stratum serial redaction, SD/API sensitive-route gating, mutation-unlock lifetime, legacy config/WiFiManager migration, SD config scrubbing, port validation, and `NM_hashing`; app SHA-256 `67a158a063923c19cb762f433c1209e582eae03d4d7de3ecb32fc67e0ae193b6`; superseded by review-05 app flash/readback |
| Post-forensics source-blocker app reflash | PASS/SUPERSEDED | Fresh backup `hardware/backups/cyd-backup-20260619-042756-pre-source-blocker-fix-app-flash.bin`; app-only `write_flash 0x10000` reported hash verified; superseded by review-05 app flash/readback |
| Post-forensics source-blocker app readback | PASS/SUPERSEDED | `/tmp/nmminer-app-readback-20260619-043113-source-blocker-fix.bin` SHA-256 `67a158a063923c19cb762f433c1209e582eae03d4d7de3ecb32fc67e0ae193b6` matched the built app; superseded by review-05 app readback |
| Post-forensics source-blocker serial boot/runtime | PASS/PARTIAL | Filtered/sanitized 24s serial check after the app flash read 49 lines, found 0 panic/abort/backtrace/brownout lines, entered config mode at `192.168.4.1`, and still showed no touch contact (`directPressed=0` / `libPressed=0`); raw serial was not stored in tracked files |
| Recovery portal CSS source/build | PASS/LIVE | `ESP32-2432S028R` build succeeded after adding custom WiFiManager narrow-viewport CSS; app SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5` |
| Latest no-device source/build recheck | PASS/LIVE | `uvx platformio run -e ESP32-2432S028R` succeeded without device write; rebuilt app stayed 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5` |
| Recovery portal CSS app reflash | PASS/LIVE | Fresh backup `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin`; app-only `write_flash 0x10000` reported hash verified |
| Recovery portal CSS app readback | PASS/LIVE | `/tmp/nmminer-app-readback-20260619-053633-web-ui-fix2.bin` SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5` matched the built app with `cmp` |
| Recovery portal CSS serial boot/runtime | PASS/PARTIAL | Earlier filtered/sanitized post-flash smoke ran 35s, produced twelve lines, and found 0 panic lines; the later no-write reset-only 45s touch window read 49 lines, found 0 panic lines, and counted 31 touch diagnostics with 0 pressed and 0 IRQ-low observations; raw serial was not stored in tracked files |
| Latest passive serial continuation | PASS/PARTIAL | `/tmp/p7-serial-passive-inline-20260619-173344.log`: 5,177 bytes / 51 lines, SHA-256 `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`; recovery AP still active at `192.168.4.1`; 23 touch diagnostics, 0 pressed observations, 0 IRQ-low observations |
| Review-05 mining/config source/build | PASS/SUPERSEDED | `ESP32-2432S028R` build succeeded after fixing target comparison, compact target validation/bounds checks, strict transactional pool endpoint parsing/API rejection, NVS mining-stat commit/error handling, and subscribe password buffer width; app SHA-256 `d31c5e551940e188fd37fac348b5f1ccbdeb06948047dde73d31e25896c980ee`; superseded by the recovery portal CSS app flash/readback |
| Review-05 app reflash | PASS/SUPERSEDED | Fresh backup `hardware/backups/cyd-backup-20260619-045710-pre-review05-fix-app-flash.bin`; app-only `write_flash 0x10000` reported hash verified; superseded by the recovery portal CSS app flash/readback |
| Review-05 app readback | PASS/SUPERSEDED | `/tmp/nmminer-app-readback-20260619-050100-review05-fix.bin` SHA-256 `d31c5e551940e188fd37fac348b5f1ccbdeb06948047dde73d31e25896c980ee` matched the built app with `cmp`; superseded by the recovery portal CSS app readback |
| Review-05 serial boot/runtime | PASS/PARTIAL/SUPERSEDED | Filtered/sanitized 26s serial check after the app flash read 51 lines, found 0 panic/abort/backtrace/brownout lines, entered config mode at `192.168.4.1`, and still showed no touch contact; raw serial was not stored in tracked files |
| Earlier read-only serial continuation | PASS/PARTIAL | `/tmp/p7-serial-readonly-continuation-20260619-020602.log`: clean boot to recovery AP, SHA-256 `ae240cbed25eb7588dfe05f69615401cc83524c992632ffbab0d48cd03a3a045`; touch still no-contact |
| Current app-only reflash | PASS | Fresh backup `hardware/backups/cyd-backup-20260618-231853-pre-current-code-review-fix-reflash.bin`; `/tmp/p7-flash-current-code-review-fix-app-20260618-232007.log` reports hash verified |
| Current app verify | PASS | `/tmp/p7-verify-current-code-review-fix-app-20260618-232048.log`: `0x10000` digest matched |
| Decoded LittleFS preserved after current app reflash | PASS | `/tmp/p7-verify-decoded-littlefs-after-current-app-reflash-20260618-232100.log`: `0x310000` digest matched |
| User-report custom app reverify | PASS | `/tmp/p7-verify-current-custom-app-after-user-report-20260618-193918.log`: `0x10000` digest matched after the user reported red LED/AP popup/touch failure |
| User-report decoded LittleFS reverify | PASS | `/tmp/p7-verify-decoded-littlefs-after-user-report-20260618-193918.log`: `0x310000` digest matched after the user report |
| User-report serial recheck | FAIL/PARTIAL | `/tmp/p7-serial-after-user-report-verify-20260618-193931.log`: recovery AP starts, but touch remains `directPressed=0` / `libPressed=0` |
| 19:58 live custom app reverify | PASS/SUPERSEDED | `/tmp/p7-verify-live-current-custom-app-20260618-195813.log`: `0x10000` digest matched against the then-live decoded-spec ST7789 app, 2,826,704 bytes; superseded by the webfix2 app flash/readback |
| 19:58 live decoded LittleFS reverify | PASS | `/tmp/p7-verify-live-current-decoded-littlefs-20260618-195813.log`: `0x310000` digest matched against `/tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin`, 917,504 bytes |
| 19:58 live serial touch recheck | FAIL/PARTIAL | `/tmp/p7-serial-live-current-after-verify-20260618-195813.log`: repeated no-contact diagnostics; recovery/AP behavior is expected, but touch remains below-UI failure |
| Current recovery web UI | PASS/PARTIAL | Host joined `nmap-2.4g`; `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200; screenshots captured under `/tmp/p7-webfix2-ui/`. The prior 320 px root and 390 px Configure WiFi horizontal clipping is fixed for the browser recovery portal. |
| Physical ST7789/LVGL panel visual parity | FAIL/UNCLASSIFIED | User physically observed the live display on 2026-06-19 and reported it looks wrong and not like expected. No photo/detail captured yet. Top suspects from read-only audit: expected recovery/config page versus miner page, ST7789 RGB/BGR/inversion/byte-swap flags, rotation, incomplete LVGL screen parity, or LVGL flush/SPI artifacts. |
| Custom JSON API in recovery portal mode | OPEN | Safe GET-only probe `/tmp/p7-webfix2-ap-get-probe.tsv`: `/`, `/info`, `/wifi`, and `/0wifi` returned 200, while `/api/status`, `/api/system/info`, and `/sd/list` returned 404. Portal-mode API validation remains unproven by design/current routing. |
| Custom no-touch-formatter build | PASS | `/tmp/p7-build-no-touch-spiffs-format-20260618-183904.log`: `ESP32-2432S028R` SUCCESS, payload 2827280 bytes |
| Board-driver gate | PASS | `ESP32-2432S028R` build switched to `-DST7789_DRIVER=1` before final flash |
| Fresh custom backup | PASS | `hardware/backups/cyd-backup-20260618-184025-pre-no-touch-spiffs-format-reflash.bin`, 4194304 bytes, ignored/not staged |
| Custom app flash/verify | PASS | `/tmp/p7-flash-no-touch-spiffs-format-app-*.log` and `/tmp/p7-verify-no-touch-spiffs-format-app-*.log`: bootloader, partitions, boot_app0, and firmware digest-matched |
| Decoded LittleFS flash | PASS | `/tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` flashed at `0x310000`; write-time hash verified |
| Decoded LittleFS preservation | PASS | Decoded LittleFS region verified before boot, after runtime boots, and after NVS clear with digest match |
| Clean recovery AP | PASS | `/tmp/p7-serial-clean-nvs-custom-decoded-20260618-185211.log`: no stored config, `nmap-2.4g`, `192.168.4.1`, config portal started |
| Touch diagnostics | FAIL/PENDING | Latest serial diagnostics still show no press signal (`directPressed=0`, `libPressed=0`) |
| Official stock binary baseline | PASS | Historical recovery baseline: official ST7789 bootloader, partitions, boot_app0, app, and LittleFS were flashed and verified before the later custom reflash |
| Official web UI screenshots | HISTORICAL/PARTIAL | `/tmp/p7-stock-ui/` captured root and Wi-Fi pages for stock recovery; these do not validate the current custom LVGL on-device UI |
| SD password write guard | PASS | Static check found no `WifiPW` or `JSON_KEY_PASW` reference inside `writeConfigToSD()` |
| Legacy SDCard fallback guard | PASS | Static check found no `SDCrd`, `SD2nvMemory`, `loadConfigFile`, or `terminate` path in `wManager.cpp` |
| Staged backup/binary guard | PASS | Static staged-file check found no staged backup or `.bin` artifact |

## Remaining Gates

1. Physical screen/touch classification: current user-observed wrong panel state needs no-secret photo/video or checklist, including visible screen/text, colors, rotation, clipping/overlap, stale regions/tearing, LED state, and touch behavior under current custom firmware.
2. Hard USB unplug/replug and post-power-cycle boot check.
3. Decide and test the intended custom JSON API exposure model: portal-only WiFiManager routes currently return 200, but custom JSON API routes return 404 in recovery portal mode.
4. Wi-Fi/pool configuration on a 2.4 GHz SSID, followed by mining-active proof.
5. Speaker, SD/API, and dashboard checks.
6. Restore proof or explicit restore deferral.

## Gaps Summary

The current custom firmware is flashed, the decoded asset preservation blockers are fixed, and the recovery web portal works without the prior narrow-layout clipping. Keep Phase 7 open because the physical ST7789/LVGL panel now has a user-observed visual parity failure, touch has no raw press signal, and hard power-cycle proof, Wi-Fi/pool/mining proof, speaker/SD/API/dashboard checks, and restore proof remain missing.

---

_Verified: 2026-06-19T05:45:21Z_
_Verifier: Codex (GSD verification pass)_
