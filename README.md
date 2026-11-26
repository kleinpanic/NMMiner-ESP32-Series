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

**Wi-Fi and mining recovered on November 26.** With the unit near the router,
SD removed and laptop USB power, the saved wallet worker authorized with the
pool and all five Chain endpoints loaded. After a statistics checkpoint, an EN
reset rejoined automatically, restored best difficulty and resumed mining without
provisioning. A subsequent cold boot on a USB charger also authorized the pool,
restored the same best and mined near 999 kH/s with no laptop USB connection.
The charger-powered observer was interrupted when the owner requested SD
reinsertion and an SD firmware update. It did not complete the 20-minute gate.
Automatic boot and mining were verified with the card removed.

The SD update fixes separate display/card owners reconfiguring the same SPI
controller. They now share the display's SPI object and transaction lock, with
clock/data routed to both wired buses and separate chip selects. Touch retains
its existing controller. The update builds, passes host regression checks and
was installed with a fresh verified full backup on November 26. All four flash
images verify, and a native saved-settings boot detects the inserted SD card
before joining Wi-Fi. The owner reports this USB location is out of range;
card CSV writes, display/touch coexistence and the fresh endurance run still
need the downstairs charger boot near the router.
With a mounted FAT32 card, firmware writes measured rates, accepted/rejected
shares, best difficulty, memory and temperature once per minute to
`/sd/mining_stats_v2.csv`, rotating at 1 MB. The Stats page and local API report
card capacity and successfully saved rows. Existing internal settings take
priority over the card's optional recovery configuration.

The router's airtime fairness was disabled through authenticated JNAP while the
unit was moved closer. Signal improved from about -63 to -38 dBm (-34 dBm on
restart). Both changes occurred together; neither is established as the sole
cause. The router setting briefly restarted Wi-Fi, which recovered. The current
working configuration retains airtime fairness disabled and WPA2 security.

The repair fixes a mining-job buffer overrun, long-coinbase truncation, blocking
job logging and inconsistent work-counter reads. Network startup scans all
channels and tries matching access points on their detected channels. Exact
firmware digests and native results are in
[runtime validation](docs/panicminer-validation-2026-11-25.md).

Mine, Hash, Chain, Clock, Stats, Boot and Setup share the Afterglow design;
Hazard is also available. Native captures show live price, retarget, network
difficulty, CPU/RAM/temperature and pool authorization. Pages stay manual, with
one action per debounced press. The original TLS allocation failure was fixed
by freeing 12 KB from LVGL; certificate verification remains enabled.

The original 20-minute Mine run averaged **972.6 kH/s**, 2.15% below the
994 kH/s baseline, but some rolling readings fell outside the 5% band. A new
20-minute run is required on the repaired binary. SD reinsertion and sustained
unattended-use acceptance remain open. Publication stays on hold.

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
