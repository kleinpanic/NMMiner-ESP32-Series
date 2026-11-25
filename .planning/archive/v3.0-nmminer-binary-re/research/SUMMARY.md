# Project Research Summary

**Project:** NMMiner ESP32 Series — Milestone v3.0 (Reverse-Engineer NMMiner v2.0.02 + Re-integrate Custom Speaker/SD/LED/Branding)
**Domain:** Closed ESP32 (Xtensa LX6) / Arduino-ESP32 / ESP-IDF v4.4.6 / LVGL 8.x firmware reverse-engineering → asset-informed native reimplementation on an owned MIT fork
**Researched:** 2026-06-17
**Confidence:** HIGH (stack/architecture/pitfalls verified against the real 4 MB backup + current upstream sources; features grounded in stock binary strings + saved API reference)

> Supersession, 2026-06-19: this is now a historical research snapshot. The
> active route is official NMMiner `v2.0.02` `esp32-2432s028r-st7789` binary
> decompilation/custom-edit, documented in
> `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md`,
> `07-BINARY-BASELINE-ROUTE.md`, `07-BINARY-DECOMP-TASKS.md`, and
> `07-BINARY-EDIT-TARGET-MAP.md`. Do not follow this file's old recommendation
> to make the fork rebuild primary. The locked workbench is Ghidra `11.3.1` plus
> dynacylabs loader `v11.3.1`, not `v12.0`; the active `v2.0.02` LittleFS
> geometry is `spiffs@0x390000` size `0x60000`.

## Executive Summary

This milestone recovers the *look and feel* of the closed NMMiner v2.0.02 firmware and re-adds our custom hardware features (speaker, SD, LED, kleinpanic branding) on top. The decisive research finding reshapes the strategy: **NMMiner's screens are runtime-drawn LVGL widgets, not stored bitmaps** — they cannot be lifted out as files. The "8 recovered PNGs" in `hardware/recovered-assets/` are mis-carved noise, and the v8 `lv_img_dsc_t` scan returned zero matches because the screen art lives in *code* (widget-construction calls), not in a scannable asset table. What *is* recoverable from the binary is the **design contract**: screen inventory (6 named LVGL pages), fonts/glyphs (extractable as LVGL C-arrays), per-screen layout geometry, the Binance/open-meteo data shapes, and the `.ss` screensaver format. The screensaver `.ss` format is NMMiner's own `SN@` RLE+LZSS encoding, already partly reverse-engineered by this project (`scripts/nmminer_screensaver.py`).

**Recommended approach: (B\*) Asset-Informed Reimplement.** Treat the closed binary as a read-only *specification* (via Ghidra 11.x native Xtensa + dynacylabs flash-loader v12.0 + a self-built ESP-IDF v4.4.6 FIDB), then rebuild NMMiner's exact screens natively as LVGL on the open `NerdMiner-CYD-Custom` fork — which already ships working, tested speaker/SD/LED/branding (v1.0.0 builds green, 73 tests @94% branch coverage). This is LVGL-on-LVGL: reproducing a layout is ordinary UI work driven by RE evidence, the custom hardware features are already built, and flashing the fork is fully reversible against the 4 MB backup (near-zero brick risk). **Binary-patching the stock image (Path A) is rejected as primary** — the ~787 KB free flash is fragmented across 135 non-contiguous runs, Xtensa offset/cache-alignment math is brittle, the image carries a SHA-256 integrity check that any byte-edit invalidates, the unlicensed binary's no-redistribution constraint applies, and every NMMiner update would nuke the patch. Path A survives only as a time-boxed last-resort spike if a NMMiner-exclusive behavior proves impossible to reproduce (none identified).

**Top risks and mitigations:** (1) Tooling correction — the "spiffs" partition is **LittleFS, not SPIFFS**; use `mklittlefs`/`littlefs-python`, not `mkspiffs`. (2) Image-integrity recon (`esptool image_info` + `espefuse summary`) must run *before* any device-write to settle the SHA/secure-boot question. (3) The CYD dual-SPI gotcha: touch (XPT2046) and SD both want VSPI, so **touch must be moved to SoftSPI before SD integration** or one peripheral kills the other. (4) The closed binary is unlicensed with per-device activation — keep any modified image out of git, personal-use only. (5) The WiFi radio is fragile under rapid reflash; hard power-cycle + `verify_flash` in every device-write phase.

## Key Findings

### Recommended Stack

