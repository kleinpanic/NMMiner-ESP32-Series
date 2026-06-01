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
- Original firmware: `v1.8.28`.
- Latest official firmware: `v2.0.02`.
- Display target: `esp32-2432s028r-st7789`.
- `nmap-2.4g` AP observed locally.
- User clarified that `Homebase` exists and is the 2.4 GHz SSID.
- `NMController_web` README, main Python file, and web monitor template snapshots downloaded.
- Firmware upgraded to official `v2.0.02` using the `esp32-2432s028r-st7789` target.
- License verification passes after upgrade.
- Serial config sent for `Homebase` without disconnecting the laptop from `Homebase5`.
- Device joined LAN at `192.168.1.136`.
- API verified firmware `v2.0.02`, hostname `cyd2.8_1a76f7`, SSID `Homebase`, timezone `-4`, and NTP sync.
- Recovered BTC wallet is configured for both primary and fallback pools.
- Runtime pool testing selected `pool.nerdminers.org:3333` for both primary and fallback so both configured URLs match the Amazon manual PDF and the only PDF-listed pool that completed live subscribe testing.
- Post-reboot API showed active mining on `pool.nerdminers.org:3333`, accepted shares increasing, and zero rejected shares.
- Customization audit completed in `docs/customization-audit.md`.
- Fresh official flasher snapshots match saved snapshots; `v2.0.02` still supports `esp32-2432s028r-st7789`.
- Main firmware repo audit found prebuilt firmware binaries but no published ESP32 firmware source tree.
- LittleFS extraction confirmed embedded web UI and screensaver assets are inspectable.
- GitHub profile review completed for `kleinpanic` and saved in `docs/kleinpanic-customization-basis.md`.
- Official NMminer1024 repo review confirmed `NMminer1024/NMMiner` is the repo for this ESP32 software bundle; no separate public source repo for this CYD firmware was found.
- Confirmed the running `/home/broklein/codeWS/Python/NMController_web` monitor pre-existed this repo work and is a clean upstream checkout, not a customized local dashboard.
- Canonical local dashboard added under `apps/nmminer-control`.
- Local dashboard is running at `http://127.0.0.1:8788`.
- Dashboard implementation split live status polling from settings loading to reduce request pressure on the ESP32.
- A normal API restart recovered mining after a temporary zero-hash state; post-restart readbacks showed about `1.04 MH/s` and accepted shares increasing.

## Needs User Input

- Hardware photos or pin measurements before wiring JST, CH1/CN1, P3, speaker, or SD/TF modifications.

## Risk Notes

- `Homebase5` was observed only on 5 GHz channels during local scan; use `Homebase` for this ESP32.
- Flash backup and recovered config contain secrets and are gitignored.
- Firecrawl could not scrape the Amazon page because the account has insufficient credits.
- Do not connect the laptop to the miner AP while it needs to stay online through `Homebase5`; use USB serial and LAN API.

## Session Continuity

Last session: 2026-06-01.
Stopped at: Baseline setup and customization/profile audit complete; ready to implement safe customizations.
Resume file: none.
