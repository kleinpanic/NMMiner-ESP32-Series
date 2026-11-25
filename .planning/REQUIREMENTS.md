# Requirements: NMMiner ESP32 Series — Milestone v3.0 (NMMiner RE + Custom Integrations)

**Defined:** 2026-06-18
**Core Value:** The active route is the backed-up official NMMiner `v2.0.02`
`esp32-2432s028r-st7789` binary: decompile it, identify safe custom-edit seams,
and produce local-only patched artifacts without touching license/activation
paths, Wi-Fi credentials, wallet values, or redistributing closed firmware. The
fork rebuild work is now historical context and recovery/failure evidence, not
the primary implementation route.

> Prior milestone (v2.0.02 bring-up + NerdMiner fork) requirements & roadmap are archived under `.planning/archive/v2.0.02-fork-milestone/`. This file is scoped to v3.0 only.

## v1 Requirements

Requirements for milestone v3.0. Each maps to exactly one roadmap phase (see Traceability).

### Reverse-Engineering Spec (RE)

- [x] **RE-01**: A working Ghidra 11.3.1 + dynacylabs esp32 flash-loader v11.3.1 (prebuilt) workbench loads the stock image with DROM (`0x3f400020`) / IROM (`0x400d0020`) mapped; the self-built ESP-IDF v4.4.6 FIDB is attempted, and stripped SDK/Arduino-core symbols are recovered via the FIDB where it applies **OR** via a documented fallback (dynacylabs loader ROM/SVD annotations + the proven string→`l32r` xref pipeline) sufficient to decompile the screen / render / share-accept functions.
- [x] **RE-02**: The stock LVGL major version (v8 vs v9) is confirmed authoritatively in-decompiler, citing **at least two independent items from the acceptable-evidence set** (decoder/render call-shape; `lv_img_dsc_t`/`lv_font_t` descriptor layout; screen-constructor API signatures; DROM string family `LV_IMG_CF_*` v8 / `LV_COLOR_FORMAT_*` v9; build-era/version strings) with a recorded Confidence label, before any extractor is written.
- [x] **RE-03**: The located screen-construction functions (`Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`) plus the render / share-accept / screensaver code paths are decompiled into `hardware/recovered-assets/SPEC.md` (screen inventory, per-screen widget geometry, fonts/colors, market field set, screensaver render path).
- [x] **RE-04**: SPEC.md is markdown-only and never linked into firmware; all recovered closed-firmware assets remain gitignored (no redistribution).

### Official Binary Customization (BIN)

- [x] **BIN-01**: The official `v2.0.02` ST7789 baseline is provenance-locked:
  `hardware/backups/cyd-backup-20260617-172106.bin` byte-matches the local
  official bundle at bootloader, partition table, boot app, and app firmware
  offsets; `v2.0.03` is acknowledged as current public flasher metadata but is
  not silently substituted.
- [~] **BIN-02**: Ghidra/decompiler entries are normalized for all six stock
  screens and the runtime update paths that bind Miner, Clock, Loading, Weather,
  SPrice, `ss_render`, and share-accept behavior to UI state. Screen builders,
  generic setters, Miner binding fan-out plus candidate producer blocks, Clock
  BTC price text route, SPrice kline/candle renderer roles, and share-result
  format callsites are mapped; proof/safety gaps remain before function edits.
- [~] **BIN-03**: A binary edit target map records candidate literals/functions,
  replacement-size constraints, available code/data relocation options, checksum
  and ESP32 image validation requirements, and explicit no-touch zones. Literal
  candidates and function/callsite navigation records exist; function edits are
  not approved.
- [~] **BIN-04**: Local-only patch tooling can produce a modified app or
  filesystem artifact, validate partition bounds and ESP32 image
  checksum/hash/metadata, and keep generated binaries out of git. Current tool
  covers approved fixed-length DROM literals only.
- [ ] **BIN-05**: Any future write of a patched official artifact is gated by a
  fresh 4 MB backup, `verify_flash`, hard USB power-cycle, and preserved restore
  command. No patched stock image is redistributed.

### Asset Recovery (ASSET)

