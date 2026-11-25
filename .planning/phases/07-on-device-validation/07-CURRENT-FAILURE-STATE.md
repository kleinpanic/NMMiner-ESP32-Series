# Phase 7 Current Failure State

**Generated:** 2026-06-19T00:29:04Z
**Updated:** 2026-06-19T18:03:38Z
**Scope:** Operator-facing truth table for the current NMMiner ESP32 Phase 7 recovery state.
**Status:** Phase 7 remains open. This is not a completion summary.

> Route note, 2026-06-19: this file describes the current live device/fork
> failure and recovery context. It is not the active implementation route. The
> active GSD route is official NMMiner `v2.0.02` ST7789 binary
> decompilation/custom-edit via `07-BINARY-BASELINE-ROUTE.md`,
> `07-BINARY-DECOMP-TASKS.md`, and `07-BINARY-EDIT-TARGET-MAP.md`. Use the
> physical panel/touch steps below only for restore/reversibility or live-device
> recovery decisions.

## Current Truth

- The live device is the custom decoded-spec `ESP32-2432S028R` ST7789 fork in WiFiManager recovery AP mode, now reporting the recovered NMMiner `v2.0.02` identity.
- The live device is not the proprietary official stock binary, not proven to be mining, and not proven to have working physical touch.
- Official stock `v2.0.02` is the verified recovery baseline and source of the decoded specification; the current live firmware is a rebuilt custom fork that intentionally reports the same recovered version string.
- The current live app includes the 2026-06-19 fallback-pool source fix, the post-forensics source-blocker fixes, the later review-05 mining/config hardening fixes, and the WiFiManager recovery portal CSS fix. It was flashed app-only at `0x10000` after a fresh full 4 MB backup and independently read back with a matching SHA-256 digest.
- The AP/captive-portal popup is expected in WiFiManager config mode. Join `nmap-2.4g` and browse to `192.168.4.1`.
- The red LED is expected in `NM_Connecting` / `NM_waitingConfig`; it is not by itself a flash-identity failure.
- The physical ST7789/LVGL panel is now user-observed as visibly wrong and not matching the expected NMMiner UI. No photo, text sample, color/orientation detail, or visible screen classification has been captured yet, so the exact display-failure class remains untriaged.

## Evidence That Is Strong

| Claim | Evidence |
|-------|----------|
| Current custom app artifact builds | `uvx platformio run -e ESP32-2432S028R` passed again on 2026-06-19 after the WiFiManager recovery portal CSS fix. App payload: 2,840,400 bytes. |
| Live app artifact identity | The app flashed at 2026-06-19T05:36Z is 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`; strings still include the recovered `v2.0.02` identity and custom recovery/config behavior. |
| Fresh backup before latest write | `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin` is 4,194,304 bytes, SHA-256 `b6c6c58ca755b84bcc27e5cc00c1fca0a6ab9411ae5f378ac3a28f192277f2ed`. |
| Live app matched that artifact | 2026-06-19T05:39Z independent app readback from `0x10000` produced `/tmp/nmminer-app-readback-20260619-053633-web-ui-fix2.bin`, 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, matching the built app byte-for-byte. |
| Decoded LittleFS is preserved | 2026-06-19T00:20Z `verify_flash --diff no 0x310000 /tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` passed with `-- verify OK (digest matched)`. |
| Post-identity runtime boot is clean | `/tmp/p7-serial-post-v2002-identity-20260619-002803.log` captured 2026-06-19T00:28:03Z..00:29:04Z, SHA-256 `40fa9402bc1cd51b9dc0875a80e726de5c17ef491ad6d9aa14257f5dbd765659`; no panic/boot-loop markers found. |
| Earlier read-only serial continuation is clean | `/tmp/p7-serial-readonly-continuation-20260619-020602.log` captured 2026-06-19T02:06:02Z..02:06:22Z without flash/write/erase/settings mutation, SHA-256 `ae240cbed25eb7588dfe05f69615401cc83524c992632ffbab0d48cd03a3a045`; it shows `POWERON_RESET`, recovery AP `nmap-2.4g` at `192.168.4.1`, and no `Guru Meditation` / `PANIC` markers. |
| Post-webfix2-flash boot is clean | The earlier filtered/sanitized post-flash smoke ran 35s, produced twelve lines, and found 0 panic lines. A later no-write reset touch window ran 45s, produced 49 lines, and also found 0 panic lines. The raw serial text was not stored in tracked files. |
| Latest no-write touch windows still no-contact | The reset-only 45s touch window counted 31 touch diagnostics, 0 pressed observations, and 0 IRQ-low observations. A later 2026-06-19T17:33Z passive serial window captured `/tmp/p7-serial-passive-inline-20260619-173344.log`, 5,177 bytes / 51 lines, SHA-256 `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`, with 23 touch diagnostics, 0 pressed observations, and 0 IRQ-low observations. Prior touch review confirmed the active touch path is running and found no source-level touch fix justified before physical controller/FPC/PENIRQ/SPI probing. |
| Recovery AP still starts after identity reflash | The same log shows `StartAP with SSID: nmap-2.4g`, `AP IP address: 192.168.4.1`, and `Entered Configuration Mode`. |
| Current recovery web portal is reachable | After the webfix2 app flash, the host joined `nmap-2.4g`; `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200 in AP mode. Current screenshots are under `/tmp/p7-webfix2-ui/`. |
| Current recovery web portal narrow clipping fixed | The prior 320 px root and 390 px Configure WiFi right-edge clipping was fixed in the flashed app with custom WiFiManager head CSS. Recaptured `/tmp/p7-webfix2-ui/root-320x568.png`, `root-390x844.png`, `root-1440x900.png`, `wifi-320x568.png`, `wifi-390x844.png`, and `info-390x844.png` render without the earlier cutoff. |
| Physical panel visual parity fails | The user physically observed the live miner display on 2026-06-19 and reported that it looks wrong and nothing like expected. `07-UI-REVIEW.md` now scores Phase 7 UI at 8/24 and treats physical ST7789/LVGL parity as a current failed gate. No photo/detail is captured yet, so this proves failure, not root cause. |
| Touch path is active but sees no contact | `/tmp/p7-serial-readonly-continuation-20260619-020602.log` and `/tmp/p7-serial-post-v2002-identity-20260619-002803.log` repeat `irq=1`, `miso=0`, `directX=0`, `directY=4095`, `directPressed=0`, and `libPressed=0`. |
| Current touch bus contract is no-VSPI, not Phase-4 SoftSPI | `pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R -t envdump` shows `TOUCH_ENABLE` + `NM_TOUCH_NO_VSPI` and no `USE_HSPI_PORT` / `CONFIG_TFT_eSPI_ESPIDF`; current source calls `nm_touch_poll_gesture()` as the sole CYD touch path and gates out legacy `TouchHandler`. |
| No remaining critical software touch fix found | `07-TOUCH-SOFTWARE-REVIEW.md` found 0 critical findings and 1 warning. WR-01 was a real second-reader defect in legacy `doLedStuff()`; it is patched in source and included in the live app. |
| Latest source blockers fixed and flashed | The previous UAT mismatch for primary/fallback pool support is resolved in source and in the live app. The post-forensics pass fixed the merkle-root buffer terminator, stratum serial redaction, SD/API sensitive-route gating, mutation-unlock lifetime, legacy config/WiFiManager credential migration, SD config scrubbing after import, pool-port validation, and `NM_hashing` transition. The review-05 pass fixed mining target comparison, compact-target bounds checks, strict pool endpoint parsing, API pool-port rejection, NVS stat commit/error handling, and the stratum password buffer. Runtime mining UAT is still pending. |

