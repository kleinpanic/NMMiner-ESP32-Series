# PanicMiner for the CYD

Open firmware for the ESP32-2432S028R 2.8-inch Cheap Yellow Display. This source
combines a pipelined SHA256d miner with the Afterglow UI,
live Bitcoin network data, touch navigation and a local configuration portal.

This is a fork of [BitMaker's NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2).
The classic ESP32 mining loop comes from HenrysCat's NerdMiner Modded. See
[NOTICE](NOTICE), [LICENSE](LICENSE), and the font licences in `src/ui/fonts/licenses/`.

## Use the miner

Power it from USB. Saved Wi-Fi and pool settings load automatically; the host
computer is not needed for normal mining. Wi-Fi must be **2.4 GHz**.

| Control | Action |
| --- | --- |
| Tap the right half | Next page |
| Tap the left half | Previous page |
| Short BOOT press | Next page |
| Double BOOT press | Rotate the display 180 degrees |
| Triple BOOT press | Toggle the backlight; a tap wakes it |
| Hold BOOT for 5 seconds | Open Setup while preserving saved settings |
| Leave it alone | The selected page stays put |

The resistive touch controller uses the original CYD HSPI wiring and calibration:
CLK25, MISO39, MOSI32, CS33, IRQ36. Polling runs on the display task, once per
Monitor frame, with one page change per press. Presses must remain stable for
60 ms, release for 300 ms, and respect a 500 ms cooldown; a narrow center band
ignores ambiguous taps. All LVGL operations stay on that
task; the button changes only the requested page.

To move to another network, hold BOOT, join the AP shown on Setup, and open
`http://192.168.4.1`. Select the new 2.4 GHz network and save. The portal retains
existing pool settings, and the new Wi-Fi credentials are copied into the app's
saved configuration. Leaving Setup without saving resumes the old settings on
restart. Failed automatic joins also open the portal temporarily before retrying.

## Pages and measurements

- **Mine:** current rolling hashrate, 60-point history, shares, best difficulty,
  uptime, block and fee activity.
- **Hash:** best software-verified proof, target zero digits and live prefilter hashes.
- **Chain:** verified HTTPS data from mempool.space: price, height, difficulty,
  network hashrate, fees and the 2,016-block adjustment period.
- **Clock:** local time and mining status.
- **Stats:** 10-sample and 60-sample time-weighted rates (about 10 seconds and one
  minute), average and work since boot, one-second min/max over the last minute,
  every pool-confirmed accepted share, rejected tracked submissions, jobs,
  SHA checks/errors, sampled busy percentage for both CPU cores, RAM usage,
  chip temperature, Wi-Fi signal and SD rows saved. CPU and internal-chip
  temperature readings are estimates. Share/reject counts are separate
  counters, not an acceptance percentage. Saved lifetime counters can outlive a boot;
  rate windows, work and SHA checks start fresh.

When enabled in Setup, mining statistics checkpoint to NVS every five minutes;
unplugging can lose work since the last checkpoint. SD logging writes measured
rates and counters once per minute to `/sd/mining_stats_v2.csv`, preserving the
older CSV. Sound stays quiet at startup and reconnect: a short rate-limited
chirp marks pool-confirmed work, and a distinct melody marks a pool-accepted
network-target block. An accepted share is not a payment; this is solo lottery
mining. Volume and mute are available through the local API.

Boot and Setup use the same visual system. Afterglow is the default; Hazard is
available at build time. Formatting/animation work is limited to the visible page.
The rate history uses fixed storage instead of allocating list nodes each second.

The 2026-11-25 Mine run before the touch/Stats update averaged **972.6 kH/s** over
20 minutes against a 994 kH/s baseline, with zero SHA mismatches. Eleven rolling
readings were below the 5% band, so the strict per-reading gate remained open.
See the parent workspace's runtime validation for binary identities and current
hardware acceptance; rendering success alone does not establish physical touch.

## Build and flash safely

Use the [parent workspace](https://github.com/kleinpanic/NMMiner-ESP32-Series)
for pinned Python dependencies, full-flash backups and verified flashing:

```bash
git clone https://github.com/kleinpanic/NMMiner-ESP32-Series.git
cd NMMiner-ESP32-Series
# Reuse .venv if it already exists.
uv venv .venv
uv pip install --python ./.venv/bin/python -r requirements.txt
cd firmware/custom/NerdMiner-CYD-Custom
../../../.venv/bin/python -m platformio run -e ESP32-2432S028R
cd ../../..
./scripts/flash_fork_build.sh panicminer
```

The flash helper takes a fresh full 4 MB backup, verifies it against the device,
writes the build and verifies every written image. Restore also backs up first:

```bash
./scripts/restore_full_flash_backup.sh hardware/backups/<full-image.bin>
```

The verified unit uses `ILI9341_2_DRIVER` with `TFT_INVERSION_ON` in the CYD
environment. The vendor calls this hardware ST7789; changing the working panel
configuration based only on that name previously produced wrong colours.

## Repository map

| Path | Purpose |
| --- | --- |
| `src/ui/` | Shared LVGL UI, motion and bundled fonts |
| `src/drivers/displays/panicMinerDisplayDriver.cpp` | Panel, touch and UI data bridge |
| `src/mining.cpp`, `src/pipelined_hw_sha_classic_v2.S` | Mining and SHA verification |
| `src/chainFeed.cpp` | Pinned-CA HTTPS network feed |
| `src/wManager.cpp` | Saved settings, Setup and network recovery |
| `tools/uisim/` | Headless renderer using the firmware's LVGL source |
| `docs/design/panicminer-design.md` | Palette, layout, type and motion |
| `docs/API.md` | Local HTTP API inherited from the custom fork |

Render all screen fixtures from `tools/uisim` with `make THEME=1` or `make THEME=2`.
The renderer pins LVGL to the firmware's version. Native serial diagnostics include
`PMDUMP`, `PMNEXT`, `PMFEED`, `PMMEM`, `PMSTATE` and `PMTOUCH`. `PMSAVE` requests
a statistics checkpoint. `PMMINE` selects Mine; `PMAUTO` explicitly enables
diagnostic page rotation. A physical tap restores manual paging. Long BOOT
and `PMSETUP` use the same Setup path.

`PMRF` is an explicit USB recovery command accepted only during the six-second
serial configuration window at boot. It stops Wi-Fi, clears only Espressif's
saved RF calibration, then restarts for full calibration. App configuration and
mining statistics are preserved; it does not run on normal boots.

Run `bash tools/tests/run.sh` for 132 production-code regression cases and the
speaker tests. The suite covers target decoding/comparison, fragmented pool
messages, authorization and acceptance responses, touch, SD paths, stalled work,
coinbase hashing, merkle roots and complete header byte ordering. Linux host
builds need `libssl-dev`. See `tools/tests/README.md` for the hardware limits.

Credentials, wallets, device licences, flash images, serial logs and generated
captures belong only in ignored local storage. Do not commit a real `config.json`.
The wider upstream board support and legacy SD/speaker/API features are retained;
this workspace's physical validation applies to the CYD target, not every board.
