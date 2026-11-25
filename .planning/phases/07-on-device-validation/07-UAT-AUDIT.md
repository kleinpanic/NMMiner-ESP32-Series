---
phase: 07-on-device-validation
generated: 2026-06-19T04:33:46Z
source: "$gsd-audit-uat"
audit_command: "gsd-sdk query audit-uat --raw"
total_items: 8
active_items: 8
needs_update_items: 0
stale_items: 0
status: active_prerequisite_gated
---

# Phase 7 UAT Audit

This is a UAT queue audit, not a completion report. The original audit was read-only. A later 2026-06-19 continuation implemented the primary/fallback-pool gap in the custom decoded-spec source, a post-forensics pass fixed source blockers before runtime UAT, review-05 hardened mining/config edge cases, and the latest continuation fixed the WiFiManager recovery portal narrow-layout clipping. The latest app was built, backed up-before-write, flashed at `0x10000`, and independently read back byte-for-byte.

The latest physical-panel observation changes the Phase 7 UI gate from merely unproven to failed/unclassified: the user observed the live miner display and reported it looks wrong and nothing like expected. No photo/detail is captured yet, so this audit treats physical UI parity as failed pending classification, while keeping the browser recovery portal evidence as separate and still passing for its own narrow scope.

## Current Result

`$gsd-audit-uat` reports 8 outstanding human UAT items:

| Source | Count | Status |
|--------|------:|--------|
| Phase 04 touch/button verification | 4 | Active, prerequisite-gated |
| Phase 06 custom hardware/runtime verification | 3 | Active, prerequisite-gated |
| Phase 06 mining/fallback-pool verification | 1 | Active, prerequisite-gated |

No stale UAT items were found. All eight items now describe real behavior that should be tested on the current custom decoded-spec firmware after the prerequisite evidence exists. The previous primary/fallback source mismatch, post-forensics source blockers, review-05 mining/config blockers, and recovery portal clipping are resolved in source and live app flash evidence, but runtime mining UAT is still required before `HW-04` can pass.

