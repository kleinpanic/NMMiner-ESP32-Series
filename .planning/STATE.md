---
gsd_state_version: 1.0
milestone: v3.0
milestone_name: — NMMiner RE + Custom Integrations
status: paused-validation-failed
last_updated: "2026-06-18T17:30:00.000Z"
last_activity: 2026-06-18 -- Phase 7 PAUSED by user; on-device validation FAILED, rebuilt UI does not render correctly
progress:
  total_phases: 8
  completed_phases: 2   # ONLY Phases 1-2 (RE-spec + asset recovery) are genuinely done; 3-6 were COMPILE-verified only and FAIL on-device (see Phase 7 audit)
  total_plans: 21
  completed_plans: 21   # plans executed + compiled; NOT on-device-validated
  percent: 40           # honest: device-rendering goal unmet
---

# State

Last updated: 2026-06-18 (ROADMAP.md created — 8-phase v3.0 spine).

## Completed (Phase 8, Plan 01)

**2026-06-04:** Phase 8 Plan 1 executed (08-01): PlatformIO setup and NerdMiner_v2 fork.

- Tasks 1-3, 5 complete: PlatformIO installed, NerdMiner cloned, ST7789 verified, secrets .gitignore hardened.
- Task 4 (baseline build) blocked by upstream TFT_eSPI configuration issue (documented in SUMMARY.md).
- Commit: 1b9ee59 (feat(08-01): establish PlatformIO build environment and clone NerdMiner_v2 fork).
- Baseline firmware compilation deferred; all environment and repository setup complete.

> NOTE: the "Phase 8" above belongs to the ARCHIVED v2.0.02 milestone
> (`.planning/archive/v2.0.02-fork-milestone/`). v3.0 phase numbering resets at 1.

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
- 2026-06-03 DECISION: do NOT extend the stock binary. Fork the MIT-licensed `BitMaker-hub/NerdMiner_v2`, target the ESP32-2432S028R ST7789 variant, and graft SD + speaker code from `witnessmenow/ESP32-Cheap-Yellow-Display`. Build custom features + our own web API and re-point the dashboard. Binary RE kept only as a fallback. (Archived milestone.)

## Needs User Input

- Hardware photos or pin measurements before wiring JST, CH1/CN1, P3, speaker, or SD/TF modifications.

## Risk Notes

- `Homebase5` was observed only on 5 GHz channels during local scan; use `Homebase` for this ESP32.
- Flash backup and recovered config contain secrets and are gitignored.
- Firecrawl could not scrape the Amazon page because the account has insufficient credits.
- Do not connect the laptop to the miner AP while it needs to stay online through `Homebase5`; use USB serial and LAN API.

## Session Continuity

Last session: 2026-06-18T07:04:44.085Z

## Milestone v3.0 — NMMiner RE + Custom Integrations (status: planning → ready to plan Phase 1)

**Decided:** Recover stock NMMiner v2.0.02's original look (closed binary) by RE-ing it as a SPECIFICATION, then rebuild its exact screens natively on our `firmware/custom/NerdMiner-CYD-Custom` fork (which already has working speaker/SD/LED/branding) + restore touch/market/screensaver. NOT a binary patch (rejected: fragmented flash, SHA check, no-redistribution, brick risk). NerdMiner fork = build base/fallback. Device currently mines on the fork (Homebase, 192.168.1.136).

**ROADMAP (8 phases, coarse, versioned `v3.0 / Phase N`):**

1. RE-Spec the UI (read-only) — RE-01..RE-04
2. Recover Fonts/Glyphs + Filesystem Assets — ASSET-01..ASSET-03
3. Rebuild Core Screens + UIManager Carousel — UI-01..UI-06
4. Restore Interactive Touch (SoftSPI) — TOUCH-01..TOUCH-03 (before SD)
5. Data-Driven Screens + HTTP API Parity — DATA-01..DATA-06
6. Wire Custom Features at the Share-Accept Seam — HW-01..HW-04
7. On-Device Validation — VALID-01..VALID-03 (BLOCKED on /dev/ttyUSB0)
8. Fallback Path-A Binary-Patch Spike (contingency) — FALLBACK-01

Coverage: 30/30 v3.0 requirements mapped, 0 unmapped.

**DONE prior session:**

- v3.0 formalized in PROJECT.md + STATE.md + REQUIREMENTS.md (committed).
- 4-dimension research + synthesis committed: `.planning/research/{STACK,FEATURES,ARCHITECTURE,PITFALLS,SUMMARY}.md`.
- RE FOUNDATION (all on disk, recovered-assets gitignored): ESP32 image parsed → DROM `0x3f400020` + IROM `0x400d0020` isolated; **LittleFS extracted** (web UI + 17 screensavers); **`.ss` format fully decoded** → `scripts/nmminer_ss_decode.py` (all 17 decode clean); **IROM disassembled** (476k lines) via objcopy→ELF; **string→function xref works** — located `Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`. Pipeline doc: `docs/nmminer-re-pipeline.md`.

