# Monitor, Customization, And Hardware Notes

Date: 2026-06-01.

## Current Working Baseline

The miner is healthy enough to customize from a known-good state:

- Firmware: `v2.0.02`, current official release at audit time.
- Board target: `esp32-2432s028r-st7789`.
- Miner API: `http://192.168.1.136`.
- Pool: `pool.nerdminers.org:3333`.
- Live API readback showed about `1.04 MH/s`, increasing accepted shares, and one rejected share during this session.
- The laptop stayed on `Homebase5`; no AP-mode Wi-Fi handoff was used.

## Sanctioned Customization Paths

The safe paths are the ones exposed by the official firmware:

1. Use the embedded web UI at the miner IP for normal settings and built-in monitoring.
2. Use the documented HTTP API for settings, status, weather, market symbols, brightness, screen saver mode, and restarts.
3. Upload custom screen saver images through the official web UI/API.
4. Run or customize the official `NMController_web` LAN monitor.

The official `NMminer1024/NMMiner` repo is useful as a firmware/reference repo, but it is not currently a full ESP32 firmware source tree for this CYD miner. Do not plan on local source edits to the mining core or on-device LVGL pages unless upstream publishes that source.

Recommended local development split:

- Keep `NMminer1024/NMMiner` as the reference for releases, flash offsets, docs, and prebuilt binaries.
- Use `NMminer1024/NMController_web` or a new small local dashboard for personal UI/profile work.
- Build a `nmminerctl`-style local CLI for repeatable status checks, pool checks, settings changes, and screen saver uploads.

## Centralized Monitor Status

The Amazon manual lists two centralized-monitor routes:

- `3.1 NMController`: separate LAN monitor software.
- `3.2 NMMiner Monitor`: the built-in monitor at the miner's IP address.

The built-in route is currently the most reliable one: open `http://192.168.1.136`.

`NMController_web` was cloned locally under `external/NMController_web` for audit and customization experiments. A separate older instance is already running from `/home/broklein/codeWS/Python/NMController_web` on:

```text
http://127.0.0.1:7877/web_monitor
```

Status check on 2026-06-01:

- The running monitor process is `/usr/bin/python3 nmcontroller.py`.
- Its working directory is `/home/broklein/codeWS/Python/NMController_web`.
- Both `/home/broklein/codeWS/Python/NMController_web` and `external/NMController_web` are clean upstream checkouts at commit `ba048864f247ff4678ed75a47c00c6d88a6c23fb`.
- No separate local dashboard app was found in this setup repo.
- The page is the stock `NMController Web Monitor` UI and is showing stale data for old IP `10.0.0.96`, firmware `v1.8.28`, not the live miner at `192.168.1.136`, firmware `v2.0.02`.

Conclusion: NMController Web is installed and running, but custom dashboard/profile work is not done yet. The upstream code explains why it currently misses this device: it only listens for UDP broadcast JSON on port `12345` and does not poll `/api/system/info`. For this device, an API-polling dashboard will be more reliable than depending only on UDP broadcasts.

## Visual And Profile Customization

The best first personalization target is not firmware patching. It is:

- A local dashboard branded around the device owner/profile.
- Custom 320x240 screen saver images uploaded through the official screen saver path.
- Safer settings controls for hostname, display preference, weather, market symbols, and pool config.

The embedded screen saver path is validated by firmware and has a 200 KB upload limit. Existing bundled screen savers are about 70-83 KB, and an 80 KB preflight test passed.

## Hardware Notes

The official manual says the board's external I/O ports and TF slot are reserved for a later NMMiner version. Current firmware does not require an SD/TF card, and adding a card should not be treated as enabling a current feature.

The exact Amazon listing is the Aideepen "Bitcoin Miner NerdMiner V2 ESP32 Development Board 2.8 inch" item for `ESP32-2432S028`. The listing confirms the board/shell bundle, ESP32-D0WD-V3, 2.8 inch touch screen, Type-C power, and a 5 V / 2 A troubleshooting recommendation. It does not publish the electrical pinout for the JST/header connectors.

Generic `ESP32-2432S028R` / CYD pinout references line up with the labels on this class of board, but clones and NMMiner-branded variants can differ. Treat these as leads until verified against high-resolution photos, silkscreen, and a multimeter:

- `CN1` / `P3` expansion headers may expose spare ESP32 GPIO pins plus power rails. The label that looks like `CH1` may be `CN1`.
- On common CYD boards, `P3` exposes `GND`, `GPIO35`, `GPIO22`, and `GPIO21`; `GPIO21` is also the display backlight, so it is not a clean general-purpose output while the screen is lit.
- On common CYD boards, `CN1` exposes `GND`, `GPIO22`, `GPIO27`, and `3V3`.
- The speaker/buzzer output is commonly tied to `GPIO26`.
- The microSD/TF slot is commonly wired over SPI using `GPIO5`, `GPIO18`, `GPIO19`, and `GPIO23`.
- Display, touch, backlight, and SD functions share limited ESP32 pins, so "free" pins are not always actually safe for new use.

Power improvements are worth doing before signal/header mods:

- Use a stable 5 V supply with enough current headroom; the product listing and manual context point to 5 V USB power, with the listing recommending 5 V / 2 A for stability.
- Verify JST connector polarity before applying external power.
- Avoid powering through USB-C and a JST/external input from two independent supplies unless the board's power path is confirmed to prevent backfeed.
- If restart loops or Wi-Fi instability appear, test with a short known-good USB cable and a dedicated 5 V / 2 A supply before changing firmware.

Before connecting anything to `CH1`, `P3`, a speaker header, or JST power, capture board photos:

- Front and back of the board.
- Close-ups of the JST, `CH1`, `P3`, speaker pads/header, SD slot, voltage regulator, and USB-C power section.
- Any silkscreen labels next to each pin.

## Decision

Clone and customize source-available monitor tooling, not the binary firmware, for the first personalization pass. The next useful build target is a local controller/dashboard that polls `http://192.168.1.136/api/system/info`, displays the miner state cleanly, and can upload custom screen savers.

## Sources

- Official NMMiner repo: `https://github.com/NMminer1024/NMMiner`
- Official NMController Web repo: `https://github.com/NMminer1024/NMController_web`
- Official flasher: `https://flash.nmminer.com/`
- Amazon manual PDF: `https://m.media-amazon.com/images/I/A1e9fPR2b%2BL.pdf?ref=dp_product_quick_view`
- CYD pinout reference: `https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/`
