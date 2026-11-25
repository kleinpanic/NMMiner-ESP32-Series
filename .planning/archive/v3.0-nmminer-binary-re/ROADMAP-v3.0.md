# Roadmap: NMMiner ESP32 Series

## v3.0 — NMMiner RE + Custom Integrations

**Milestone:** v3.0 — NMMiner RE + Custom Integrations
**Granularity:** coarse
**Phase ID convention:** `Phase N` headings under the `## v3.0` milestone section
**Created:** 2026-06-18

> Prior milestone (v2.0.02 bring-up + NerdMiner fork) is archived under
> `.planning/archive/v2.0.02-fork-milestone/`. Its free-form roadmap is fully
> replaced by this file. Phase numbering RESETS at Phase 1 for v3.0.

## Core Value

The active path is the official NMMiner `v2.0.02`
`esp32-2432s028r-st7789` binary: preserve provenance, decompile it, identify
safe custom-edit seams, and produce local-only modified artifacts without
touching license/activation paths, secrets, or redistributing closed firmware.
The fork rebuild is historical context and recovery/failure evidence, not the
primary implementation route.

## Dependency Spine

Official `v2.0.02` provenance lock → Ghidra/decompiler function normalization →
dynamic UI/update path map → binary edit target map → local-only patch tooling →
ESP32 image validation → gated device write only after a fresh full backup.

Hard ordering locks:
- **Version lock:** the public flasher now lists `v2.0.03`, but the repo's
  extracted/decompiled baseline is official `v2.0.02`. Do not silently switch
  versions.
- **No-touch zones:** license/activation, Wi-Fi credentials, wallet values, NVS
  secrets, and device licenses are never patch targets.
- **No redistribution:** recovered assets, full flash backups, and modified
  stock binaries stay local and gitignored.
- **On-device writes are gated:** every future firmware write must preserve a
  fresh full 4 MB backup first and verify the write afterward. The board target
  remains `esp32-2432s028r-st7789` unless hardware is re-identified.

## Phases

> ⚠️ **STATUS (2026-06-18): milestone in Phase 7, on-device validation FAILED, then software-repaired and reflashed with the custom decoded-spec ST7789 fork.** The `[x]` on Phases
> 3–6 means **code-complete + compiled + code-reviewed**, NOT hardware-verified. On-device (Phase 7)
> the rebuilt UI did not render correctly before repair (boot-loop, stock-screen clobbering,
> config crash, upside-down render, white-screen history, and Wi-Fi regression). 07-02 has
> build-passed and flashed software repairs for
> H1-H5, follow-up SD/RotateScreen blockers, the physical-presence API mutation gate, and the later
> LittleFS/legacy-SD code-review fixes. 07-01 has partial hardware evidence: the custom app verifies
> at `0x10000`, decoded LittleFS still verifies at `0x310000`, and the recovery web UI returns HTTP
> 200. Custom physical panel/touch/mining validation is still incomplete.
> Genuinely complete + device-independent: **Phases 1–2 only**. See STATE.md "HONEST FAILURE RECORD"
> + `phases/07-on-device-validation/07-REVIEW.md` (H1–H5).
>
> Route correction (2026-06-19): fork UI/touch repair is no longer the active
> implementation route. Continue Phase 7 as official `v2.0.02` ST7789 binary
> decompilation/custom-edit. Use fork failures only as recovery/reversibility
> context.