**NEXT:** `/gsd-plan-phase 1 --research` (FIDB build + per-screen geometry extraction flagged research-recommended in SUMMARY.md). Phases 1-6 + 8 are device-independent and should be front-loaded; Phase 7 waits on `/dev/ttyUSB0`.

**Fallback track (v2.0.02 fork):** device mines + speaker/API verified; on-device fork validation never formally shipped. Kept as working fallback.

Resume: `/gsd-resume-work`.
Resume file: .planning/phases/05-data-driven-screens-http-api-parity/05-05-SUMMARY.md

## Current Position

Phase: 7 — PAUSED (user-requested) / ON-DEVICE VALIDATION FAILED
Status: milestone NOT complete. The rebuilt v3.0 UI does not render correctly on hardware.
Last activity: 2026-06-18 -- Phase 7 paused by user after on-device validation exposed systemic render failures.

## ⚠️ HONEST FAILURE RECORD (2026-06-18) — read before resuming

**What actually happened:** Phases 3–6 were marked "complete/verified" on the strength of
**compile-success + code-review only**. The actual on-device *rendering* was deferred to Phase 7.
When Phase 7 finally flashed to the real ESP32-2432S028R, the rebuilt LVGL UI **did not work**:

- **Boot-loop crash** — default 8 KB Arduino loopTask stack overflowed (LVGL + ArduinoJson +
  .ss decode) → "Stack canary watchpoint triggered" Guru Meditation → reboot loop → black screen.
  FIXED this session (SET_LOOP_TASK_STACK_SIZE 32 KB, commit 054e9f5).
- **Stock screens clobbering LVGL** — ungated stock TFT_eSPI loading/setup screens painted over
  the LVGL panel. Partly fixed (gated), but see CR-02 below.
- **Config-mode LoadProhibited crash** — drawSetupScreen rerouting to the LVGL config page from the
  WiFiManager callback dereferenced an unbuilt page → boot loop. FIXED (no-op), but CR-02 regression.
- **Upside-down render** — nm_display used setRotation(1); stock flips to 3. FIXED (commit b83a12a).
- **WHITE SCREEN (still broken at pause)** — loading + config screens set bg_color black WITHOUT
  bg_opa COVER (only Miner does it right). Dark-theme flip masked nothing; the real fix is H5.
- **WiFi regression (still broken)** — a bug THIS session introduced: "SD-authoritative config"
  (tryConfigFromSD) overrides NVS creds every boot + writes cleartext WifiPW to the card (CR-01).
  Probable cause of "Homebase connects once then drops to portal." Back this out (H1).
- **Config mode invisible** — the drawSetupScreen no-op removed the SSID/192.168.4.1 display with
  nothing replacing it (CR-02).
- **Touch** — calibration still for rotation 1, panel now rotation 3 (WR-02).

**Process failure (root):** the GSD lifecycle for Phases 3–6 used compile gates, not on-device
verification. "Verified 6/6 / UI 6/6" meant "compiled + code-reviewed," NOT "renders on hardware."
Resume MUST add on-device verification per screen as the gate (user-approved: "harden the LVGL
rebuild, on-device gated").

**Full audit + fix plan:** `.planning/phases/07-on-device-validation/07-REVIEW.md`
(2 Critical, 7 Warning, 5 Info; hardening tasks H1–H5).

### Resume plan (paused — do NOT auto-run; user controls cadence/limit)

1. **H1** — back out SD-authoritative config: NVS wins, SD is import-only; stop writing WifiPW to SD. (fixes WiFi regression CR-01)
2. **H5** — add `bg_opa COVER` to nm_screen_loading + nm_screen_config. (fixes white screen WR-01)
3. **H2** — restore config-mode on-screen feedback (SSID + 192.168.4.1) via the LVGL config path. (CR-02)
4. **H3** — re-derive touch calibration/axis-swap for rotation 3. (WR-02)
5. **H4** — make rotation + brightness runtime CONFIG, applied at the right boot point. (WR-02/WR-03)
6. Then: per-screen on-device verify (Loading/Miner/Clock/SPrice/Weather/Config), then VALID-01..03.

### Device / repo state at pause

- Fork HEAD: `b83a12a` (all session work committed; working tree clean).
- Device: running the BROKEN v3.0 build (white screen, not mining).
- Fresh 4 MB backup of the working firmware: `hardware/backups/cyd-backup-20260618-164206.bin`
  (sha `0aafd81d…`). Restore to a working miner:
  `esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 <that backup>`.
- SD card: formatted FAT32 (label NMMINER), inserted; mounts on-device (HW-02 confirmed live).
- /dev/ttyUSB0 present. ESP32-D0WD-V3, ILI9341 panel (confirmed via raw self-test).

### Earlier blocker (resolved)

- 05-05 dram0_0_seg overflow (D-58) — resolved during Phase 5 (s_pairs resize); build green at 90.5%.
