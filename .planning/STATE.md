---
gsd_state_version: 1.0
milestone: v3.0
milestone_name: NMMiner RE + Custom Integrations
status: planning
last_updated: "2026-06-18T00:53:15.366Z"
last_activity: 2026-06-18
progress:
  total_phases: 0
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

# State

Last updated: 2026-06-04 (Plan 08-01 execution).

## Completed (Phase 8, Plan 01)

**2026-06-04:** Phase 8 Plan 1 executed (08-01): PlatformIO setup and NerdMiner_v2 fork.

- Tasks 1-3, 5 complete: PlatformIO installed, NerdMiner cloned, ST7789 verified, secrets .gitignore hardened.
- Task 4 (baseline build) blocked by upstream TFT_eSPI configuration issue (documented in SUMMARY.md).
- Commit: 1b9ee59 (feat(08-01): establish PlatformIO build environment and clone NerdMiner_v2 fork).
- Baseline firmware compilation deferred; all environment and repository setup complete.

## Prior Completed

- Git repository initialized.
- Manual PDF downloaded to `docs/vendor/NM-CYD-ESP32-2432S028-manual.pdf`.
- Manual text extracted to `docs/vendor/NM-CYD-ESP32-2432S028-manual.txt`.
- Official GitHub README, API reference, flasher JS, and version manifest saved under `docs/research/`.
- Device detected on `/dev/ttyUSB0`.
- Full 4 MB flash backup completed.
- Backup SHA-256: `a8c7cae425f1bb206d49adf4be802bda91b626035a1bf5c1d9fafd0cfe6715e4`.
- Original firmware: `v1.8.28`.
- Latest official firmware: `v2.0.02`.
- Display target: `esp32-2432s028r-st7789`.
- `nmap-2.4g` AP observed locally.
- User clarified that `Homebase` exists and is the 2.4 GHz SSID.
- `NMController_web` README, main Python file, and web monitor template snapshots downloaded.
- Firmware upgraded to official `v2.0.02` using the `esp32-2432s028r-st7789` target.
- License verification passes after upgrade.
- Serial config sent for `Homebase` without disconnecting the laptop from `Homebase5`.
- Device joined LAN at `192.168.1.136`.
- API verified firmware `v2.0.02`, hostname `cyd2.8_1a76f7`, SSID `Homebase`, timezone `-4`, and NTP sync.
- Recovered BTC wallet is configured for both primary and fallback pools.
- Runtime pool testing selected `pool.nerdminers.org:3333` for both primary and fallback so both configured URLs match the Amazon manual PDF and the only PDF-listed pool that completed live subscribe testing.
- Post-reboot API showed active mining on `pool.nerdminers.org:3333`, accepted shares increasing, and zero rejected shares.
- Customization audit completed in `docs/customization-audit.md`.
- Fresh official flasher snapshots match saved snapshots; `v2.0.02` still supports `esp32-2432s028r-st7789`.
- Main firmware repo audit found prebuilt firmware binaries but no published ESP32 firmware source tree.
- LittleFS extraction confirmed embedded web UI and screensaver assets are inspectable.
- GitHub profile review completed for `kleinpanic` and saved in `docs/kleinpanic-customization-basis.md`.
- Official NMminer1024 repo review confirmed `NMminer1024/NMMiner` is the repo for this ESP32 software bundle; no separate public source repo for this CYD firmware was found.
- Confirmed the running `/home/broklein/codeWS/Python/NMController_web` monitor pre-existed this repo work and is a clean upstream checkout, not a customized local dashboard.
- Canonical local dashboard added under `apps/nmminer-control`.
- Local dashboard is running at `http://127.0.0.1:8788`.
- Dashboard implementation split live status polling from settings loading to reduce request pressure on the ESP32.
- A normal API restart recovered mining after a temporary zero-hash state; post-restart readbacks showed about `1.04 MH/s` and accepted shares increasing.
- Rendered visual audit completed for the local dashboard at desktop and mobile widths with no console errors or horizontal overflow.
- Dashboard polish added safe device personalization controls for hostname and time display, fixed partial API error rendering, slowed live polling to reduce ESP32 load, and documented LAN access limits.
- Device display/weather preferences were configured: 12-hour time, Eastern offset, US date format, 70% brightness, LED enabled, image screensaver after 5 minutes, and Fahrenheit/mph weather for Washington, DC.
- Current firmware exposes brightness and timed screensaver behavior, but not the old `AutoBrightness`/`ScreenTimeout` settings found in older firmware strings.
- Laptop wallpaper images were converted into valid NMMiner `.ss` screen savers with `scripts/nmminer_screensaver.py`; generated batches are local artifacts and gitignored.
- Screen saver uploads are stored on the ESP32 internal LittleFS partition, not on SD/TF. Live readback after upload showed `393216` bytes total and `344064` used.
- Current NMMiner firmware should be treated as not using external humidity/sensor, speaker, or SD/TF workflows even though the CYD board family exposes those connectors.
- 2026-06-03: User physically installed a microSD card and a speaker on the CYD board (a Flipper Zero is also plugged into the laptop but is unrelated/charging).
- 2026-06-03: Firmware feasibility researched (Brave + Firecrawl + local binary recon). Findings in `docs/firmware-customization-feasibility.md`: stock NMMiner v2.0.02 is binary-only, unlicensed, and exposes zero SD/audio in its 21-endpoint HTTP API; its image already links the SD (VSPI/FatFS) and LEDC drivers but never uses them for files/audio.
- 2026-06-03 DECISION: do NOT extend the stock binary. Fork the MIT-licensed `BitMaker-hub/NerdMiner_v2`, target the ESP32-2432S028R ST7789 variant, and graft SD + speaker code from `witnessmenow/ESP32-Cheap-Yellow-Display`. Build custom features + our own web API and re-point the dashboard. Binary RE kept only as a fallback. Added as ROADMAP Phase 8.

