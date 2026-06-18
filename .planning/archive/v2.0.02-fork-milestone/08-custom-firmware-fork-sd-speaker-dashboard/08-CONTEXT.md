# Phase 8: Custom Firmware Fork (SD + Speaker + Dashboard) - Context

**Gathered:** 2026-06-03
**Status:** Ready for planning
**Mode:** Autonomous (decisions confirmed via user Q&A 2026-06-03)

<domain>
## Phase Boundary

Build a custom ESP32 miner firmware for THIS unit (ESP32-2432S028R "CYD", ST7789 panel) by forking the MIT-licensed `BitMaker-hub/NerdMiner_v2`, adding SD-card and speaker features the local web dashboard can drive, then releasing it as our own public GitHub fork — secure, optimized, tested, and validated.

Rationale (locked in `docs/firmware-customization-feasibility.md`): stock NMMiner v2.0.02 is binary-only, unlicensed, and exposes zero SD/audio in its HTTP API. NerdMiner v2 is MIT, PlatformIO/Arduino, already supports the ESP32-2432S028R board and SD `config.json`; the CYD reference repo `witnessmenow/ESP32-Cheap-Yellow-Display` provides working SD + speaker code for this exact board.

IN SCOPE: firmware fork + custom features, custom web/JSON API, dashboard rework + UI upgrade + Playwright visual validation, build/compile validation, security + optimization passes, tests, public GitHub release.
OUT OF SCOPE (this phase): binary reverse-engineering of NMMiner (fallback only); on-device LVGL deep redesign; ASIC/NMAxe.
</domain>

<decisions>
## Implementation Decisions (confirmed)

### Base fork
- Fork `BitMaker-hub/NerdMiner_v2` (MIT). Clone into `firmware/custom/` (or `external/NerdMiner_v2` for upstream reference + a working copy we own). Track upstream as a remote for future merges.
- Display target: **ST7789** ESP32-2432S028R. Mainline NerdMiner CYD env defaults to ILI9341 — must override TFT_eSPI driver/setup to ST7789 to match this unit. Verify against device (stock firmware target was `esp32-2432s028r-st7789`).

### Speaker (ALL behaviors confirmed) — passive speaker/buzzer via LEDC `tone()`, lead pin GPIO26 (verify on board)
- Beep on accepted share.
- Alarm tone on pool/WiFi disconnect.
- Boot/startup chime on power-up / successful pool connect.
- Website test-tone button + volume/mute control.
- Implement as a small `Speaker`/`Buzzer` module (non-blocking, LEDC channel that does not conflict with TFT backlight PWM channel). Crib from witnessmenow CYD audio example.

### SD card (ALL uses confirmed) — microSD over VSPI (CS=GPIO5, CLK=18, MISO=19, MOSI=23; verify on board), FAT32
- Stats logging to CSV (hashrate/shares/temps/uptime), downloadable via website.
- Config store: read/write `config.json` (WiFi + pool + wallet) on SD (NerdMiner supports this).
- Website file browser: list / download / delete SD files via dashboard.
- Screensaver/image store on SD.
- Implement an `SDStore` module + web endpoints. Crib SD init from witnessmenow CYD example.

### Mining identity
- KEEP current pool + recovered BTC wallet: `pool.nerdminers.org:3333`, recovered BTC mainnet address. These are applied to OUR build only.

### Custom web/JSON API (we own it)
- `GET /api/status` → mining stats + `sd` block (mounted, total/used/free) + `speaker` block (enabled, volume).
- `GET/POST /api/speaker` → test tone, volume, mute, per-event enables.
- `GET /api/sd/list`, `GET /api/sd/download?path=`, `POST /api/sd/delete`, `GET /api/sd/log` → file browser + CSV log.
- Keep config endpoints for pool/wallet/wifi (or reuse NerdMiner's portal).

### Dashboard (`apps/nmminer-control`)
- Re-point/extend to the custom firmware API. Add SD file-browser panel, speaker control panel, live stats.
- UI upgrade (visual polish) + Playwright MCP visual validation at desktop + mobile widths (mirror prior `docs/ui-review-nmminer-control.md` rigor).

### Release
- **Public** GitHub fork under `kleinpanic`. MIT retained + NOTICE/attribution to BitMaker-hub. Confirm-before-push (push is the one outward-facing gate).
- Repo name proposal: `NerdMiner-CYD-Custom` (or user override).

### Security / secrets (hard constraint)
- NO secrets in the public repo: WiFi password, license, flash backup, serial logs stay gitignored. Personal WiFi creds + wallet provided via gitignored local config header AND/OR SD `config.json` — committed repo ships only `*.example` placeholders.
- BTC address is public data (safe to include) but default to placeholder in committed config; real values in gitignored local config for our build.

### Validation gates
- BUILD/COMPILE validation is the automated gate (PlatformIO build for the ST7789 CYD env must succeed). PlatformIO installed into `.venv` (`pip install platformio`).
- FLASHING the device is HARDWARE-GATED and overwrites the working miner — confirm before flashing; full flash backup (`hardware/backups/...-2026-06-01.bin`) is the rollback.
- On-device validation (speaker audible, SD reads, shares accepted) requires a human at the device — flag as human_needed.
</decisions>

<code_context>
## Existing Code Insights
- `apps/nmminer-control/` — Flask proxy dashboard (server.py, static/{index.html,app.js,styles.css}) currently polling NMMiner `/api/system/info`. Reuse + re-point; do not rewrite from scratch.
- `external/NMController_web/` — upstream reference only.
- `scripts/` — existing flash/backup/screensaver scripts; add a flash script for the fork mirroring `flash_v2_0_02.sh` safety (backup-first, ST7789 offsets).
- `firmware/v2.0.02/` — stock binaries kept as reference/rollback.
- Toolchain: git + gh (authed as kleinpanic) + node + python3 present; PlatformIO NOT yet installed.
</code_context>

<specifics>
## Specific Ideas
- Non-blocking speaker (LEDC, separate channel from backlight) so mining loop isn't stalled.
- SD file browser must sanitize paths (no traversal) — security boundary.
- CSV log rotation / size cap to avoid filling SD.
- Optimization: keep mining hashrate parity with stock NerdMiner; audio/SD work off the hot path.
- Keep ST7789 vs ILI9341 as a build flag so the repo serves both CYD variants.
</specifics>

<deferred>
## Deferred Ideas
- Binary RE of NMMiner (Ghidra Xtensa + FIDB) — fallback only if fork proves insufficient.
- OTA update endpoint.
- Multi-board release matrix beyond CYD ST7789/ILI9341.
- Flipper Zero integration (unrelated/charging this session).
</deferred>
