# NMMiner ESP32 Series Setup

This repo tracks setup, recovery, and upgrade notes for the NM-CYD / ESP32-2432S028 NMMiner device.

Current device state:

- USB serial: `/dev/ttyUSB0`
- Chip: ESP32-D0WD-V3, 4 MB flash
- Board family: CYD 2.8
- Display firmware target: `esp32-2432s028r-st7789`
- Running firmware: custom decoded-spec `ESP32-2432S028R` ST7789 fork reporting recovered NMMiner `v2.0.02`; not the official stock binary
- Live app verification: final webfix2 app was flashed at `0x10000`, independently read back, and rebuilt locally without a device write; all three app artifacts match.
- Live app artifact: 2,840,400 bytes, SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, contains `v2.0.02` and no `V1.8.3`
- Live asset verification: decoded LittleFS image `/tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` verified byte-for-byte at `0x310000` on 2026-06-19 00:20 UTC / 2026-06-18 20:20 EDT
- Latest serial runtime: no-write reset touch window showed clean recovery/config behavior, 0 panic lines, 31 touch diagnostics, 0 pressed observations, and 0 IRQ-low observations; raw serial was not stored in tracked files.
- Recovery AP: `nmap-2.4g`
- Recovery portal: `http://192.168.4.1`
- Recovery web UI: WiFiManager root page reviewed at desktop and mobile widths; custom JSON API is not reachable in portal mode
- Touch: unresolved; serial diagnostics show no raw press/contact signal (`directPressed=0`, `libPressed=0`)
- Official `v2.0.02` ST7789 image: verified recovery baseline and decoded-spec source; `v2.0.02` text alone no longer distinguishes stock from the custom rebuilt firmware
- Historical LAN/mining state: prior stock-baseline runs used `Homebase` 2.4 GHz and the LAN API, but the current live unit is in recovery AP mode
- Latest pre-write full flash backup: `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin` (gitignored)
- Recovered credentials and wallet notes: `secrets/recovered-device-config-2026-06-01.txt` (gitignored)

Key docs:

- [Setup Procedure](docs/setup-nm-cyd.md)
- [Amazon Product Description Notes](docs/product-description.md)
- [Code/UI/Math Audit Notes](docs/codebase-audit.md)
- [Customization Audit](docs/customization-audit.md)
- [Monitor, Customization, And Hardware Notes](docs/monitor-and-hardware-notes.md)
- [Kleinpanic Customization Basis](docs/kleinpanic-customization-basis.md)
- [Official Manual Text Extract](docs/vendor/NM-CYD-ESP32-2432S028-manual.txt)
- [GSD Project Context](.planning/PROJECT.md)
- [Phase 7 Current Failure State](.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md)

Canonical local tools:

- [Kleinpanic NMMiner Control](apps/nmminer-control/README.md)

Useful commands:

```bash
# Run the local dashboard/controller
./scripts/run_nmminer_control.sh

# Back up current flash
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 read_flash 0x0 0x400000 hardware/backups/nmminer-esp32-d0wd-v3-full-flash-$(date +%F).bin

# Reflash official stock v2.0.02 ST7789 recovery baseline if recovery/reinstall is needed.
# Take a fresh full 4 MB backup first; current custom firmware also reports v2.0.02.
./scripts/flash_v2_0_02.sh

# Send config over USB serial; keep the laptop on its current Wi-Fi
./scripts/send_serial_config.py --ssid 'Homebase' --password '<wifi password>' --wallet '<btc wallet>' --delay 4.5
```