- [x] **ASSET-01**: The stock LVGL font(s)/glyphs are extracted as reusable LVGL C-arrays (or a matched substitute is documented), together with the stock color palette.
- [x] **ASSET-02**: The LittleFS partition is unpacked with LittleFS tooling (NOT mkspiffs): gzipped web UI + the 17 `/ss/saver_320_240_*.ss` screensavers.
- [x] **ASSET-03**: A byte-accurate `.ss` decoder (extending `scripts/nmminer_ss_decode.py`) decodes all 17 stock screensavers cleanly and is reusable by the firmware screensaver carousel.

### Core UI (UI)

> Hardware-dependent UI requirements stay unchecked until Phase 7 proves them on the device.

- [ ] **UI-01**: On boot the device shows a Loading screen reproducing the stock look (NMMiner / "Make it better", firmware version, connection progress) with kleinpanic branding layered on. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **UI-02**: A `UIManager` registers the 6 named screens (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) as a page list with `LastPage` NVS persistence (resumes on last page after reboot). *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **UI-03**: The Miner screen renders the full `/api/system/info.miner` field set (hashrate, shares R/A, net/pool/last/best diff session + all-time, blkhits, freeheap, RSSI, temp, pool URL) matching stock formatting. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **UI-04**: The Clock screen renders NTP time (selectable 12/24h + date format), date, and BTC price with green-up / red-down coloring. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **UI-05**: A Config screen shows the config-mode indicator; the AP `nmap-2.4g` + captive portal at `192.168.4.1` is entered via long-press boot (10s) or 15s WiFi-connect failure. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **UI-06**: All screens are rebuilt natively from SPEC.md on the `NerdMiner-CYD-Custom` fork (driven by recovered evidence, not imagination or mis-carved assets). *(Code/build provisional; Phase 7 hardware retest pending.)*

### Touch & Input (TOUCH)

- [ ] **TOUCH-01**: Touch works without using hardware VSPI (HSPI/TFT_eTouch/direct-probe path, leaving VSPI free for SD), diagnosed via raw-Z pressure first to separate bus vs calibration faults. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **TOUCH-02**: Short touch = next page, long-press (10s) = config mode, tap wakes a sleeping screen. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **TOUCH-03**: Boot button: double-click = next page, long-press = force config, single-click = wake screen. *(Code/build provisional; Phase 7 hardware retest pending.)*

### Data-Driven Screens & API (DATA)

- [ ] **DATA-01**: The SPrice screen renders Binance (`data-api.binance.vision`) kline candlestick + pricewall modes, page-gated to run only on Miner/Clock/Price. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **DATA-02**: The Weather screen renders open-meteo forecast/AQI (°C/°F, wind, sunrise/sunset) with a rate-limited `/api/weather/refresh`. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **DATA-03**: The screensaver carousel renders decoded `.ss` images after idle timeout, supports black mode and timeout prefs, exposes upload/preflight endpoints, and resumes on touch. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **DATA-04**: Swarm LAN discovery (Miner-page-gated) + `/api/swarm/find` FindMe overlay that visibly overrides an active screensaver. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **DATA-05**: The HTTP API matches stock paths/shapes (`/probe`, `/alive`, `/api/system/info`, `/api/setting/*`, `/api/market/pairs`, `/api/weather/refresh`, `/api/swarm/find`, `/api/system/restart`, screensaver preflight/upload) so the existing dashboard/controller contract holds. *(Code/build provisional; Phase 7 hardware retest pending.)*
- [ ] **DATA-06**: Branding/identity strings (`model="NMMiner"`, "Make it better") are preserved so controllers keying off `model` still work. *(Code/build provisional; Phase 7 hardware retest pending.)*

### Custom Hardware Re-add (HW)

- [ ] **HW-01**: The speaker (LEDC GPIO26) beeps on an accepted share and alarms on pool disconnect, wired at the NerdMiner share-accept seam (in-process call, not an Xtensa hook). *(Code/build PASS; audible confirm -> Phase 7/VALID-02.)*
- [ ] **HW-02**: The SD card (VSPI GPIO5/18/19/23) mounts and logs on the now-free hardware VSPI (after touch moved off hardware VSPI). *(Partial live evidence exists; final repaired build still needs Phase 7.)*
- [ ] **HW-03**: Custom LED behavior and kleinpanic branding accents are integrated over the recovered UI, verified against silkscreen pins. *(Code/build PASS; visible confirm -> Phase 7.)*
- [ ] **HW-04**: Mining stays fully functional throughout all changes (NerdMiner stratum core unchanged; primary + fallback pool, switch-back to primary <=10s). *(Code/build/app-flash/readback PASS for fallback support; live-mining confirm -> Phase 7.)*

