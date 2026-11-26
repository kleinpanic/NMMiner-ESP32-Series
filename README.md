# PanicMiner ESP32 workspace

Hardware bring-up, firmware, recovery tools and validation for an
ESP32-2432S028R 2.8-inch Cheap Yellow Display miner. The open PanicMiner
firmware lives in the [firmware source](firmware/custom/NerdMiner-CYD-Custom).
This single repository includes the firmware source and reverse-engineering notes.

## Daily use

Plug in USB power. The board loads its saved Wi-Fi and pool settings and starts
mining without a computer. Wi-Fi must be **2.4 GHz**.

- Tap right for the next page, left for the previous page. A held press acts once.
- Short BOOT press: next page. Double press: rotate. Triple press: backlight.
- Pages stay where you leave them: Mine, Hash, Chain, Clock, Stats.
- Hold BOOT for 5 seconds to open Setup without erasing the current settings.
  Join the AP shown on the screen and open `http://192.168.4.1`.
- At a new location, choose the new 2.4 GHz network and save. Existing pool
  settings remain populated. A failed join opens Setup temporarily, then retries.

The screen colours use the verified `ILI9341_2_DRIVER` + `TFT_INVERSION_ON`
configuration. The vendor identifies this unit as ST7789; the working custom
configuration is recorded in PlatformIO rather than inferred from that label.

## Current validation

The matching Mine, Hash, Chain, Clock, Stats, Boot and Setup screens are built
and installed. Navigation is manual and debounced. Stats shows measured 10/60-second
rates, verified best difficulty, pool-accepted/rejected shares, CPU/RAM usage,
chip temperature, signal and SD writes. Chain uses certificate-verified live
price, network difficulty, fees and retarget data.

SD now shares the display's actual SPI controller and transaction lock, with
separate chip selects; touch keeps its own controller. A mounted FAT32 card
records measured statistics every minute in `/sd/mining_stats_v2.csv`, rotating
at 1 MB. Saved internal settings take priority over optional SD recovery settings.
Native tests have confirmed card capacity, successful rows and a complete Stats
screen. Recovery tools take and verify a full backup before every firmware write.

**Hardware acceptance is still open.** The newest image adds case-insensitive
SD private-path protection and rejects FAT filename aliases. It was installed
after a fresh verified full backup; fresh native qualification is running.
The previous image passed a strict 20-minute speed test at **967.639 kH/s**
against the 994 kH/s baseline, with all sampled rolling rates in the required
944.3–1043.7 band and all 17 checks passing. Complete SD downloads passed,
including cancellation recovery and simultaneous status requests. Its first
hour attempt encountered an API timeout and a partial feed cycle; automatic
recovery worked, but that attempt failed. A repeat was interrupted for the
verified SD privacy repair. Charger cold boot with the SD card inserted still
requires the owner's physical test. Publication remains on hold.
Exact binary digests and all failed/interrupted attempts are preserved in
[runtime validation](docs/panicminer-validation-2026-11-25.md).

Wallet BTC/USD values come from the **configured payout address**, after removing
its worker suffix. They describe that address's funds, including other transfers;
accepted solo shares do not create a payout. Unknown or stale data is labelled.
No private key or signing access is needed or stored. Reward sounds occur only
for confirmed events, with volume and mute controls; startup and taps stay quiet.

## Get the source and build

```bash
git clone https://github.com/kleinpanic/NMMiner-ESP32-Series.git
cd NMMiner-ESP32-Series
# Reuse .venv if present; create it only on a fresh checkout.
uv venv .venv
uv pip install --python ./.venv/bin/python -r requirements.txt
cd firmware/custom/NerdMiner-CYD-Custom
../../../.venv/bin/python -m platformio run -e ESP32-2432S028R
cd ../../..
./scripts/flash_fork_build.sh panicminer
```