## Needs User Input

- Hardware photos or pin measurements before wiring JST, CH1/CN1, P3, speaker, or SD/TF modifications.

## Risk Notes

- `Homebase5` was observed only on 5 GHz channels during local scan; use `Homebase` for this ESP32.
- Flash backup and recovered config contain secrets and are gitignored.
- Firecrawl could not scrape the Amazon page because the account has insufficient credits.
- Do not connect the laptop to the miner AP while it needs to stay online through `Homebase5`; use USB serial and LAN API.

## Session Continuity

Last session: 2026-06-17/18 (huge session — pivoted to v3.0 NMMiner reverse-engineering).

## Milestone v3.0 — NMMiner RE + Custom Integrations (status: planning → ready to execute)

**Decided:** Recover stock NMMiner v2.0.02's original look (closed binary) by RE-ing it as a SPECIFICATION, then rebuild its exact screens natively on our `firmware/custom/NerdMiner-CYD-Custom` fork (which already has working speaker/SD/LED/branding) + restore touch/market/screensaver. NOT a binary patch (rejected: fragmented flash, SHA check, no-redistribution, brick risk). NerdMiner fork = build base/fallback. Device currently mines on the fork (Homebase, 192.168.1.136).

**DONE this session:**

- v3.0 formalized in PROJECT.md + STATE.md (committed 662d075).
- 4-dimension research + synthesis committed: `.planning/research/{STACK,FEATURES,ARCHITECTURE,PITFALLS,SUMMARY}.md`.
- RE FOUNDATION (all on disk, recovered-assets gitignored): ESP32 image parsed → DROM `0x3f400020` + IROM `0x400d0020` isolated; **LittleFS extracted** (web UI + 17 screensavers); **`.ss` format fully decoded** → `scripts/nmminer_ss_decode.py` (all 17 decode clean); **IROM disassembled** (476k lines) via objcopy→ELF; **string→function xref works** — located `Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`. Pipeline doc: `docs/nmminer-re-pipeline.md`.

**NEXT (per research SUMMARY.md, ~8 phases):** finish milestone setup (REQUIREMENTS.md + roadmapper → ROADMAP.md) if missing; then: (1) Ghidra 11 + dynacylabs esp32 flash-loader + self-built ESP-IDF v4.4.6 FIDB → decompile located screen funcs → SPEC.md; (2) extract fonts (LVGL C-arrays) + assets; (3) rebuild core screens + UIManager carousel on fork; (4) restore touch on SoftSPI BEFORE SD; (5) market(Binance)/weather(open-meteo)/screensaver screens; (6) wire speaker/SD/LED/branding at share-accept seam; (7) on-device validate (needs /dev/ttyUSB0 + hard power-cycle between flashes); (8) fallback path-A spike only if needed.

**Fallback track (v2.0.02 fork):** task list item — device mines + speaker/API verified; on-device fork validation never formally shipped. Kept as working fallback.

Resume: `/gsd-resume-work`, then the `/goal` handed to the user (drives the full GSD phase lifecycle for v3.0).
Resume file: none.

## Current Position

Phase: Not started (defining requirements)
Plan: —
Status: Defining requirements
Last activity: 2026-06-18 — Milestone v3.0 started