- [x] **Phase 1: RE-Spec the UI (read-only)** - Ghidra/FIDB decompile the located screen funcs into SPEC.md; confirm LVGL v8 vs v9. No flashing. ✅ done (device-independent)
- [x] **Phase 2: Recover Fonts/Glyphs + Filesystem Assets** - Extract LVGL font C-arrays + palette; unpack LittleFS; byte-accurate `.ss` decoder for all 17 screensavers. ✅ done (device-independent)
- [~] **Phase 3: Rebuild Core Screens + UIManager Carousel** - Loading/Miner/Clock/Config screens + page list + LastPage NVS persistence + branding splash. ⚠️ COMPILE-ONLY; white-screen/render failures were repaired in code, but physical panel proof is still pending.
- [~] **Phase 4: Restore Interactive Touch (SoftSPI)** - Historical SoftSPI touch plan; current source is no-VSPI HSPI/direct-probe touch. ⚠️ COMPILE-ONLY; current hardware evidence is no-contact/no-press below calibration, requiring physical XPT2046/PENIRQ/FPC probe or touch-variant re-identification.
- [~] **Phase 5: Data-Driven Screens + HTTP API Parity** - SPrice/Weather/screensaver/Swarm/API. ⚠️ COMPILE-ONLY; unverified on-device (UI doesn't render to confirm).
- [~] **Phase 6: Wire Custom Features at the Share-Accept Seam** - Speaker/SD/LED/branding. ⚠️ Partial on-device: SD mount confirmed; red config/connecting LED is expected but does not prove mining/share LED behavior. Speaker/beep/share LED/branding remain unverified until Wi-Fi/pool/mining runtime.
- [~] **Phase 7: Official v2.0.02 Binary Customization** - Active. Baseline
  provenance is locked; decompilation/custom-edit map and local patch tooling
  are pending.
- [ ] **Phase 8: Reserved / Superseded Fallback Slot** - Not active. The
  former Path-A contingency is now folded into Phase 7 as the primary route.

## Phase Details

### Phase 1: RE-Spec the UI (read-only)
**Goal**: The closed stock binary is turned into a written design contract that all UI rebuild work can be driven from, with the LVGL version question settled authoritatively.
**Depends on**: Nothing (first phase; RE foundation already on disk — DROM/IROM isolated, IROM disassembled, screen funcs located)
**Requirements**: RE-01, RE-02, RE-03, RE-04
**Success Criteria** (what must be TRUE):
  1. A Ghidra 11.3.1 + dynacylabs flash-loader v11.3.1 (prebuilt) workbench opens the stock image with DROM (`0x3f400020`) / IROM (`0x400d0020`) mapped; the self-built ESP-IDF v4.4.6 FIDB is attempted, and stripped SDK/Arduino-core symbols are recovered via the FIDB where it applies OR via a documented fallback (loader ROM/SVD + string→`l32r` xref) sufficient to decompile the screen/render/share-accept functions.
  2. The stock LVGL major version (v8 vs v9) is stated citing at least two independent items from the acceptable-evidence set (decoder/render call-shape; descriptor layout; constructor API signatures; DROM string family `LV_IMG_CF_*`/`LV_COLOR_FORMAT_*`; build-era strings) with a Confidence label, before any extractor is written.
  3. `hardware/recovered-assets/SPEC.md` exists and documents the screen inventory, per-screen widget geometry, fonts/colors, market field set, and screensaver render path, decompiled from `Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`, and the render / share-accept / screensaver paths.
  4. SPEC.md is markdown-only, never linked into firmware, and all recovered closed-firmware assets remain gitignored (no redistribution).
**Plans**: 3 plans
Plans:
- [ ] 01-01-PLAN.md — Ghidra 11.3.1 + dynacylabs v11.3.1 workbench, image import, ESP-IDF v4.4.6 FIDB (time-boxed), reusable xref script (RE-01)
- [ ] 01-02-PLAN.md — LVGL v8-vs-v9 verdict citing two items from the acceptable-evidence set, seeds SPEC.md metadata (RE-02)
- [ ] 01-03-PLAN.md — locate all 6 screens + seam + ss_render, deep-spec Miner+Clock, write SPEC.md (RE-03, RE-04)
**Research**: recommended (FIDB build is a non-trivial Tarlogic workflow; per-screen geometry extraction is novel for this project)

### Phase 2: Recover Fonts/Glyphs + Filesystem Assets
**Goal**: The reusable visual assets that ARE extractable from the binary (fonts, palette, screensavers, web UI) are recovered cleanly and ready to feed the rebuild.
**Depends on**: Phase 1 (SPEC.md identifies which fonts/glyphs/assets matter and the confirmed LVGL version)
**Requirements**: ASSET-01, ASSET-02, ASSET-03
**Success Criteria** (what must be TRUE):
  1. The stock LVGL font(s)/glyphs are extracted as reusable LVGL C-arrays (or a matched substitute is documented), together with the stock color palette.
  2. The LittleFS partition is unpacked with LittleFS tooling (NOT mkspiffs), yielding the gzipped web UI plus the 17 `/ss/saver_320_240_*.ss` screensavers.
  3. A byte-accurate `.ss` decoder (extending `scripts/nmminer_ss_decode.py`) decodes all 17 stock screensavers cleanly and is reusable by the firmware screensaver carousel.
**Plans**: 2 plans
- [ ] 02-01-PLAN.md — LittleFS manifest + .ss decoder regression test (bit-replication fix) + ss_render C-port spec (ASSET-02, ASSET-03)
- [ ] 02-02-PLAN.md — Identify stock fonts, enable/regenerate Montserrat + FA subset C-arrays, palette header (ASSET-01)

### Phase 3: Rebuild Core Screens + UIManager Carousel
**Goal**: Booting the fork reproduces the stock NMMiner structural identity — the named screens, the page carousel, and last-page persistence — driven by recovered evidence.
**Depends on**: Phase 1 (SPEC.md geometry), Phase 2 (fonts/palette)
**Requirements**: UI-01, UI-02, UI-03, UI-04, UI-05, UI-06
**Success Criteria** (what must be TRUE):
  1. On boot the device shows a Loading screen reproducing the stock look (NMMiner / "Make it better", firmware version, connection progress) with kleinpanic branding layered on.
  2. A `UIManager` registers the 6 named screens (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) as a page list with `LastPage` NVS persistence, resuming on the last page after reboot.
  3. The Miner screen renders the full `/api/system/info.miner` field set (hashrate, shares R/A, net/pool/last/best diff session + all-time, blkhits, freeheap, RSSI, temp, pool URL) matching stock formatting.
  4. The Clock screen renders NTP time (12/24h + date format), date, and BTC price with green-up / red-down coloring, and a Config screen shows the config-mode indicator with AP `nmap-2.4g` + captive portal at `192.168.4.1`.
  5. All screens are rebuilt natively from SPEC.md on the `NerdMiner-CYD-Custom` fork (recovered evidence, not imagination or mis-carved assets).
**Plans**: 6 plans
Plans:
- [ ] 03-01-PLAN.md — LVGL-over-TFT_eSPI bridge for CYD + boot brightness ramp (black-screen fix) + resolve Clock price colors
- [ ] 03-02-PLAN.md — UIManager: 6-page carousel + nm_screen_t contract + LastPage NVS persistence + boot routing
- [ ] 03-03-PLAN.md — Loading + Config screens from SPEC geometry (+ kleinpanic branding)
- [ ] 03-04-PLAN.md — Miner screen: full SPEC layout + /api/system/info.miner field set
- [ ] 03-05-PLAN.md — Clock screen (NTP + green-up/red-down price) + SPrice/Weather page stubs
- [ ] 03-06-PLAN.md — Integration build (pio run -e ESP32-2432S028R SUCCESS) + branding pass + 03-VALIDATION.md
**UI hint**: yes

### Phase 4: Restore Interactive Touch (SoftSPI)
**Goal**: The user can navigate the rebuilt UI by touch and boot button. Historical Phase 4 targeted software SPI to free VSPI for SD; current Phase 7 source supersedes that implementation with no-VSPI HSPI `TFT_eTouch` plus direct probes and still fails at raw no-contact.
**Depends on**: Phase 3 (screens + carousel to navigate). MUST come before Phase 6 SD wiring / bus-ownership lock-in. Current evidence requires physical touch-controller probing before more touch-code tuning.
**Requirements**: TOUCH-01, TOUCH-02, TOUCH-03
**Success Criteria** (what must be TRUE):
  1. Touch works on the current no-VSPI touch path, or the board is re-identified as a different/no-touch variant. Historical SoftSPI criteria are superseded by Phase 7 no-contact evidence.
  2. Short touch = next page, long-press (10s) = config mode, tap wakes a sleeping screen.
  3. Boot button: double-click = next page, long-press = force config, single-click = wake screen.
**Plans**: 2 plans
Plans:
- [ ] 04-01-PLAN.md — Historical plan: enable touch for CYD env + SoftSPI bit-bang XPT2046 driver (nm_touch) + raw-Z diagnosis + rotation-1 calibration; superseded at runtime by Phase 7 no-VSPI HSPI/direct probes (TOUCH-01)
- [ ] 04-02-PLAN.md — nm_display wake API + nm_uimanager touch/button nav + idle-dim + autoadvance OFF + boot-button re-point + final build gate + 04-VALIDATION (TOUCH-02, TOUCH-03)
**UI hint**: yes

### Phase 5: Data-Driven Screens + HTTP API Parity
**Goal**: The page-gated market/weather/swarm/screensaver behaviors and the full HTTP API match stock cadence and shapes so the existing dashboard/controller contract holds.
**Depends on**: Phase 3 (carousel), Phase 4 (touch wake/nav), Phase 2 (`.ss` decoder)
**Requirements**: DATA-01, DATA-02, DATA-03, DATA-04, DATA-05, DATA-06
**Success Criteria** (what must be TRUE):
  1. The SPrice screen renders Binance (`data-api.binance.vision`) kline candlestick + pricewall modes, page-gated to run only on Miner/Clock/Price.
  2. The Weather screen renders open-meteo forecast/AQI (°C/°F, wind, sunrise/sunset) with a rate-limited `/api/weather/refresh`.
  3. The screensaver carousel renders decoded `.ss` images after idle timeout, supports black mode and timeout prefs, exposes upload/preflight endpoints, and resumes on touch.
  4. Swarm LAN discovery (Miner-page-gated) plus `/api/swarm/find` FindMe overlay visibly overrides an active screensaver.
  5. The HTTP API matches stock paths/shapes (`/probe`, `/alive`, `/api/system/info`, `/api/setting/*`, `/api/market/pairs`, `/api/weather/refresh`, `/api/swarm/find`, `/api/system/restart`, screensaver preflight/upload), and branding/identity strings (`model="NMMiner"`, "Make it better") are preserved.
**Plans**: 6 plans
- [ ] 05-01-PLAN.md — NVS settings foundation (nm_stock namespace, stock field set + ss-timeout map)
- [ ] 05-02-PLAN.md — Binance market fetch + SPrice kline/pricewall screen (DATA-01)
- [ ] 05-03-PLAN.md — open-meteo weather fetch + Weather screen (DATA-02)
- [ ] 05-04-PLAN.md — .ss screensaver carousel + swarm scan + idle/FindMe state machine (DATA-03, DATA-04)
- [ ] 05-05-PLAN.md — HTTP API parity + branding + swarm/upload endpoints (DATA-04, DATA-05, DATA-06)
- [ ] 05-06-PLAN.md — wire modules into loop + integration build gate + flash budget (DATA-01..06)
**Research**: recommended (`.ss` header byte-layout, kline rendering specifics, idle→screensaver state machine are open)
**UI hint**: yes

### Phase 6: Wire Custom Features at the Share-Accept Seam
**Goal**: Our custom speaker / SD / LED / branding light up over the recovered UI at the in-process NerdMiner share-accept seam, without breaking mining.
**Depends on**: Phase 4 touch bus ownership work (historically SoftSPI; currently no-VSPI HSPI touch with SD owning VSPI), Phase 3/5 (UI to brand). Custom feature code already built+tested in the fork.
**Requirements**: HW-01, HW-02, HW-03, HW-04
**Success Criteria** (what must be TRUE):
  1. The speaker (LEDC GPIO26) beeps on an accepted share and alarms on pool disconnect, wired at the NerdMiner share-accept seam (in-process call, not an Xtensa hook).
  2. The SD card (VSPI GPIO5/18/19/23) mounts and logs on the now-free hardware VSPI.
  3. Custom LED behavior and kleinpanic branding accents are integrated over the recovered UI, verified against silkscreen pins.
  4. Mining stays fully functional throughout (NerdMiner stratum core unchanged; primary + fallback pool, switch-back to primary ≤10s).
**Plans**: 2 plans
Plans:
- [ ] 06-01-PLAN.md — Name the share-accept beep (Speaker::beepShareAccepted, D-62) + add nm_led mining/share/disconnect status (HW-01, HW-03, HW-04)
- [ ] 06-02-PLAN.md — Graceful at-seam SD log on VSPI + full build gate + mining.cpp diff gate (HW-02, HW-04)
**UI hint**: yes

### Phase 7: Official v2.0.02 Binary Customization
**Goal**: The official `v2.0.02` ST7789 binary is decompiled and mapped deeply enough to support local custom edits with no license-path changes, no secrets, no redistribution, and a reversible device-write gate.
**Depends on**: Phase 1 RE foundation, Phase 2 asset recovery, and the locked official backup/bundle provenance in `07-BINARY-BASELINE-ROUTE.md`.
**Requirements**: BIN-01, BIN-02, BIN-03, BIN-04, BIN-05, VALID-01, VALID-03
**Success Criteria** (what must be TRUE):
  1. The official `v2.0.02` ST7789 baseline is locked to a full-device backup,
     local bundle hashes, image metadata, and extracted DROM/IROM artifacts.
  2. Ghidra/decompiler entries are normalized for all six screens plus dynamic
     Miner/Clock/Loading/Weather/SPrice, `ss_render`, and share-accept paths.
     Current status: screen builders/setters, Miner binding fan-out plus
     candidate producer blocks, Clock BTC text route, SPrice kline/candle
     anchors, and share-result format callsites are mapped; proof/safety gaps
     remain before function edits.
  3. A binary edit target map identifies safe edit sites, replacement
     constraints, checksum/image-validation work, and explicit no-touch zones.
     Current status: literal candidates and function/callsite navigation records
     exist; function edits are not approved.
  4. Local-only patch tooling can produce and validate modified artifacts while
     keeping backups, recovered assets, and patched binaries out of git. Current
     status: fixed-length DROM literal patching is implemented only for approved
     targets.
  5. Any future device write uses a fresh 4 MB backup, `verify_flash`, hard
     power-cycle, and preserved restore command.
**Plans**: 2 plans
Plans:
- [ ] 07-01-PLAN.md — Official `v2.0.02` binary decompilation/custom-edit
  route; original fork flash/visual runbook is superseded unless needed for
  restore/reversibility.
- [x] 07-02-PLAN.md — Historical fork UI/touch repair detour; keep as failure
  evidence only.

### Phase 8: Reserved / Superseded Fallback Slot
**Goal**: No active work. The old fork-era binary-patch contingency is superseded because official binary customization is now Phase 7's primary route.
**Depends on**: Phase 7
**Requirements**: FALLBACK-01 (superseded)
**Success Criteria** (what must be TRUE):
  1. No work is routed here unless Phase 7 is explicitly replaced later.
  2. If reused, the new scope must keep the same no-secrets, no-license-path,
     no-redistribution constraints.
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. RE-Spec the UI | 3/3 | Complete (device-independent) | 2026-06-18 |
| 2. Recover Fonts/Assets | 2/2 | Complete (device-independent) | 2026-06-18 |
| 3. Core Screens + Carousel | 6/6 | Compile/code-review complete; failed on-device in Phase 7 | 2026-06-18 |
| 4. Restore Touch (SoftSPI) | 2/2 | Compile/code-review complete; touch no-press physical probe/re-identification pending | 2026-06-18 |
| 5. Data Screens + API | 6/6 | Compile/code-review complete; live device validation pending | 2026-06-18 |
| 6. Custom Features | 2/2 | Compile/code-review complete; partial live evidence only | 2026-06-18 |
| 7. Official v2.0.02 Binary Customization | 1/2 | In progress; baseline locked, runtime anchors and candidate producer/caller blocks mapped, proof/safety decompilation active | - |
| 8. Reserved / Superseded Fallback Slot | 0/0 | Not active | - |

## Coverage

- v3.0 requirements: 35
- Mapped to phases: 35
- Unmapped: 0 ✓

Full mapping in `REQUIREMENTS.md` Traceability table.