### On-Device Validation (VALID)

- [ ] **VALID-01**: Every device flash is preceded by a fresh 4 MB esptool backup and followed by `verify_flash` + a hard USB power-cycle (documented flash runbook).
- [ ] **VALID-02**: The rebuilt fork firmware is flashed and serial-confirmed to boot, passing a visual parity checklist against stock, with mining confirmed active on-device.
- [ ] **VALID-03**: The restore-from-backup path is documented and proven reversible against the 4 MB backup.

### Fallback Contingency (FALLBACK)

- [~] **FALLBACK-01**: Historical fork-era contingency. Superseded by BIN-01..05:
  official `v2.0.02` binary customization is now the active route, still with
  integrity recon first, no license-path edits, no secrets, and no
  redistribution.

## v2 Requirements

Deferred polish; tracked, not in this milestone's roadmap.

### Differentiator Polish (POLISH)

- **POLISH-01**: New-firmware-available reminder on loading/miner page.
- **POLISH-02**: Christmas surprise easter-egg (Dec 25).
- **POLISH-03**: Auto-timezone derivation.
- **POLISH-04**: Dark / "remove second" clock styling variants.
- **POLISH-05**: Browser cache strategy + web-frontend capability gating from stock.

## Out of Scope

Explicitly excluded. Documented to prevent scope creep (anti-features from research).

| Feature | Reason |
|---------|--------|
| License/activation path | Closed, server-dependent, and explicitly no-touch; custom work must not bypass or modify it |
| Further fork UI/touch tuning as the primary route | Superseded by the explicit official `v2.0.02` binary decompilation/custom-edit route |
| Redistributing any recovered closed-firmware asset or modified stock image | Unlicensed binary, no redistribution; personal-use only, gitignored |
| Non-Binance / non-open-meteo data sources | Breaks number/cadence parity with stock |
| Multi-board abstraction (30+ boards) | Target exactly one board: `esp32-2432s028r-st7789` |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| RE-01 | Phase 1 | Complete |
| RE-02 | Phase 1 | Complete |
| RE-03 | Phase 1 | Complete |
| RE-04 | Phase 1 | Complete |
| BIN-01 | Phase 7 | Complete: baseline provenance locked in `07-BINARY-BASELINE-ROUTE.md` |
| BIN-02 | Phase 7 | In progress: mapped screen builders/setters, key runtime anchors, and candidate producer/caller blocks; proof/safety decompilation remains |
| BIN-03 | Phase 7 | In progress: literal candidates and function/callsite navigation records mapped; no function edit approved |
| BIN-04 | Phase 7 | In progress: fixed-length DROM literal patcher exists; function/filesystem patch tooling not yet approved |
| BIN-05 | Phase 7 | Pending |
| ASSET-01 | Phase 2 | Complete |
| ASSET-02 | Phase 2 | Complete |
| ASSET-03 | Phase 2 | Complete |
| UI-01 | Phase 3 | Provisional: code/build complete; on-device failed before 07-02 repair |
| UI-02 | Phase 3 | Provisional: code/build complete; on-device pending |
| UI-03 | Phase 3 | Provisional: code/build complete; on-device pending |
| UI-04 | Phase 3 | Provisional: code/build complete; on-device pending |
| UI-05 | Phase 3 | Provisional: code/build complete; on-device pending |
| UI-06 | Phase 3 | Provisional: code/build complete; on-device pending |
| TOUCH-01 | Phase 4 | Partial live evidence: no-VSPI HSPI/direct-probe path boots, but touch controller reports no press signal; hardware/pin diagnosis pending |
| TOUCH-02 | Phase 4 | Provisional: code/build complete; hardware validation pending |
| TOUCH-03 | Phase 4 | Provisional: code/build complete; hardware validation pending |
| DATA-01 | Phase 5 | Provisional: code/build complete; on-device pending |
| DATA-02 | Phase 5 | Provisional: code/build complete; on-device pending |
| DATA-03 | Phase 5 | Partial live evidence: decoded LittleFS `.ss` asset image flashed at `0x310000` and verifies after the latest app reflash; visual carousel proof pending |
| DATA-04 | Phase 5 | Provisional: code/build complete; on-device pending |
| DATA-05 | Phase 5 | Provisional: code/build complete; on-device pending |
| DATA-06 | Phase 5 | Provisional: code/build complete; on-device pending |
| HW-01 | Phase 6 | Provisional: code/build complete; audible confirm → Phase 7 |
| HW-02 | Phase 6 | Partial live evidence: SD mounted once; final repaired build still needs Phase 7 |
| HW-03 | Phase 6 | Provisional: code/build complete; visible confirm → Phase 7 |
| HW-04 | Phase 6 | Provisional: fallback source/build/app-flash/readback complete; live primary/fallback mining confirmation pending Phase 7 |
| VALID-01 | Phase 7 | Partial: fresh backups and app readback verification performed for custom/stock/NVS-clear/current app writes; hard physical USB power-cycle proof still pending |
| VALID-02 | Phase 7 | Partial: current custom decoded-spec ST7789 firmware is live with fallback-pool support, serial-boots to recovery/config mode, and WiFiManager recovery routes return HTTP 200; custom JSON API routes return 404 in portal mode; physical panel/touch/mining proof pending |
| VALID-03 | Phase 7 | Partial: latest full backup exists with SHA and known restore command; restore round-trip not run |
| FALLBACK-01 | Phase 8 | Superseded by BIN route |