The new capability for v3.0 is the **decompiler + symbol-recovery layer** (the project has only used `objdump` so far). Ghidra 11.x gained native Xtensa support in 11.0, and the entire ESP32 RE ecosystem (flash loader, FIDB method) is Ghidra-first. The highest-leverage step is a self-built ESP-IDF **v4.4.6** Function-ID database (no public Xtensa/IDF prebuilt exists) to auto-name stripped SDK/Arduino-core functions. Asset/filesystem extraction uses LittleFS tooling (the single most important tooling correction) plus the project's own `.ss` decoder. See `.planning/research/STACK.md`.

**Core technologies:**
- **Ghidra 11.4.x** (≥11.0) + **dynacylabs/ghidra-esp32-flash-loader v12.0** — primary decompiler workbench; loader auto-maps DROM `0x3f400020`/IROM `0x400d0020` and annotates ROM symbols + Espressif SVD registers. Requires JDK 21 (present).
- **Self-built ESP-IDF v4.4.6 FIDB** (Tarlogic method) — recovers SDK function names in the stripped image; must match the binary's IDF version. Highest-leverage symbol-recovery step.
- **mklittlefs / littlefs-python** (NOT mkspiffs) — unpack the LittleFS partition at `0x3e0000` (params `-b 4096 -p 256 -s 0x10000`); contains the gzipped web UI + `/ss/` screensavers.
- **tenable/esp32_image_parser** (`create_elf`) — alternative ELF-from-flash import path for Ghidra/IDA.
- **scripts/nmminer_screensaver.py** (extend with a decoder) — the only tool for the `SN@` RLE+LZSS `.ss` format; no third-party tool reads it.
- **Pillow + objdump** (already in use) — RGB565↔PNG conversion and fast linear-disasm cross-checks.

**LVGL version:** working hypothesis is **v8.x** (Arduino-ESP32 on IDF v4.4.6, Oct-2023 build predates LVGL v9.0). Confirm via FIDB-named `lv_img_decoder_*`/`lv_image_*` symbols and `LV_IMG_CF_*` vs `LV_COLOR_FORMAT_*` string family — not a struct scan.

### Expected Features

This is a **stock-parity recovery** target: the rebuild must "feel identical" to NMMiner v2.0.02. Features are grounded in stock binary strings + the 21-endpoint HTTP API reference + vendor manual. See `.planning/research/FEATURES.md`.

**Must have (table stakes — match stock exactly):**
- **UIManager page list + 6 named LVGL screens** (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) with `LastPage` NVS persistence — the structural identity; class names are the contract.
- **Touch + boot-button nav** — short touch = next page, long-press (10s) = config mode, tap wakes screen; double-click button = next page.
- **Miner stat screen** — full `/api/system/info.miner` field set (hashrate, shares R/A, net/pool/last/best diff session+all-time, blkhits, freeheap, RSSI, temp, pool URL).
- **Stratum mining core** — primary + fallback pool, switch-back to primary ≤10s.
- **Config/AP captive portal** — `nmap-2.4g` (open), 192.168.4.1, 15s WiFi-fail trigger.
- **21-endpoint HTTP API** (15 REST + 6 portal routes) at identical paths/shapes — controller + dashboard contract.
- **Branding strings** — `model="NMMiner"`, "Make it better" (controllers key off `model`).

**Should have (P2 — behavioral parity, after core):**
- **Clock screen** (NTP, 12/24h, date format, BTC price green-up/red-down).
- **Screensaver** — `.ss` decoder (`ss_render`, RGB565+RLE, magic `0x4E53`) + carousel + black mode + idle-timeout prefs + upload/preflight endpoints.
- **Market** (Binance `data-api.binance.vision`) — SPrice screen with kline + pricewall modes; **page-gated** (runs only on Miner/Clock/Price).
- **Weather** (open-meteo) — forecast/AQI screen + rate-limited refresh.
- **Swarm** — LAN scan (Miner-page-gated, 30s idle) + `/api/swarm/find` FindMe overlay (overrides screensaver).

**Defer / NEW re-add layer (our additions, post-parity):**
- **Speaker** (LEDC GPIO26 — beep on share, alarm on disconnect), **SD card** (VSPI — explicitly NOT stock; stock reserves the TF slot), **custom LED**, **kleinpanic splash**. All already built and tested in the fork.
- Differentiator polish (new-fw reminder, Christmas egg, auto-TZ, dark clock style).

**Anti-features:** do NOT reproduce the license/activation gate, do NOT patch the stock binary as primary, do NOT swap data sources (breaks number/cadence parity), do NOT abstract for 30+ boards (target one: `esp32-2432s028r-st7789`).