Every firmware write must have a fresh full 4 MB backup. The flash helper reads
and verifies that backup, writes the build, then verifies every image. Restore
also backs up and verifies the current device before writing:

```bash
./scripts/restore_full_flash_backup.sh hardware/backups/<full-image.bin>
```

The historical `scripts/flash_v2_0_02.sh` stock-recovery helper follows the same
backup-before-write rule and verifies all five stock images, including the
filesystem. Both recovery helpers have stubbed failure-path tests in CI; those
tests do not write a device.

The connected unit has an ESP32-D0WD-V3 with 4 MB flash. Flash and restore prefer
its stable `/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0` link because the
`ttyUSB` number can change after reconnection. An explicit `PORT` overrides it;
without the link, the helpers retain the `/dev/ttyUSB0` fallback. Close serial
monitors before flashing. Generated builds, full backups and serial logs stay
local in ignored directories.

## Workspace map

| Path | Purpose |
| --- | --- |
| `firmware/custom/NerdMiner-CYD-Custom/` | Open firmware, touch, UI, mining and live feed |
| `scripts/` | Backup, verified flash/restore, provisioning and framebuffer capture |
| `docs/panicminer-validation-2026-11-25.md` | Hardware results, measurements and remaining gates |
| `firmware/custom/NerdMiner-CYD-Custom/docs/design/` | Current display design |
| `apps/nmminer-control/` | Local controller/dashboard; historical stock tooling |
| `.planning/` | GSD requirements, decisions and milestone state |
| `hardware/backups/`, `logs/`, `secrets/` | Ignored private device data |

The firmware README documents controls, measured statistics and developer
commands. The host renderer in its `tools/uisim/` directory renders the same UI
source in both themes. Framebuffer capture is deliberately kept outside timed
mining runs because serial dumping blocks the display task.

The application, mining loop and UI are built from the source in this workspace;
the vendor's NMMiner application is replaced. Espressif's radio libraries and
chip ROM still contain proprietary components. See the verified
[firmware provenance](docs/firmware-provenance.md) for that boundary.

Firmware source was consolidated from `v4/panicminer` commit `bf08ec8` into this
repository. It is an owned source fork of NerdMiner with separate NMMiner binary
research/recovery notes; it is not a completely decoded vendor firmware. The
old nested Git history is retained locally under ignored `external/`.

## Secrets and recovery

Do not commit Wi-Fi passwords, wallet values, device licences, full flash images,
serial logs or real configuration files. Private GitHub visibility does not
replace those exclusions. Dependencies belong in `requirements.txt` and all
Python tools run through `./.venv/bin/python`.

The saved wallet's checksum and match against the recovered configuration were
verified privately, and the board authorized that worker with the pool. No wallet
value is included here. Best difficulty measures a software-verified candidate;
shares count pool-confirmed submissions, and blocks use the full network target.
Zero shares or blocks are valid results and are not replaced with simulated
earnings. Audio is silent at boot and reconnect, with distinct events for a
confirmed share and a confirmed block. No payout has been observed or claimed.

The original closed NMMiner recovery procedure is historical: long BOOT, join
`nmap-2.4g`, open `192.168.4.1`. It applies when that firmware is installed.
Reflashing official stock recovery replaces PanicMiner; take a fresh backup first.

Earlier research remains available in [setup notes](docs/setup-nm-cyd.md),
[hardware notes](docs/monitor-and-hardware-notes.md),
[customization audit](docs/customization-audit.md),
[code/UI/math audit](docs/codebase-audit.md), and
[vendor manual](docs/vendor/NM-CYD-ESP32-2432S028-manual.txt).
Those June recovery and asset investigations do not describe the current runtime.

## Contributing and licensing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build and validation commands. The
application retains its upstream [MIT license](firmware/custom/NerdMiner-CYD-Custom/LICENSE)
and [attribution](firmware/custom/NerdMiner-CYD-Custom/NOTICE). The provenance
document records SDK and third-party license boundaries.