Current webfix2 recovery web UI evidence is now captured: the host joined `nmap-2.4g`; `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200; screenshots are under `/tmp/p7-webfix2-ui/`. This does not close physical panel/touch UAT. It closes the earlier browser recovery-portal clipping finding for the current flashed app, but the physical ST7789/LVGL panel now fails visual parity by user observation.

The latest no-write passive serial window is `/tmp/p7-serial-passive-inline-20260619-173344.log`, summarized as 51 lines, SHA-256 `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`, 23 touch diagnostics, 0 pressed observations, 0 IRQ-low observations, 1 reset marker, and 2 AP/config markers. Future touch-probe captures should be summarized with `python3 scripts/phase7_serial_touch_summary.py /tmp/<serial-log>.log`, copying only derived counts into tracked docs.

## 2026-06-19 Recovery Portal CSS Fix And Flash

- Fixed the WiFiManager recovery portal narrow-layout clipping by injecting custom head CSS from `src/wManager.cpp`.
- Build passed with `uvx platformio run -e ESP32-2432S028R`.
- Built app: 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.
- Factory image: 2,908,160 bytes, SHA-256 `ec320e3e61edad45b405ba172ae7804d53463551676bfdfe343834ee2b50b5a4`.
- Fresh pre-write backups: `hardware/backups/cyd-backup-20260619-052605-pre-web-ui-fix-app-flash.bin`, 4,194,304 bytes, SHA-256 `9fab4b71f3e28f142a193a0d5464effd4575c62a7ce2438a1a2ed2b4c21e7a25`; and `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin`, 4,194,304 bytes, SHA-256 `b6c6c58ca755b84bcc27e5cc00c1fca0a6ab9411ae5f378ac3a28f192277f2ed`.
- Final flash performed: app-only write to `0x10000`.
- Independent readback: `/tmp/nmminer-app-readback-20260619-053633-web-ui-fix2.bin`, 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, matching the built app.
- The earlier filtered/sanitized post-flash smoke ran 35s, produced twelve lines, found 0 panic lines, and still showed no touch contact. A later no-write reset-only 45s touch window read 49 lines, found 0 panic lines, and counted 31 touch diagnostics with 0 pressed and 0 IRQ-low observations. No raw serial log was written into tracked files.
- AP browser recheck: `/`, `/info`, `/wifi`, and `/0wifi` returned 200; `/api/status`, `/api/system/info`, and `/sd/list` returned 404 in portal mode; 320/390/desktop screenshots under `/tmp/p7-webfix2-ui/` no longer show the previous right-edge clipping.

## 2026-06-19 Review-05 Mining/Config Hardening And Flash

- Fixed the review-05 blockers/warnings before runtime UAT: target/work comparison, compact `nbits` validation, target-buffer bounds checks, strict transactional pool endpoint parsing/rejection, NVS mining-stat save/restore checks, `nvs_commit()`, and stratum subscribe password buffer width.
- Build passed with `uvx platformio run -e ESP32-2432S028R`.
- Built app: 2,839,504 bytes, SHA-256 `d31c5e551940e188fd37fac348b5f1ccbdeb06948047dde73d31e25896c980ee`.
- Factory image: 2,908,160 bytes, SHA-256 `efac2c55a377ea966d7f818f9db6a9fd6e44d6e46faa1c04efc1bf6165d5e383`.
- Fresh pre-write backup: `hardware/backups/cyd-backup-20260619-045710-pre-review05-fix-app-flash.bin`, 4,194,304 bytes, SHA-256 `99541243892212422fa7071137deb8e00183d3c18dd23de706380dafbd67b5b8`.
- Flash performed: app-only write to `0x10000`.
- Independent readback: `/tmp/nmminer-app-readback-20260619-050100-review05-fix.bin`, 2,839,504 bytes, SHA-256 `d31c5e551940e188fd37fac348b5f1ccbdeb06948047dde73d31e25896c980ee`, matching the built app.
- Filtered/sanitized post-flash serial check read 51 lines, found 0 panic/abort/backtrace/brownout lines, imported SD app settings without usable Wi-Fi credentials, entered config mode at `192.168.4.1`, and still showed no touch contact. No raw serial log was written into tracked files.

## 2026-06-19 Post-Forensics Source-Blocker Fix And Flash

Superseded by the later review-05 app flash above, but kept as historical evidence for the source-blocker implementation.

- Fixed the post-forensics blockers before runtime UAT: merkle-root terminator, stratum serial redaction, SD/API sensitive-route gating, mutation-unlock lifetime, legacy config/WiFiManager credential migration, SD config scrubbing, pool-port validation, and `NM_hashing` transition.
- Build passed with `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R`.
- Built app: 2,837,840 bytes, SHA-256 `67a158a063923c19cb762f433c1209e582eae03d4d7de3ecb32fc67e0ae193b6`.
- Factory image: 2,904,064 bytes, SHA-256 `39b7b527ba256ce43fece6bc505641f98d36c914ac79a90ecff306849118d67b`.
- Fresh pre-write backup: `hardware/backups/cyd-backup-20260619-042756-pre-source-blocker-fix-app-flash.bin`, 4,194,304 bytes, SHA-256 `d3c97e45dad2ebb98d00b089c431be7f0affea4af9f3cee3ed75da3680bcdbf9`.
- Flash performed: app-only write to `0x10000`.
- Independent readback: `/tmp/nmminer-app-readback-20260619-043113-source-blocker-fix.bin`, 2,837,840 bytes, SHA-256 `67a158a063923c19cb762f433c1209e582eae03d4d7de3ecb32fc67e0ae193b6`, matching the built app.
- Filtered/sanitized post-flash serial check read 49 lines, found 0 panic/abort/backtrace/brownout lines, mounted SD, imported SD app settings without usable Wi-Fi credentials, entered config mode, and still showed no touch contact (`directPressed=0` / `libPressed=0`). No raw serial log was written into tracked files.

## 2026-06-19 Fallback-Pool Source Fix And Flash

Superseded by the later source-blocker-fix and review-05 app flashes above, but kept as historical evidence for the fallback-pool implementation.

- Implemented secondary/fallback pool persistence, portal fields, serial/SD JSON keys, API GET/POST fields, and mining failover/switch-back logic in the custom `ESP32-2432S028R` firmware.
- Build passed with `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R`.
- Built app: 2,834,128 bytes, SHA-256 `49d318889eaef3f8e9d80a37b3e4a256ede586252475ff35277979521b0bc3a9`.
- Factory image: 2,899,968 bytes, SHA-256 `c9ef9f8949902857c90ae3d90febb661ddccc856502fd983c43543c1dde22e3c`.
- Fresh pre-write backup: `hardware/backups/cyd-backup-20260618-235938-pre-fallback-pool-flash.bin`, 4,194,304 bytes, SHA-256 `11acf9261f0d3ef97c6295522027e8efdda58b3020210176f15634585007309c`.
- Flash performed: app-only write to `0x10000`.
- Independent readback: `/tmp/nmminer-app-readback-20260619-000220.bin`, 2,834,128 bytes, SHA-256 `49d318889eaef3f8e9d80a37b3e4a256ede586252475ff35277979521b0bc3a9`, matching the built app.
- Filtered post-flash serial check read 46 lines, found 0 panic/abort lines, mounted SD, imported SD app settings without usable Wi-Fi credentials, and entered config mode. No secrets were written into this artifact.

## Prerequisite Ladder

Run the UAT in this order. Each step exists to avoid turning a missing prerequisite into a misleading test failure.

| Priority | Gate | Unlocks | Required Evidence |
|----------|------|---------|-------------------|
| 1 | Physical baseline + UI failure classification | Phase 7 VALID-01/VALID-02 | Hard USB unplug/replug, clean return to current recovery state, no-secret note/photo or written checklist of the wrong physical panel: visible screen/text, colors, rotation, clipping/overlap, stale regions/tearing, and LED state |
| 2 | Touch electrical probe | Phase 4 touch UAT | Touch controller/FPC identified, PENIRQ/GPIO36 behavior under firm press recorded, CS=33/CLK=25/MOSI=32/MISO=39 activity or absence recorded |
| 3 | Touch UI behavior | Tap, 10s hold, dim wake | Single tap advances/persists page, 10s touch hold opens config portal non-destructively, dim tap wakes without page change |
| 4 | Boot button behavior | Phase 4 button UAT | GPIO0 single-click wakes, double-click advances page, long-press opens config portal |
| 5 | 2.4 GHz Wi-Fi/pool runtime | Phase 6 mining-dependent UAT | Device joins LAN without recording secrets, connects to pool, has nonzero hashrate, accepts at least one share |
| 6 | Custom outputs | Speaker, LED, SD CSV | Accepted-share beep, disconnect alarm if tested, mining/share/disconnect LED states, `/sd/mining_stats.csv` stats and `event=share` rows |
| 7 | Restore decision | VALID-03 | Full restore round-trip succeeds, or restore is explicitly deferred with latest backup/command preserved |

Do not run `$gsd-verify-work` for Phase 4 or Phase 6 as a normal pass/fail closure until the prerequisite ladder reaches the relevant item. It would currently report expected prerequisite failures, not fresh implementation evidence.

## Active Items

| Item | Status | Prerequisite | Source Evidence |
|------|--------|--------------|-----------------|
| Phase 4 single tap advances carousel and persists LastPage | Active, prerequisite-gated | Touch electrical probe proves a real press signal reaches the controller path | `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:391`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:233`, current no-contact evidence in `07-CURRENT-FAILURE-STATE.md` |
| Phase 4 10s touch hold opens WiFiManager AP without wiping config | Active, prerequisite-gated | Touch press signal and a clear non-portal starting state | `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:438`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:248` |
| Phase 4 tap while dimmed wakes backlight without page change | Active, prerequisite-gated | Touch press signal plus visible panel/backlight observation | `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:209`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:336` |
| Phase 4 boot button single/double/long behavior | Active, prerequisite-gated | Physical button and panel/serial observation | `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:119`, `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:258` |
| Phase 6 accepted-share beep and disconnect alarm | Active, prerequisite-gated | Wi-Fi + pool + accepted share; disconnect transition for alarm | `firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp:258`, `firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp:444`, `firmware/custom/NerdMiner-CYD-Custom/src/speaker.cpp:129`, `firmware/custom/NerdMiner-CYD-Custom/src/speaker.cpp:137` |
| Phase 6 RGB LED mining/share/disconnect states | Active, prerequisite-gated | Mining runtime active; do not count current recovery/config red LED | `firmware/custom/NerdMiner-CYD-Custom/src/nm_led.cpp:53`, `firmware/custom/NerdMiner-CYD-Custom/src/nm_led.cpp:76` |
| Phase 6 SD `/sd/mining_stats.csv` periodic and `event=share` rows | Active, prerequisite-gated | FAT SD mounted, mining runtime for at least one interval, accepted share for event row | `firmware/custom/NerdMiner-CYD-Custom/include/sdstore.h:18`, `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:288`, `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:323`, `firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp:219` |
| Phase 6 mining stays functional with primary + fallback pool and switch-back to primary within <=10s | Active, prerequisite-gated | 2.4 GHz Wi-Fi, primary and distinct fallback pool values, controlled primary outage/recovery, accepted-share proof | `firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`, `firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.cpp` |

## Needs Update

None at this time. The fallback-pool item is no longer a source/spec mismatch; it is an active runtime UAT item.

## Current Authority

- Live device state: `07-CURRENT-FAILURE-STATE.md`
- Physical touch probe path: `07-TOUCH-PROBE-RUNBOOK.md`
- Latest UI audit state: `07-UI-REVIEW.md` (8/24 after physical-panel parity failure)
- Current live app SHA-256: `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.
- Do not perform any future firmware write without another fresh full 4 MB backup first.
