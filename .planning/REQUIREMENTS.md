# Requirements: NMMiner ESP32 Series — Milestone v3.0 (NMMiner RE + Custom Integrations)

**Defined:** 2026-06-18
**Core Value:** The device looks and behaves exactly like stock NMMiner v2.0.02 (recovered by reverse-engineering the closed binary as a specification), rebuilt natively on the owned MIT `NerdMiner-CYD-Custom` fork, PLUS our custom speaker / SD / LED / kleinpanic branding — while mining keeps working throughout.

> Prior milestone (v2.0.02 bring-up + NerdMiner fork) requirements & roadmap are archived under `.planning/archive/v2.0.02-fork-milestone/`. This file is scoped to v3.0 only.

## v1 Requirements

Requirements for milestone v3.0. Each maps to exactly one roadmap phase (see Traceability).

### Reverse-Engineering Spec (RE)

- [x] **RE-01**: A working Ghidra 11.3.1 + dynacylabs esp32 flash-loader v11.3.1 (prebuilt) workbench loads the stock image with DROM (`0x3f400020`) / IROM (`0x400d0020`) mapped; the self-built ESP-IDF v4.4.6 FIDB is attempted, and stripped SDK/Arduino-core symbols are recovered via the FIDB where it applies **OR** via a documented fallback (dynacylabs loader ROM/SVD annotations + the proven string→`l32r` xref pipeline) sufficient to decompile the screen / render / share-accept functions.
- [x] **RE-02**: The stock LVGL major version (v8 vs v9) is confirmed authoritatively in-decompiler, citing **at least two independent items from the acceptable-evidence set** (decoder/render call-shape; `lv_img_dsc_t`/`lv_font_t` descriptor layout; screen-constructor API signatures; DROM string family `LV_IMG_CF_*` v8 / `LV_COLOR_FORMAT_*` v9; build-era/version strings) with a recorded Confidence label, before any extractor is written.
- [x] **RE-03**: The located screen-construction functions (`Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`) plus the render / share-accept / screensaver code paths are decompiled into `hardware/recovered-assets/SPEC.md` (screen inventory, per-screen widget geometry, fonts/colors, market field set, screensaver render path).
- [x] **RE-04**: SPEC.md is markdown-only and never linked into firmware; all recovered closed-firmware assets remain gitignored (no redistribution).

### Asset Recovery (ASSET)

- [x] **ASSET-01**: The stock LVGL font(s)/glyphs are extracted as reusable LVGL C-arrays (or a matched substitute is documented), together with the stock color palette.
- [x] **ASSET-02**: The LittleFS partition is unpacked with LittleFS tooling (NOT mkspiffs): gzipped web UI + the 17 `/ss/saver_320_240_*.ss` screensavers.
- [x] **ASSET-03**: A byte-accurate `.ss` decoder (extending `scripts/nmminer_ss_decode.py`) decodes all 17 stock screensavers cleanly and is reusable by the firmware screensaver carousel.

### Core UI (UI)

- [x] **UI-01**: On boot the device shows a Loading screen reproducing the stock look (NMMiner / "Make it better", firmware version, connection progress) with kleinpanic branding layered on.
- [x] **UI-02**: A `UIManager` registers the 6 named screens (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) as a page list with `LastPage` NVS persistence (resumes on last page after reboot).
- [x] **UI-03**: The Miner screen renders the full `/api/system/info.miner` field set (hashrate, shares R/A, net/pool/last/best diff session + all-time, blkhits, freeheap, RSSI, temp, pool URL) matching stock formatting.
- [x] **UI-04**: The Clock screen renders NTP time (selectable 12/24h + date format), date, and BTC price with green-up / red-down coloring.
- [x] **UI-05**: A Config screen shows the config-mode indicator; the AP `nmap-2.4g` + captive portal at `192.168.4.1` is entered via long-press boot (10s) or 15s WiFi-connect failure.
- [x] **UI-06**: All screens are rebuilt natively from SPEC.md on the `NerdMiner-CYD-Custom` fork (driven by recovered evidence, not imagination or mis-carved assets).

