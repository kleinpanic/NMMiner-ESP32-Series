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

**Hardware acceptance is open.** Removing the SD card and reconnecting USB
restored automatic Wi-Fi connection, pool authorization and mining. An EN reset
also restored saved statistics and live Chain data. A later timed run caught
Wi-Fi loss and a speed dip. Association still failed after returning near the
router, so distance alone is not an established explanation. Testing continues
with laptop USB power and the SD card out.

The current repair fixes a mining-job buffer overrun, long-coinbase truncation,
blocking job logging and inconsistent work-counter reads. It also tries normal
Wi-Fi rates first, scans all channels and retries individual access points
on their detected channels while retaining recovery profiles. The exact firmware digests,
failed runs and restart results are in
[runtime validation](docs/panicminer-validation-2026-11-25.md).

Mine, Hash, Chain, Clock, Stats, Boot and Setup share the Afterglow design;
Hazard is also available. Native captures show live price, retarget, network
difficulty, CPU/RAM/temperature and pool authorization. Pages stay manual, with
one action per debounced press. The original TLS allocation failure was fixed
by freeing 12 KB from LVGL; certificate verification remains enabled.

The original 20-minute Mine run averaged **972.6 kH/s**, 2.15% below the
994 kH/s baseline, but some rolling readings fell outside the 5% band. A new
20-minute run is required on the repaired binary. SD operation after reinsertion
and unattended-use acceptance remain open. Publication stays on hold.

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

The original closed NMMiner recovery procedure is historical: long BOOT, join
`nmap-2.4g`, open `192.168.4.1`. It applies when that firmware is installed.
Reflashing official stock recovery replaces PanicMiner; take a fresh backup first.

Earlier research remains available in [setup notes](docs/setup-nm-cyd.md),
[hardware notes](docs/monitor-and-hardware-notes.md),
[customization audit](docs/customization-audit.md),
[code/UI/math audit](docs/codebase-audit.md), and
[vendor manual](docs/vendor/NM-CYD-ESP32-2432S028-manual.txt).
Those June recovery and asset investigations do not describe the current runtime.