The custom rebuilt app now intentionally reports `v2.0.02` for stock-parity identity. Use flash offset, artifact hash, and source/build provenance to distinguish it from the proprietary official stock binary.

## What Claude/Past Work Got Wrong

- Phases 3-6 were treated as complete from compile success and code review, while the actual on-device rendering and runtime behavior were deferred to Phase 7.
- Phase 7 exposed the real failures: boot-loop stack crash, stock-screen clobbering, config-mode crash, upside-down render, white-screen history, Wi-Fi config regression, API exposure mismatch, and touch no-contact.
- Several historical docs mixed stock baseline, old fork mining, and later custom decoded-spec states. `07-VALIDATION.md`, `07-VERIFICATION.md`, this file, and `STATE.md` are now the current authority.

## Current Failure Classification

| Area | Current Classification |
|------|------------------------|
| Flash identity | Proven custom decoded-spec ST7789 app reporting `v2.0.02`, plus decoded LittleFS assets; not the official stock binary. |
| Web recovery UI | Proven reachable for the current WiFiManager root, Info, and WiFi forms. The latest recaptured 320/390/desktop browser screenshots no longer show the prior horizontal clipping. |
| Physical LVGL panel UI | Failed by user observation, details not captured. Browser screenshots do not validate the CYD panel. Top suspects are recovery/config screen state versus miner screen, ST7789 RGB/BGR/inversion/byte-swap flags, rotation, incomplete rebuilt LVGL screen parity, or LVGL flush/SPI artifacts. |
| Touch | Failing below UI/LVGL/calibration. The active HSPI/direct-probe path reads no raw contact. No-flash software reviews fixed source-side ownership/hardening issues for the next build, but those do not explain the current idle/no-pressure evidence. |
| Wi-Fi/mining | Not proven in the current custom state; device is in recovery AP mode. Fallback-pool source support, post-forensics runtime fixes, and review-05 mining/config hardening are now live, but no 2.4 GHz Wi-Fi/pool runtime or accepted-share proof has been captured on this build. |
| Custom JSON API | Open product decision: routes return 404 in recovery portal mode. |
| Restore | Restore command and backups exist; full restore round-trip has not been run. |

## Requirement Gate Status