### Architecture Approach

The architecture is a one-way **RE-Spec → Custom-Firmware** pipeline. The closed binary's DROM/IROM are read-only spec sources that produce a static design contract (`hardware/recovered-assets/SPEC.md`); that contract informs native LVGL screen modules built on the fork. The mining core (NerdMiner, unchanged) emits a share-accept event consumed in-process by the already-built `speaker.cpp`/`sdstore.cpp` — the integration seam is a normal function call, not an Xtensa hook. See `.planning/research/ARCHITECTURE.md`.

**Major components:**
1. **RE Spec Layer** (read-only) — `objdump`/`strings`/Ghidra on `drom.bin`/`irom.bin` → screen inventory, layout geometry, fonts/colors, market + screensaver behavior, written to `SPEC.md` (markdown, never linked into firmware).
2. **LVGL UI Layer** (fork) — new `ui_*.c` modules following the existing `ui_HomeScreen.c` pattern, reproducing NMMiner's 6 screens pixel-close.
3. **Mining core** (NerdMiner upstream, unchanged) — hashing, stratum, share accept/reject, emits share-accept event.
4. **Custom feature layer** (ALREADY BUILT in fork) — `speaker.cpp` (LEDC GPIO26), `sdstore.cpp`/`SDCard.cpp` (VSPI 5/18/19/23), LED, custom JSON web API, branding splash.

### Critical Pitfalls

Top items from `.planning/research/PITFALLS.md`, sequenced for the roadmap:

1. **Wrong LVGL header format scan (v9 magic vs v8 packed bitfield).** Zero matches is the *tell*, not a dead end — NMMiner is v8 (no magic byte; recognize the 12-byte `lv_img_dsc_t` by structure: `always_zero`==0, valid `cf`, plausible `w`/`h`, `data` pointer inside `[0x3f400020, +len(drom))`, `data_size == w*h*bpp`). Fonts are the extractable LVGL C-arrays; screens are not.
2. **Image SHA-256 integrity check ("image is corrupt").** ESP-IDF images can carry an appended SHA-256; any byte-patch invalidates it. **Run integrity recon BEFORE any patching** — `esptool image_info` + `espefuse summary` to settle hash/secure-boot/encryption. (Plaintext strings already recovered = encryption is off; document it.) Only relevant if the Path-A spike is ever attempted.
3. **CYD dual-SPI collision: touch + SD both want VSPI.** Touch (XPT2046) is on VSPI (GPIO25/32/39/33/36); SD is on default VSPI (GPIO5/18/19/23). **Restore touch on SoftSPI BEFORE SD integration** so SD owns hardware VSPI cleanly. Diagnose touch via raw Z-pressure first (bus vs calibration).
4. **Closed-binary licensing.** Unlicensed binary + per-device activation = no-redistribution. Keep any modified image gitignored, personal-use only; never touch the license-verification code path. Record the constraint in requirements.
5. **WiFi/flash fragility under iterative reflash.** Hard power-cycle (pull USB, clear RTC/PHY) between flash cycles, power from a quality port, and `esptool verify_flash` after every write. Bake into the flash runbook for every device-write phase.

## Implications for Roadmap

Based on combined research, suggested phase structure. The dependency spine is: **RE-spec the binary → recover fonts/palette → rebuild core screens → add data-driven screens → restore touch → wire custom HW → on-device validate.** Note the current hard blocker: on-device validation is blocked until the USB-DATA port (`/dev/ttyUSB0`) appears (per STATE.md).

### Phase 1: RE-Spec the UI (read-only)
**Rationale:** Everything visual depends on the design contract; the screens are code, so the spec must be extracted before any rebuild. This is where the Ghidra/FIDB stack pays off and where the v8-vs-v9 question is settled authoritatively.
**Delivers:** `hardware/recovered-assets/SPEC.md` — screen inventory, per-screen widget geometry, fonts/colors, market field set, screensaver render path. No flashing.
**Uses:** Ghidra 11.x + dynacylabs loader v12.0 + ESP-IDF v4.4.6 FIDB; objdump cross-check.
**Avoids:** Pitfall 1 (confirm v8 first; recognize descriptors by structure, not magic). Anti-pattern: treating mis-carved PNGs as real screens.

