# Roadmap: NMMiner ESP32 Series

**Milestone:** v3.0 — NMMiner RE + Custom Integrations
**Granularity:** coarse
**Phase ID convention:** versioned milestone headings (`v3.0 / Phase N`)
**Created:** 2026-06-18

> Prior milestone (v2.0.02 bring-up + NerdMiner fork) is archived under
> `.planning/archive/v2.0.02-fork-milestone/`. Its free-form roadmap is fully
> replaced by this file. Phase numbering RESETS at Phase 1 for v3.0.

## Core Value

The device looks and behaves exactly like stock NMMiner v2.0.02 — recovered by
reverse-engineering the closed binary as a read-only specification, rebuilt
natively on the owned MIT `NerdMiner-CYD-Custom` fork — PLUS our custom speaker /
SD / LED / kleinpanic branding, while mining keeps working throughout.

## Dependency Spine

RE-spec the binary → recover fonts/palette + filesystem assets → rebuild core
screens + carousel → restore touch (SoftSPI, before SD) → add data-driven
screens + API parity → wire custom HW at the share-accept seam → on-device
validate → (contingency) Path-A binary-patch spike.

Hard ordering locks:
- **Spec-before-build:** screens are runtime LVGL code, not stored bitmaps, so the read-only spec gates all UI work.
- **Touch-before-SD:** touch must move to SoftSPI before SD claims hardware VSPI (CYD dual-SPI collision).
- **Core-screens-before-data-screens:** page-gated market/weather/swarm hang off the UIManager carousel.
- **On-device validation is BLOCKED** until `/dev/ttyUSB0` (data, not charge-only) appears. All software phases (1-6) and the fallback decision (8) are completable without the device.

## Phases

- [ ] **v3.0 / Phase 1: RE-Spec the UI (read-only)** - Ghidra/FIDB decompile the located screen funcs into SPEC.md; confirm LVGL v8 vs v9. No flashing.
- [ ] **v3.0 / Phase 2: Recover Fonts/Glyphs + Filesystem Assets** - Extract LVGL font C-arrays + palette; unpack LittleFS; byte-accurate `.ss` decoder for all 17 screensavers.
- [ ] **v3.0 / Phase 3: Rebuild Core Screens + UIManager Carousel** - Loading/Miner/Clock/Config screens + page list + LastPage NVS persistence + branding splash on the fork.
- [ ] **v3.0 / Phase 4: Restore Interactive Touch (SoftSPI)** - Touch on SoftSPI (frees VSPI for SD); short/long-press nav + config entry + wake; boot-button controls.
- [ ] **v3.0 / Phase 5: Data-Driven Screens + HTTP API Parity** - SPrice (Binance), Weather (open-meteo), screensaver carousel, Swarm + FindMe, 21-endpoint API, branding strings.
- [ ] **v3.0 / Phase 6: Wire Custom Features at the Share-Accept Seam** - Speaker (LEDC GPIO26), SD (VSPI) on the now-free bus, custom LED + kleinpanic accents; mining stays functional.
- [ ] **v3.0 / Phase 7: On-Device Validation (BLOCKED on /dev/ttyUSB0)** - Backup → flash fork → verify_flash + power-cycle → serial boot + visual parity + mining active; reversible restore.
- [ ] **v3.0 / Phase 8: Fallback Path-A Binary-Patch Spike (contingency)** - Go/no-go decision recorded; execute the spike only if RE surfaced an unreproducible NMMiner-exclusive behavior, else formally waive.

## Phase Details

### v3.0 / Phase 1: RE-Spec the UI (read-only)
**Goal**: The closed stock binary is turned into a written design contract that all UI rebuild work can be driven from, with the LVGL version question settled authoritatively.
**Depends on**: Nothing (first phase; RE foundation already on disk — DROM/IROM isolated, IROM disassembled, screen funcs located)
**Requirements**: RE-01, RE-02, RE-03, RE-04
**Success Criteria** (what must be TRUE):
  1. A Ghidra 11.x + dynacylabs flash-loader v12.0 + self-built ESP-IDF v4.4.6 FIDB workbench opens the stock image with DROM (`0x3f400020`) / IROM (`0x400d0020`) mapped and stripped SDK/Arduino-core symbols recovered.
  2. The stock LVGL major version (v8 vs v9) is stated with evidence (FIDB-named decoder symbols + `LV_IMG_CF_*` / `LV_COLOR_FORMAT_*` string family) before any extractor is written.
  3. `hardware/recovered-assets/SPEC.md` exists and documents the screen inventory, per-screen widget geometry, fonts/colors, market field set, and screensaver render path, decompiled from `Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`, and the render / share-accept / screensaver paths.
  4. SPEC.md is markdown-only, never linked into firmware, and all recovered closed-firmware assets remain gitignored (no redistribution).