### Touch & Input (TOUCH)

- [x] **TOUCH-01**: Touch works on SoftSPI (moved off hardware VSPI, leaving VSPI free for SD), diagnosed via raw-Z pressure first to separate bus vs calibration faults.
- [x] **TOUCH-02**: Short touch = next page, long-press (10s) = config mode, tap wakes a sleeping screen.
- [x] **TOUCH-03**: Boot button: double-click = next page, long-press = force config, single-click = wake screen.

### Data-Driven Screens & API (DATA)

- [x] **DATA-01**: The SPrice screen renders Binance (`data-api.binance.vision`) kline candlestick + pricewall modes, page-gated to run only on Miner/Clock/Price.
- [x] **DATA-02**: The Weather screen renders open-meteo forecast/AQI (°C/°F, wind, sunrise/sunset) with a rate-limited `/api/weather/refresh`.
- [x] **DATA-03**: The screensaver carousel renders decoded `.ss` images after idle timeout, supports black mode and timeout prefs, exposes upload/preflight endpoints, and resumes on touch.
- [x] **DATA-04**: Swarm LAN discovery (Miner-page-gated) + `/api/swarm/find` FindMe overlay that visibly overrides an active screensaver.
- [x] **DATA-05**: The HTTP API matches stock paths/shapes (`/probe`, `/alive`, `/api/system/info`, `/api/setting/*`, `/api/market/pairs`, `/api/weather/refresh`, `/api/swarm/find`, `/api/system/restart`, screensaver preflight/upload) so the existing dashboard/controller contract holds.
- [x] **DATA-06**: Branding/identity strings (`model="NMMiner"`, "Make it better") are preserved so controllers keying off `model` still work.

### Custom Hardware Re-add (HW)

- [x] **HW-01**: The speaker (LEDC GPIO26) beeps on an accepted share and alarms on pool disconnect, wired at the NerdMiner share-accept seam (in-process call, not an Xtensa hook). *(Code/build PASS — `beepShareAccepted()` non-blocking at mining.cpp seam; audible confirm → Phase 7/VALID-02.)*
- [x] **HW-02**: The SD card (VSPI GPIO5/18/19/23) mounts and logs on the now-free hardware VSPI (after touch moved to SoftSPI). *(Code/build PASS — `logShareEvent()` isReady-gated via existing appendCSVLog; physical-card confirm → Phase 7.)*
- [x] **HW-03**: Custom LED behavior and kleinpanic branding accents are integrated over the recovered UI, verified against silkscreen pins. *(Code/build PASS — `nm_led` sole RGB owner on LED_PIN/G/B macros; visible confirm → Phase 7.)*
- [x] **HW-04**: Mining stays fully functional throughout all changes (NerdMiner stratum core unchanged; primary + fallback pool, switch-back to primary ≤10s). *(PASS — mining.cpp phase diff = the single seam line; build SUCCESS; live-mining confirm → Phase 7.)*

### On-Device Validation (VALID)

- [ ] **VALID-01**: Every device flash is preceded by a fresh 4 MB esptool backup and followed by `verify_flash` + a hard USB power-cycle (documented flash runbook).
- [ ] **VALID-02**: The rebuilt fork firmware is flashed and serial-confirmed to boot, passing a visual parity checklist against stock, with mining confirmed active on-device.
- [ ] **VALID-03**: The restore-from-backup path is documented and proven reversible against the 4 MB backup.

### Fallback Contingency (FALLBACK)

