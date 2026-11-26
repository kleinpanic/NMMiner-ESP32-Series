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

**Hardware acceptance is open.** The latest firmware is backed up, flashed and
verified, but this unit is currently failing Wi-Fi authentication/association.
SD initialization is deferred until Wi-Fi connects. All four radio profiles
still failed with zero SD access. Live pool, CSV logging and restart persistence
need verification on this binary. It is not yet cleared for unattended use.

The redesigned Mine, Hash, Chain, Clock, Stats, Boot and Setup screens share the
Afterglow visual system; Hazard is also available. The touch update restores the
original CYD touch bus and calibration, polls each display frame and debounces
held presses. Physical acceptance and firmware digests are recorded in
[runtime validation](docs/panicminer-validation-2026-11-25.md).

An earlier native run fetched all five mempool.space endpoints with certificate
verification enabled. RSA verification was failing from insufficient usable
heap; reducing the LVGL reservation by 12 KB fixed that failure. Live Chain data
must be checked again after this unit reconnects.

The original 20-minute Mine check averaged **972.6 kH/s**, 2.15% below the
994 kH/s baseline, with zero SHA mismatches. Eleven rolling readings briefly
fell below the 5% band, so the strict per-reading gate remains open. This run
predates the touch/Stats update; the evidence distinguishes the tested binaries.
Two restart/rejoin checks passed; those were EN resets, not literal USB power
removal. See the validation document for the current limits of hardware testing.

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

The connected unit uses `/dev/ttyUSB0` and has an ESP32-D0WD-V3 with 4 MB flash.
Close serial monitors before flashing. Generated builds, full backups and serial
logs stay local in ignored directories.

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

The original closed NMMiner recovery procedure is historical: long BOOT, join
`nmap-2.4g`, open `192.168.4.1`. It applies when that firmware is installed.
Reflashing official stock recovery replaces PanicMiner; take a fresh backup first.

Earlier research remains available in [setup notes](docs/setup-nm-cyd.md),
[hardware notes](docs/monitor-and-hardware-notes.md),
[customization audit](docs/customization-audit.md),
[code/UI/math audit](docs/codebase-audit.md), and
[vendor manual](docs/vendor/NM-CYD-ESP32-2432S028-manual.txt).
Those June recovery and asset investigations do not describe the current runtime.
