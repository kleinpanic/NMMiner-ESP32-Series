# 08-08 SUMMARY — Flash + On-Device Validation

**Status:** BLOCKED — hardware not connected for flashing.

## Blocker
The CYD must be on a USB serial port of the build machine to flash. As of this session:
- NMMiner CYD is **alive on LAN** (`192.168.1.136`, stock v2.0.02, mining) → powered + on WiFi.
- **No `/dev/ttyUSB*` and no CH340/CP210x on the USB bus** — only the Flipper Zero (`/dev/ttyACM0`).
- Conclusion: the board is receiving **power only** (charge-only cable, or plugged into a charger / power-only port), not a data link to this laptop.

## Required user action
Connect the CYD's USB **data** port to this laptop with a known-good **data** cable
(many cables are charge-only). When `/dev/ttyUSB0` appears, the flow is:
1. Fresh 4 MB backup (`esptool read_flash 0x0 0x400000`) + checksum.
2. `pio run -e ESP32-2432S028R -t upload --upload-port /dev/ttyUSB0`.
3. Serial monitor @115200 — confirm boot, display init, SD mount, speaker boot-chime, web API start.
4. Configure WiFi/pool/wallet via WiFiManager portal or SD `config.json` (keep `pool.nerdminers.org:3333` + recovered wallet).
5. Run the on-device checklist in fork `docs/FLASH_GUIDE.md` (share beep, disconnect alarm, `/api/status`, `/api/speaker/test-tone`, `/api/sd/list`, dashboard panels live).

## Rollback
`esptool write_flash 0x0 <backup>.bin` (prior backup also at `hardware/backups/...2026-06-01.bin`).

## Validated WITHOUT hardware (so far)
- Firmware builds green; 73 tests pass @94% branch coverage; E2E proxy↔mock-firmware chain passes.
- On-device runtime behavior (audible tones, real SD I/O, accepted-share beep) is NOT yet observed — pending the flash above. Per honest-progress, not claimed as working until flashed + observed.