| Gate | Status | Reason |
|------|--------|--------|
| VALID-01 | Partial | Fresh backups and verify_flash evidence exist, and the latest serial capture shows `POWERON_RESET`, but operator-confirmed hard physical USB unplug/replug proof is still missing. |
| VALID-02 | Partial / failed physical UI | Current custom firmware boots to recovery AP and web root works, but the physical panel is now user-observed as visually wrong; touch, Wi-Fi/pool join, mining, speaker, SD/API, and dashboard proof are also missing. |
| VALID-03 | Partial | Restore path is documented and backup integrity is preserved, but the full restore round-trip was not run. |

Do not create `07-01-SUMMARY.md` unless these gates close or the plan is explicitly abandoned with a failure summary.

## Prioritized Human UAT Plan

The GSD UAT audit currently has 8 outstanding human items: 4 from Phase 4 and 4 from Phase 6. The detailed queue is in `07-UAT-AUDIT.md`: all 8 items are active but prerequisite-gated, 0 are stale, and 0 need update after the fallback-pool, post-forensics, review-05 source fixes, and web recovery CSS fix were flashed. Run the active items in this order so each later test only happens after the prerequisite evidence exists.

| Priority | Group | Covers | Prerequisite | Pass Evidence |
|----------|-------|--------|--------------|---------------|
| 1 | Physical baseline + UI failure classification | Phase 7 VALID-01/02 | Device in current custom recovery AP state; no firmware write | Hard USB unplug/replug, clean return to `nmap-2.4g` / `192.168.4.1`, no panic/boot-loop, no-secret note/photo or written checklist of the wrong panel content, colors, rotation, clipping/overlap, visible text, and LED state |
| 2 | Touch electrical probe | Phase 4 tap/hold/wake blockers | Baseline power-cycle done | `07-TOUCH-PROBE-RUNBOOK.md` evidence: touch controller/FPC present or variant identified, PENIRQ/GPIO36 behavior under firm press, CS/CLK/MOSI/MISO activity or absence recorded |
| 3 | Touch UI behavior | Phase 4 tap navigation, 10s touch hold, dim tap wake | Touch electrical probe proves a real press signal reaches the controller path | Single tap advances carousel and persists LastPage, 10s hold opens portal without wiping config, dim tap wakes without page change |
| 4 | Boot button behavior | Phase 4 boot button item | Physical panel/serial observation available | GPIO0 double-click advances page, long-press opens config portal, single-click wakes screen |
| 5 | Wi-Fi and mining runtime | Phase 7 VALID-02 plus Phase 6 mining prerequisite | Physical baseline accepted; configure only a 2.4 GHz SSID; do not record secrets | Device joins LAN, pool connects, hashrate nonzero, at least one accepted share, LAN/API/dashboard behavior recorded without sensitive values |
| 6 | Custom hardware outputs | Phase 6 beep, LED, SD CSV | Mining runtime active | Accepted-share beep heard, disconnect alarm distinguishable if tested, blue/green/red LED states observed in the expected mining/share/disconnect states, `/sd/mining_stats.csv` has stats and `event=share` rows |
| 7 | Restore decision | VALID-03 | Current backup integrity preserved | Either opt-in full restore round-trip succeeds, or restore is explicitly deferred with the latest backup filename, size, checksum sidecar, and restore command preserved |

Do not treat the Phase 6 LED item as passed from the current red LED alone. Red in recovery/config/connecting mode is expected, but the Phase 6 LED UAT requires mining/share/disconnect behavior.

## Next Correct Action For Live-Fork Recovery

If the current custom fork must be recovered or physically revalidated, run the
physical panel/touch path before more fork firmware tuning:

1. While the AP portal is active, capture a no-secret photo/video or written checklist of the physical panel: visible screen name/text, colors, rotation, clipping/overlap, stale rectangles/tearing, and LED state.
2. Decide whether the panel is merely showing the expected recovery/config LVGL page or whether the rendered page itself is wrong.
3. If the rendered page is wrong, triage one display variable at a time after a fresh backup: ST7789 RGB/BGR order, inversion, byte-swap behavior, then rotation.
4. Inspect or photograph the touch controller/FPC area; confirm the controller is populated and the FPC is seated.
5. Probe PENIRQ/GPIO36 during a firm press.
6. Confirm CS=33, CLK=25, MOSI=32, and MISO=39 activity at the touch controller during diagnostic polling.
7. Only after physical display/touch evidence exists, decide whether a diagnostic firmware patch is justified.

If any firmware write is chosen later, take another fresh full 4 MB backup first.

## Do Not Do

- Do not flash the ILI9341 build unless the hardware is re-identified.
- Do not keep changing touch thresholds, rotation, debounce, or LVGL gesture routing while raw contact remains absent.
- Do not treat browser recovery portal screenshots as proof that the physical ST7789/LVGL panel matches the recovered NMMiner UI.
- Do not perform any future firmware write without another fresh full 4 MB backup first.
- Do not treat `v2.0.02` text alone as proof of the official stock binary; current custom rebuilt firmware also reports that version for parity.
- Do not put Wi-Fi passwords, wallet values, device licenses, flash backups, or sensitive serial logs into tracked files.
