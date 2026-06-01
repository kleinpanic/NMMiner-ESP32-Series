# NMMiner ESP32 Series Setup

This repo tracks setup, recovery, and upgrade notes for the NM-CYD / ESP32-2432S028 NMMiner device.

Current device state:

- USB serial: `/dev/ttyUSB0`
- Chip: ESP32-D0WD-V3, 4 MB flash
- Board family: CYD 2.8
- Display firmware target: `esp32-2432s028r-st7789`
- Running firmware: `v2.0.02`
- Wi-Fi: `Homebase` 2.4 GHz
- LAN API: `http://192.168.1.136`
- Primary pool: `stratum+tcp://pool.nerdminers.org:3333`
- Fallback pool: `stratum+tcp://pool.nerdminers.org:3333`
- Wallet: recovered BTC mainnet P2SH address, validated and configured
- Full flash backup: `hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin` (gitignored)
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

Useful commands:

```bash
# Back up current flash
.venv/bin/python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 read-flash 0x0 0x400000 hardware/backups/nmminer-esp32-d0wd-v3-full-flash-$(date +%F).bin

# Reflash prepared v2.0.02 ST7789 firmware if recovery/reinstall is needed
./scripts/flash_v2_0_02.sh

# Send config over USB serial; keep the laptop on its current Wi-Fi
./scripts/send_serial_config.py --ssid 'Homebase' --password '<wifi password>' --wallet '<btc wallet>' --delay 4.5
```