### Phase 2: Recover Fonts/Glyphs + Palette + Filesystem Assets
**Rationale:** Fonts ARE extractable (LVGL C-arrays in IROM), unlike the screens; the LittleFS partition holds the gzipped web UI + `.ss` screensavers. Recovering these feeds the UI rebuild and the screensaver/web-API parity work.
**Delivers:** Recovered/substituted LVGL font(s) + color scheme; unpacked LittleFS contents; extended `.ss` decoder.
**Uses:** mklittlefs/littlefs-python (NOT mkspiffs), `scripts/nmminer_screensaver.py` decoder, Pillow RGB565→PNG, binwalk triage.
**Avoids:** Pitfall 1 (LittleFS tooling correction).

### Phase 3: Rebuild Core Screens on the Fork
**Rationale:** Highest-value, simplest screens first (Clock + Stats/Home + Loading) on the LVGL-native fork, driven by `SPEC.md`. Establishes the page carousel + UIManager + LastPage identity.
**Delivers:** `ui_*.c` modules for the structural screens + page-list/persistence machinery + branding splash; Miner stat screen with full field set.
**Implements:** LVGL UI Layer; UIManager page list (table-stakes structural identity).
**Avoids:** Anti-pattern 3 (rebuild from `SPEC.md`, not imagination).

### Phase 4: Restore Interactive Touch (SoftSPI)
**Rationale:** Touch must work before SD integration (bus-ownership lock-in) and before the UI can "feel" stock; the open fork's touch is project-observed broken on this unit.
**Delivers:** Working short/long-press page nav + config entry + wake, touch on SoftSPI leaving hardware VSPI free.
**Avoids:** Pitfall 3 (touch-on-SoftSPI decision committed before SD); diagnose raw-Z first.

### Phase 5: Add Data-Driven Screens (Market / Weather / Swarm / Screensaver)
**Rationale:** These reuse recovered API shapes + the `.ss` decoder and are page-gated behaviors; they layer onto the working carousel from Phase 3.
**Delivers:** SPrice (kline + pricewall, Binance), Weather (open-meteo), Swarm scan + FindMe overlay, screensaver carousel + black mode + upload/preflight endpoints; matching 21-endpoint HTTP API.
**Implements:** Market/screensaver/weather/swarm behavior from FEATURES.md, with thread-gating parity.

### Phase 6: Wire Custom Features (mostly done) + Re-add Layer
**Rationale:** The hard custom work (speaker/SD/LED/branding) is already built and tested in the fork; this phase confirms the integration seam at the NerdMiner share-accept site and lights up SD on the now-free VSPI.
**Delivers:** Beep-on-share + disconnect alarm (LEDC GPIO26), SD logging (VSPI), custom LED, kleinpanic accents — verified against silkscreen pins.
**Implements:** Custom feature layer (the NEW re-add goal).
**Avoids:** UX pitfall (implement beep AND disconnect-alarm hooks together).

### Phase 7: On-Device Validation (BLOCKED on USB-DATA port)
**Rationale:** Backup → flash fork → serial + visual checklist is the only proof of parity, but is gated on `/dev/ttyUSB0` appearing (current STATE.md blocker).
**Delivers:** Flashed device, serial-confirmed boot, visual parity checklist, `verify_flash` confirmation.
**Avoids:** Pitfall 5 (hard power-cycle + verify_flash in the runbook for every write).

### Phase 8 (Fallback only): Path-A Binary-Patch Spike
**Rationale:** Time-boxed last resort, attempted ONLY if Phase 1 reveals a NMMiner-exclusive behavior unreproducible on the fork (none identified). Requires Pitfall-2 integrity recon first.
**Delivers:** A patch proof against the free-flash cave, or an abandon-decision.
**Avoids:** Pitfalls 2 + 4 (re-stamp SHA, never touch license path, never redistribute).

### Phase Ordering Rationale

- **Spec-before-build:** screens are runtime code, so the read-only RE spec is the gate for all UI work (ARCHITECTURE build-order step 1).
- **Touch-before-SD:** the VSPI bus-ownership decision (SoftSPI touch) must be locked before SD wiring is committed (PITFALLS sequencing).
- **Core-screens-before-data-screens:** the UIManager carousel + persistence is the structural foundation the page-gated market/weather/swarm behaviors hang off (FEATURES dependency graph).
- **Custom-features-late:** they're already built; they integrate trivially as in-process calls once the UI + touch + mining are in place — no need to front-load.
- **Integrity-recon gates any patch:** the hash/secure-boot verdict decides whether the optional Path-A spike is even possible.

### Research Flags

