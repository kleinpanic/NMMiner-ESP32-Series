# State

Last updated: 2026-06-01.

## Completed

- Git repository initialized.
- Manual PDF downloaded to `docs/vendor/NM-CYD-ESP32-2432S028-manual.pdf`.
- Manual text extracted to `docs/vendor/NM-CYD-ESP32-2432S028-manual.txt`.
- Official GitHub README, API reference, flasher JS, and version manifest saved under `docs/research/`.
- Device detected on `/dev/ttyUSB0`.
- Full 4 MB flash backup completed.
- Backup SHA-256: `a8c7cae425f1bb206d49adf4be802bda91b626035a1bf5c1d9fafd0cfe6715e4`.
- Current firmware: `v1.8.28`.
- Latest official firmware: `v2.0.02`.
- Display target: `esp32-2432s028r-st7789`.
- `nmap-2.4g` AP observed locally.
- User clarified that `Homebase` exists and is the 2.4 GHz SSID.
- `NMController_web` README, main Python file, and web monitor template snapshots downloaded.

## Needs User Input

- `Homebase` Wi-Fi password.
- Confirmation whether to flash `v2.0.02` now or keep the working `v1.8.28` until Wi-Fi is configured.

## Risk Notes

- `Homebase5` was observed only on 5 GHz channels during local scan; use `Homebase` for this ESP32.
- Flash backup and recovered config contain secrets and are gitignored.
- Firecrawl could not scrape the Amazon page because the account has insufficient credits.