**Plans**: TBD
**Research**: recommended (FIDB build is a non-trivial Tarlogic workflow; per-screen geometry extraction is novel for this project)

### v3.0 / Phase 2: Recover Fonts/Glyphs + Filesystem Assets
**Goal**: The reusable visual assets that ARE extractable from the binary (fonts, palette, screensavers, web UI) are recovered cleanly and ready to feed the rebuild.
**Depends on**: Phase 1 (SPEC.md identifies which fonts/glyphs/assets matter and the confirmed LVGL version)
**Requirements**: ASSET-01, ASSET-02, ASSET-03
**Success Criteria** (what must be TRUE):
  1. The stock LVGL font(s)/glyphs are extracted as reusable LVGL C-arrays (or a matched substitute is documented), together with the stock color palette.
  2. The LittleFS partition is unpacked with LittleFS tooling (NOT mkspiffs), yielding the gzipped web UI plus the 17 `/ss/saver_320_240_*.ss` screensavers.
  3. A byte-accurate `.ss` decoder (extending `scripts/nmminer_ss_decode.py`) decodes all 17 stock screensavers cleanly and is reusable by the firmware screensaver carousel.
**Plans**: TBD

### v3.0 / Phase 3: Rebuild Core Screens + UIManager Carousel
**Goal**: Booting the fork reproduces the stock NMMiner structural identity — the named screens, the page carousel, and last-page persistence — driven by recovered evidence.
**Depends on**: Phase 1 (SPEC.md geometry), Phase 2 (fonts/palette)
**Requirements**: UI-01, UI-02, UI-03, UI-04, UI-05, UI-06
**Success Criteria** (what must be TRUE):
  1. On boot the device shows a Loading screen reproducing the stock look (NMMiner / "Make it better", firmware version, connection progress) with kleinpanic branding layered on.
  2. A `UIManager` registers the 6 named screens (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) as a page list with `LastPage` NVS persistence, resuming on the last page after reboot.
  3. The Miner screen renders the full `/api/system/info.miner` field set (hashrate, shares R/A, net/pool/last/best diff session + all-time, blkhits, freeheap, RSSI, temp, pool URL) matching stock formatting.
  4. The Clock screen renders NTP time (12/24h + date format), date, and BTC price with green-up / red-down coloring, and a Config screen shows the config-mode indicator with AP `nmap-2.4g` + captive portal at `192.168.4.1`.
  5. All screens are rebuilt natively from SPEC.md on the `NerdMiner-CYD-Custom` fork (recovered evidence, not imagination or mis-carved assets).
**Plans**: TBD
**UI hint**: yes

### v3.0 / Phase 4: Restore Interactive Touch (SoftSPI)
**Goal**: The user can navigate the rebuilt UI by touch and boot button, with touch moved off hardware VSPI so SD can later own it cleanly.
**Depends on**: Phase 3 (screens + carousel to navigate). MUST come before Phase 6 SD wiring (VSPI bus-ownership lock-in).
**Requirements**: TOUCH-01, TOUCH-02, TOUCH-03
**Success Criteria** (what must be TRUE):
  1. Touch works on SoftSPI (moved off hardware VSPI, leaving VSPI free for SD), diagnosed via raw-Z pressure first to separate bus vs calibration faults.
  2. Short touch = next page, long-press (10s) = config mode, tap wakes a sleeping screen.
  3. Boot button: double-click = next page, long-press = force config, single-click = wake screen.
**Plans**: TBD
**UI hint**: yes

### v3.0 / Phase 5: Data-Driven Screens + HTTP API Parity
**Goal**: The page-gated market/weather/swarm/screensaver behaviors and the full HTTP API match stock cadence and shapes so the existing dashboard/controller contract holds.
**Depends on**: Phase 3 (carousel), Phase 4 (touch wake/nav), Phase 2 (`.ss` decoder)
**Requirements**: DATA-01, DATA-02, DATA-03, DATA-04, DATA-05, DATA-06
**Success Criteria** (what must be TRUE):
  1. The SPrice screen renders Binance (`data-api.binance.vision`) kline candlestick + pricewall modes, page-gated to run only on Miner/Clock/Price.
  2. The Weather screen renders open-meteo forecast/AQI (°C/°F, wind, sunrise/sunset) with a rate-limited `/api/weather/refresh`.
  3. The screensaver carousel renders decoded `.ss` images after idle timeout, supports black mode and timeout prefs, exposes upload/preflight endpoints, and resumes on touch.
  4. Swarm LAN discovery (Miner-page-gated) plus `/api/swarm/find` FindMe overlay visibly overrides an active screensaver.
  5. The HTTP API matches stock paths/shapes (`/probe`, `/alive`, `/api/system/info`, `/api/setting/*`, `/api/market/pairs`, `/api/weather/refresh`, `/api/swarm/find`, `/api/system/restart`, screensaver preflight/upload), and branding/identity strings (`model="NMMiner"`, "Make it better") are preserved.
