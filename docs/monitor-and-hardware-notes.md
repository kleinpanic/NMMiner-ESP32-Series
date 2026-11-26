# Monitor, Customization, And Hardware Notes

Date: 2026-06-01.

> Current-state note, 2026-06-19: this document describes the historical
> official-firmware LAN baseline and local monitor work. It is not the current
> live device state. The current unit is the custom decoded-spec ST7789 fork in
> WiFiManager recovery AP mode, with physical touch/mining still unproven. Use
> `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md` for the
> active recovery state.

## Historical Working Baseline

During the 2026-06-01 official-firmware audit, the miner was healthy enough to customize from a known-good state:

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

During the 2026-06-01 baseline, the built-in route was the most reliable one: open `http://192.168.1.136`. In the current Phase 7 recovery state, LAN monitor/API proof is still pending.

`NMController_web` was cloned locally under `external/NMController_web` for audit and customization experiments. A separate older instance is already running from `/home/broklein/codeWS/Python/NMController_web` on:

```text
http://127.0.0.1:7877/web_monitor
```

Status check on 2026-06-01:

- The historical monitor used the system Python interpreter. Current project tools must use `./.venv/bin/python`.
- Its working directory is `/home/broklein/codeWS/Python/NMController_web`.
- Both `/home/broklein/codeWS/Python/NMController_web` and `external/NMController_web` are clean upstream checkouts at commit `ba048864f247ff4678ed75a47c00c6d88a6c23fb`.
- No separate local dashboard app was found in this setup repo.
- The page is the stock `NMController Web Monitor` UI and was showing stale data for old IP `10.0.0.96`, firmware `v1.8.28`, not the 2026-06-01 baseline miner at `192.168.1.136`, firmware `v2.0.02`.

Conclusion from the status check: NMController Web is installed and running, but it is not the canonical customized dashboard. The upstream code explains why it currently misses this device: it only listens for UDP broadcast JSON on port `12345` and does not poll `/api/system/info`.

Canonical local controller status:

- Implemented in this repo under `apps/nmminer-control`.
- Run with `./scripts/run_nmminer_control.sh`.
- Local URL: `http://127.0.0.1:8788`.
- Polls `/api/system/info` through a Flask proxy instead of relying on UDP broadcasts.
- Separates live status polling from settings reads so the ESP32 is not hammered by repeated full-settings fetches.
- Supports display, market, weather, pool, identify, restart, and `.ss` screensaver preflight/upload surfaces.

## Visual And Profile Customization

The best first personalization target is not firmware patching. It is:

- A local dashboard branded around the device owner/profile.
- Custom 320x240 screen saver images uploaded through the official screen saver path.
- Safer settings controls for hostname, display preference, weather, market symbols, and pool config.

The embedded screen saver path is validated by firmware and has a 200 KB upload limit. Existing bundled screen savers are about 70-83 KB, and an 80 KB preflight test passed.

Screen saver images are stored on the ESP32's internal LittleFS partition under `/ss/saver_320_240_NNN.ss`. They are not stored on the laptop after upload and they are not stored on the SD/TF card slot. Live readback after the first upload pass showed:

- LittleFS total: `393216` bytes.
- LittleFS used: `344064` bytes.
- Approximate free space: `49152` bytes.
- Screen saver preflight `existingCount`: `16`.

This partition is not the same place as mining settings, Wi-Fi settings, wallet, or firmware. Filling it is not expected to erase mining config, but it can affect the embedded web UI and screen saver assets because those also live in LittleFS. The firmware's screen saver preflight/upload flow manages this by rejecting overlarge files and deleting old matching `/ss/saver_*.ss` files when needed.

The local generator is `scripts/nmminer_screensaver.py`. It mirrors the official browser encoder path and writes valid `.ss` files with the NMMiner header, RGB565 pixels, RLE body, and HS bit encoding. Generated preview/binary batches under `hardware/screensavers/generated*/` are gitignored because they are reproducible and can be large.

## Hardware Notes

The official manual says the board's external I/O ports and TF slot are reserved for a later NMMiner version. Current firmware does not require an SD/TF card, and adding a card should not be treated as enabling a current feature.

The exact Amazon listing is the Aideepen "Bitcoin Miner NerdMiner V2 ESP32 Development Board 2.8 inch" item for `ESP32-2432S028`. The listing confirms the board/shell bundle, ESP32-D0WD-V3, 2.8 inch touch screen, Type-C power, and a 5 V / 2 A troubleshooting recommendation. It does not publish the electrical pinout for the JST/header connectors.

A generic `ESP32-2432S028R-CYD` product listing for this board class lists a microSD slot, GPIO connector, speaker connector, temperature/humidity connector, and 5 V power through USB-C, microUSB, or a 4-pin 1.25 mm connector. Treat that as board-family evidence, not a guarantee that NMMiner firmware currently reads those add-ons.

Generic `ESP32-2432S028R` / CYD pinout references line up with the labels on this class of board, but clones and NMMiner-branded variants can differ. Treat these as leads until verified against high-resolution photos, silkscreen, and a multimeter:

- `CN1` / `P3` expansion headers may expose spare ESP32 GPIO pins plus power rails. The label that looks like `CH1` may be `CN1`.
- On common CYD boards, `P3` exposes `GND`, `GPIO35`, `GPIO22`, and `GPIO21`; `GPIO21` is also the display backlight, so it is not a clean general-purpose output while the screen is lit.
- On common CYD boards, `CN1` exposes `GND`, `GPIO22`, `GPIO27`, and `3V3`.
- The speaker/buzzer output is commonly tied to `GPIO26`.
- The microSD/TF slot is commonly wired over SPI using `GPIO5`, `GPIO18`, `GPIO19`, and `GPIO23`.
- Display, touch, backlight, and SD functions share limited ESP32 pins, so "free" pins are not always actually safe for new use.

Official stock NMMiner `v2.0.02` at the 2026-06-01 baseline should be treated as not supporting external humidity/sensor, speaker, or SD/TF workflows. The stock settings/API exposed internet weather, display, pool, market, and screen saver behavior, not an attached sensor readout or SD-backed asset store. The current custom fork includes SD/speaker code paths, but Phase 7 runtime validation is still pending.

Practical buying guidance:

- Stable USB-C power first: 5 V / 2 A adapter and a short known-good USB-C cable.
- For speaker experiments: 2-pin 1.25 mm JST-style pigtail plus a small speaker/passive buzzer, but only after verifying the board connector and firmware support path.
- For GPIO/sensor experiments: 4-pin 1.25 mm pigtails or 1.25 mm-to-Dupont leads, plus a 3.3 V I2C environmental sensor such as BME280, SHT31/SHT40, or AHT20. Use `CN1` only after confirming pin order.
- For SD/TF: a small microSD card is fine to physically add, but official stock firmware did not need it for mining or uploaded screensavers; current custom-fork SD behavior still needs Phase 7 validation.

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

Clone upstream monitor tooling for reference only. Keep the source-available custom controller in this repo, and avoid binary firmware patching for the first personalization pass.

## Sources

- Official NMMiner repo: `https://github.com/NMminer1024/NMMiner`
- Official NMController Web repo: `https://github.com/NMminer1024/NMController_web`
- Official flasher: `https://flash.nmminer.com/`
- Amazon manual PDF: `https://m.media-amazon.com/images/I/A1e9fPR2b%2BL.pdf?ref=dp_product_quick_view`
- CYD pinout reference: `https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/`