Phases likely needing deeper research during planning (`/gsd-plan-phase --research-phase`):
- **Phase 1 (RE-Spec):** FIDB build is a non-trivial Tarlogic workflow (compile IDF v4.4.6 example projects -Os/-O2, populate FidDb); per-screen geometry extraction from disasm is novel for this project. **Research recommended.**
- **Phase 5 (data-driven screens):** `.ss` byte-accurate header layout beyond magic, exact kline rendering specifics, and idle→screensaver state machine are open questions needing live-device capture + binary RE. **Research recommended.**
- **Phase 8 (Path-A spike, if reached):** Xtensa patch hazards (variable-length instructions, windowed-ABI CALL range, L32R literal pools) + SHA re-stamp. **Research required if attempted.**

Phases with standard patterns (skip research-phase):
- **Phase 3 (core screens):** ordinary LVGL widget work following existing `ui_HomeScreen.c` scaffolding.
- **Phase 4 (touch):** well-documented CYD SoftSPI fix (RNT guides + marcelrv example).
- **Phase 6 (custom features):** already built and tested in the fork; integration is confirmation, not discovery.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Ghidra/loader/FIDB + LittleFS facts verified against current repos and the actual 4 MB backup; LVGL v8-vs-v9 is MEDIUM until confirmed in-decompiler (working hypothesis v8). |
| Features | HIGH | Grounded in stock binary strings + 21-endpoint API reference + vendor manual; some default *values* are MEDIUM (GET-example literals, flagged inline). |
| Architecture | HIGH | Backed by local binary recon of the real image — screens-are-code, free-flash fragmentation, and the already-built fork features all verified this session. |
| Pitfalls | HIGH | Technical pitfalls 1–3 verified against esptool/LVGL/CYD sources + project boot-log; legal (4) MEDIUM (terms not published); RF fragility (5) project-observed. |

**Overall confidence:** HIGH

### Gaps to Address

- **LVGL v8 vs v9 (MEDIUM):** confirm authoritatively in Phase 1 via FIDB-named decoder symbols + `LV_IMG_CF_*`/`LV_COLOR_FORMAT_*` string family before writing any extractor. Treat v8 as the hypothesis.
- **Per-screen layout geometry + stat-field formatting:** not literal in the binary; extract from disasm in Phase 1 and cross-check against live `/api/system/info` + on-device photos.
- **`.ss` header layout beyond magic `0x4E53`** (hs_params, dims, compression params): needs RE of `ss_render` for a byte-accurate decoder (Phase 2/5).
- **Idle→screensaver state machine** (does it dim before blanking?) and **factory-default `ScreenSaver` timeout** (`never` vs legacy 60s): verify on a freshly flashed unit, not the user's reconfigured device.
- **USB-DATA port blocker:** Phase 7 on-device validation cannot proceed until `/dev/ttyUSB0` (data, not charge-only) appears. All prior phases (RE-spec, font recovery, screen rebuild, custom-feature code) are software-complete without the device and should be front-loaded.

## Sources

### Primary (HIGH confidence)
- Local binary recon (this session): `hardware/recovered-assets/{app_factory,drom,irom}.bin`, partition table parse, `littlefs` magic, `SN@` `.ss` header, `0xFF`-run free-flash analysis, DROM string dump.
- `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin` — screen classes, internal labels, data-source URLs, ss_render, findme/swarm.
- `docs/research/nmminer-api-reference.md`, `nmminer-github-readme.md`, `docs/vendor/NM-CYD-ESP32-2432S028-manual.txt`, `docs/customization-audit.md`.
- NationalSecurityAgency/ghidra (native Xtensa >=11.0); dynacylabs/ghidra-esp32-flash-loader v12.0; tenable/esp32_image_parser; pioarduino/Jason2866 mklittlefs_esp32; littlefs-python (PyPI).
- esptool firmware-image-format docs (SHA-256 `hash_appended`); LVGL v8.3 `lv_img_buf.h`; RandomNerdTutorials CYD pinout + display+touch+microSD guides.

### Secondary (MEDIUM confidence)
- Tarlogic FIDB method (compile IDF v4.4.6 examples -Os/-O2; GhidraLinkerScript); threatrack (confirms no prebuilt Xtensa FIDB); Olof Astrand Ghidra/IDA ESP32 workflow; BlackVS/ESP32-reversing index.
- LVGL v8->v9 color-format rename docs; marcelrv CYD dual-SPI touch example.
- NMMiner repo + product page (binary-only, no LICENSE, per-device activation) — license terms not explicitly published.

### Tertiary (LOW confidence)
- Default-value literals from API GET examples (Brightness 80, RotateScreen 270, etc.) — need confirmation against a factory device.

---
*Research completed: 2026-06-17*
*Ready for roadmap: yes*
