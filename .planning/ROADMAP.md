# Roadmap

## Phase 1: Preserve And Identify

Status: complete.

- Initialize git.
- Download and extract official manual.
- Capture serial boot log.
- Install local `esptool` in `.venv`.
- Identify chip and flash size.
- Back up full flash.
- Recover saved config and keep it out of git.
- Determine display target as `esp32-2432s028r-st7789`.

## Phase 2: Configuration Recovery

Status: complete.

- Use target 2.4 GHz SSID `Homebase`.
- Send serial config or use AP config page.
- Reboot and verify Wi-Fi connection.
- Find miner IP and query web/API status.

## Phase 3: Firmware Upgrade

Status: complete.

- Confirm upgrade to `v2.0.02`.
- Flash ST7789 build using official offsets.
- Preserve NVS by avoiding full erase.
- Verify serial boot, display output, license status, and web/API.
- Restore from backup if display or boot fails.

## Phase 4: Monitor And Tune

Status: complete baseline; future tuning remains available.

- Configure low-difficulty BTC pools.
- Confirm wallet/worker and accepted/rejected shares.
- Adjust UI refresh/screen timeout for hashrate.
- Document final stable settings.

## Phase 5: Code, UI, And Mining-Math Audit

Status: complete baseline; customization implementation remains pending.

- Map the official NMMiner repo and confirm firmware source availability.
- Audit `NMController_web` source for monitoring UI behavior, hashrate formatting, UDP discovery, and customization opportunities.
- Audit official flasher JavaScript for config schema, activation flow, flash offsets, and UI assumptions.
- Inspect `littlefs.bin` and `firmware.bin` for web assets, on-device UI strings, LVGL/screen behavior, and mining protocol strings.
- Determine what can be customized safely through config/API versus what would require upstream source or binary patching.
- Create a customization backlog covering visual UI tweaks, monitor UI tweaks, pool/wallet workflow, and mining-math feasibility.

## Phase 6: Hardware Upgrade Review

Status: pending.

- Check power stability.
- Evaluate enclosure/heat and USB cable/brick.
- Identify JST, CH1, P3, speaker, and TF/SD wiring from board photos before applying power or attaching peripherals.
- Confirm whether any exposed header pins are actually unused by display, touch, backlight, speaker, or SD functions.
- Compare CYD board with newer NMMiner-supported hardware.
- Decide whether any physical upgrades are worth doing for a novelty miner.

## Phase 7: Personalized Local Controller

Status: complete baseline; further polish remains available.

- Build a local API-polling dashboard for `http://192.168.1.136/api/system/info`.
- Add safe settings controls for display, weather, market symbols, and pool config.
- Add a custom 320x240 screen saver generation/upload workflow.
- Keep official firmware binary-only and avoid direct firmware patching unless upstream source becomes available.
- Canonicalize the controller under `apps/nmminer-control`.
- Keep the older upstream NMController checkout as non-canonical reference only.

## Phase 8: Custom Firmware Fork (SD + Speaker + Dashboard)

Status: pending.

Direction decided 2026-06-03: stock NMMiner v2.0.02 is binary-only with no SD/audio API, so we fork open source instead. See `docs/firmware-customization-feasibility.md`.

- Fork the MIT-licensed `BitMaker-hub/NerdMiner_v2` as our custom firmware base.
- Target this unit's display correctly: ESP32-2432S028R, **ST7789** variant (mainline NerdMiner CYD env defaults to ILI9341 — must verify/override).
- Graft SD-card file I/O and speaker/buzzer (LEDC tone) support from `witnessmenow/ESP32-Cheap-Yellow-Display` and CYD NerdMiner forks; verify CYD pins (speaker ~GPIO26; SD SPI GPIO5/18/19/23) against silkscreen before relying on them.
- Build our own custom features on top (own JSON status + control web API exposing mining stats, an `sd` block, and `speaker` control).
- Re-point/extend `apps/nmminer-control` dashboard to the new firmware API; add SD + speaker control surfaces.
- Upgrade dashboard UI and validate visually via Playwright MCP at desktop + mobile widths.
- Keep the full NMMiner flash backup as the restore path; only flash the fork after a verified backup and a documented rollback.
- Keep stock NMMiner binary/docs as reference only; do not pursue binary reverse-engineering unless the fork proves insufficient (RE recon kept as fallback).