- [ ] **FALLBACK-01**: A go/no-go decision on the Path-A binary-patch spike is made and recorded — executed (with integrity recon first, never touching the license path, never redistributing) ONLY if RE reveals an NMMiner-exclusive behavior unreproducible on the fork; otherwise formally waived with rationale.

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
| License/activation gate | Closed, server-dependent, blocks our own builds; never touch the license verification path |
| Patch stock `firmware.bin` as the primary path | Binary-only, fragmented free flash, SHA-256 integrity check, no-redistribution, brick risk — kept only as the FALLBACK-01 contingency |
| Non-Binance / non-open-meteo data sources | Breaks number/cadence parity with stock |
| Multi-board abstraction (30+ boards) | Target exactly one board: `esp32-2432s028r-st7789` |
| Redistributing any recovered closed-firmware asset or modified stock image | Unlicensed binary, no redistribution; personal-use only, gitignored |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| RE-01 | Phase 1 | Pending |
| RE-02 | Phase 1 | Pending |
| RE-03 | Phase 1 | Pending |
| RE-04 | Phase 1 | Pending |
| ASSET-01 | Phase 2 | Pending |
| ASSET-02 | Phase 2 | Pending |
| ASSET-03 | Phase 2 | Pending |
| UI-01 | Phase 3 | Complete |
| UI-02 | Phase 3 | Pending |
| UI-03 | Phase 3 | Pending |
| UI-04 | Phase 3 | Pending |
| UI-05 | Phase 3 | Pending |
| UI-06 | Phase 3 | Complete |
| TOUCH-01 | Phase 4 | Pending |
| TOUCH-02 | Phase 4 | Pending |
| TOUCH-03 | Phase 4 | Pending |
| DATA-01 | Phase 5 | Pending |
| DATA-02 | Phase 5 | Pending |
| DATA-03 | Phase 5 | Pending |
| DATA-04 | Phase 5 | Pending |
| DATA-05 | Phase 5 | Pending |
| DATA-06 | Phase 5 | Pending |
| HW-01 | Phase 6 | Complete (on-device→P7) |
| HW-02 | Phase 6 | Complete (on-device→P7) |
| HW-03 | Phase 6 | Complete (on-device→P7) |
| HW-04 | Phase 6 | Complete |
| VALID-01 | Phase 7 | Pending |
| VALID-02 | Phase 7 | Pending |
| VALID-03 | Phase 7 | Pending |
| FALLBACK-01 | Phase 8 | Pending |

**Coverage:**

- v1 requirements: 30 total
- Mapped to phases: 30
- Unmapped: 0 ✓

> ## ⚠️ ON-DEVICE VALIDATION STATUS (2026-06-18) — HONEST CORRECTION
>
> The `[x]` / "Complete" marks on **UI-01..06, TOUCH-01..03, DATA-01..06, HW-01..04** mean
> **code-complete + compiled + code-reviewed ONLY**. They were NOT verified on hardware — the
> milestone deferred all on-device checks to Phase 7. **Phase 7 then FAILED:** the rebuilt v3.0
> UI does not render correctly on the real ESP32-2432S028R (white screen, was upside-down, config
> crash, boot-loop; WiFi regression introduced this session). So these requirements are **NOT
> actually validated** — treat them as **PROVISIONAL / FAILED on-device** pending the H1–H5
> hardening in `.planning/phases/07-on-device-validation/07-REVIEW.md`.
>
> - **VALID-01** — backup+verify_flash done, but the flashed build is broken → **partial**.
> - **VALID-02** — boot serial OK + mining core starts, but **visual parity FAILED** (UI broken). → **FAILED**.
> - **VALID-03** — restore path documented + backup proven valid (4 MB, sha recorded) → **met**, not yet round-trip-exercised.
> - **HW-02** — SD mounts on FAT32 on-device (confirmed live) → the one HW item with real on-device evidence.
> - Genuinely-done-and-not-device-dependent: **RE-01..04, ASSET-01..03** (Phases 1–2).
>
> Milestone v3.0 is **NOT complete**. Status: paused by user after on-device failure.

---
*Requirements defined: 2026-06-18*
*Last updated: 2026-06-18 after milestone v3.0 definition*