**Plans**: TBD
**Research**: recommended (`.ss` header byte-layout, kline rendering specifics, idle→screensaver state machine are open)
**UI hint**: yes

### v3.0 / Phase 6: Wire Custom Features at the Share-Accept Seam
**Goal**: Our custom speaker / SD / LED / branding light up over the recovered UI at the in-process NerdMiner share-accept seam, without breaking mining.
**Depends on**: Phase 4 (touch on SoftSPI frees VSPI for SD), Phase 3/5 (UI to brand). Custom feature code already built+tested in the fork.
**Requirements**: HW-01, HW-02, HW-03, HW-04
**Success Criteria** (what must be TRUE):
  1. The speaker (LEDC GPIO26) beeps on an accepted share and alarms on pool disconnect, wired at the NerdMiner share-accept seam (in-process call, not an Xtensa hook).
  2. The SD card (VSPI GPIO5/18/19/23) mounts and logs on the now-free hardware VSPI.
  3. Custom LED behavior and kleinpanic branding accents are integrated over the recovered UI, verified against silkscreen pins.
  4. Mining stays fully functional throughout (NerdMiner stratum core unchanged; primary + fallback pool, switch-back to primary ≤10s).
**Plans**: TBD
**UI hint**: yes

### v3.0 / Phase 7: On-Device Validation (BLOCKED on /dev/ttyUSB0)
**Goal**: The rebuilt fork firmware is proven on the real device — booting, mining, and visually matching stock — with a documented reversible restore path.
**Depends on**: Phases 3-6 (software complete). HARD BLOCKER: gated on `/dev/ttyUSB0` (data port, not charge-only) appearing.
**Requirements**: VALID-01, VALID-02, VALID-03
**Success Criteria** (what must be TRUE):
  1. Every device flash is preceded by a fresh 4 MB esptool backup and followed by `verify_flash` plus a hard USB power-cycle (documented flash runbook).
  2. The rebuilt fork firmware is flashed and serial-confirmed to boot, passes a visual parity checklist against stock, and has mining confirmed active on-device.
  3. The restore-from-backup path is documented and proven reversible against the 4 MB backup.
**Plans**: TBD

### v3.0 / Phase 8: Fallback Path-A Binary-Patch Spike (contingency)
**Goal**: A recorded go/no-go verdict on the Path-A binary-patch contingency — satisfied whether the spike is executed or formally waived.
**Depends on**: Phase 1 (RE findings determine whether any NMMiner-exclusive behavior is unreproducible on the fork)
**Requirements**: FALLBACK-01
**Success Criteria** (what must be TRUE):
  1. A go/no-go decision on the Path-A binary-patch spike is made and recorded in writing (the decision-made-and-recorded state satisfies this phase regardless of outcome).
  2. IF executed: integrity recon (`esptool image_info` + `espefuse summary`) ran first, the license-verification path was never touched, and nothing was redistributed.
  3. IF waived: a written rationale is recorded (research SUMMARY identifies no NMMiner-exclusive unreproducible behavior, so a waiver is the expected outcome).
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. RE-Spec the UI | 0/0 | Not started | - |
| 2. Recover Fonts/Assets | 0/0 | Not started | - |
| 3. Core Screens + Carousel | 0/0 | Not started | - |
| 4. Restore Touch (SoftSPI) | 0/0 | Not started | - |
| 5. Data Screens + API | 0/0 | Not started | - |
| 6. Custom Features | 0/0 | Not started | - |
| 7. On-Device Validation | 0/0 | Blocked (/dev/ttyUSB0) | - |
| 8. Fallback Spike | 0/0 | Not started | - |

## Coverage

- v3.0 requirements: 30
- Mapped to phases: 30
- Unmapped: 0 ✓

Full mapping in `REQUIREMENTS.md` Traceability table.