**Coverage:**

- v1 requirements: 35 total
- Mapped to phases: 35
- Unmapped: 0 ✓

> ## ⚠️ ON-DEVICE VALIDATION STATUS (2026-06-18) — HONEST CORRECTION
>
> The `[x]` / "Complete" marks on **UI-01..06, TOUCH-01..03, DATA-01..06, HW-01..04** mean
> **code-complete + compiled + code-reviewed ONLY**. They were NOT verified on hardware — the
> milestone deferred all on-device checks to Phase 7. **Phase 7 then FAILED:** the rebuilt v3.0
> UI did not render correctly on the real ESP32-2432S028R (white screen, upside-down render, config
> crash, boot-loop, and WiFi regression). 07-02 has since repaired the known pre-flash software
> blockers and passed a build, but these requirements are **still not hardware-validated**. Treat
> them as **PROVISIONAL / FAILED-PENDING-RETEST on-device** until 07-01 records fresh hardware
> evidence in `07-VALIDATION.md`.
>
> - **VALID-01** — partial: custom and official stock flashes, decoded LittleFS writes, NVS clear, and latest app reflash were preceded by fresh backups where required and followed by verify/readback evidence; still needs hard physical USB power-cycle proof.
> - **VALID-02** — partial: current custom decoded-spec ST7789 firmware serial-boots to recovery/config mode with fallback-pool support live, WiFiManager recovery routes return HTTP 200, and the decoded LittleFS asset partition verifies after app reflashes; custom JSON API routes return 404 in portal mode, and physical panel/touch proof, Wi-Fi/pool join, and mining evidence remain open.
> - **VALID-03** — partial: latest full backup integrity hash exists and the restore command is known; full restore round-trip remains unrun.
> - **DATA-03** — partial: decoded `.ss` LittleFS image is on flash and survives the latest app reflash; visual carousel proof is still pending.
> - **HW-02** — SD mounts on FAT32 on-device (confirmed live).
> - Genuinely-done-and-not-device-dependent: **RE-01..04, ASSET-01..03** (Phases 1–2).
>
> Route correction, 2026-06-19: Phase 7 remains in progress, but the active
> implementation route is now the official `v2.0.02` ST7789 binary
> decompilation/custom-edit path. Fork physical-panel/touch/mining failures
> remain evidence and recovery context, not the next implementation target.

---
*Requirements defined: 2026-06-18*
*Last updated: 2026-06-19 after Phase 7 fallback-pool app flash/readback and post-flash serial validation*
