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

Status: pending.

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
- Compare CYD board with newer NMMiner-supported hardware.
- Decide whether any physical upgrades are worth doing for a novelty miner.
