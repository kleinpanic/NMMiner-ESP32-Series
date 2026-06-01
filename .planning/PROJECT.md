# NMMiner ESP32 Series

## Purpose

Bring up, preserve, upgrade, and operate an NM-CYD / ESP32-2432S028 NMMiner novelty miner using official documentation and repeatable local procedures.

## Context

The device was previously configured for `TheBrickMansion`, which is no longer the target Wi-Fi. It is plugged into this laptop over USB serial as `/dev/ttyUSB0`.

The device boot log identifies:

- NMMiner firmware `v1.8.28`.
- Board model `cyd 2.8`.
- ESP32-D0WD-V3, dual-core, 240 MHz.
- License verification passes.
- Saved stats: 0 block hits, best difficulty 1590.028.
- Saved timezone: `-4.0`.
- Saved display settings: refresh interval 5 seconds, timeout 360 seconds, brightness 100, auto brightness enabled.

The latest official firmware found is `v2.0.02`. The unit's current firmware image references `esp32-2432s028r-st7789`, so the ST7789 target is the correct upgrade path.

## Constraints

- Preserve secrets and flash backups outside git.
- Do not erase flash before a verified full backup exists.
- ESP32 Wi-Fi is 2.4 GHz only.
- Intended long-term SSID is `Homebase`, the 2.4 GHz network.
- Current laptop connection `Homebase5` is visible only on 5 GHz channels in local scan, so it is not usable by this ESP32.
- No SD/TF card setup is needed for current firmware; the manual says the TF slot is reserved for a future NMMiner version.

## Customization Interest

Future work should audit what code gives rise to the visuals, monitor UI, firmware behavior, and mining math. The official NMMiner firmware repo appears to publish prebuilt firmware binaries rather than the full ESP32 source, so customization may split into:

- Open/inspectable code: `NMController_web`, official flash tool JavaScript, HTTP API surface, web monitor assets, and strings/assets inside LittleFS/firmware.
- Closed/prebuilt code: on-device LVGL screens and mining core, unless upstream source is obtained.

## Sources

- Amazon manual PDF: `docs/vendor/NM-CYD-ESP32-2432S028-manual.pdf`.
- Official NMMiner README snapshot: `docs/research/nmminer-github-readme.md`.
- Official NMMiner API reference snapshot: `docs/research/nmminer-api-reference.md`.
- Official flasher HTML/JS snapshots: `docs/research/flash-nmminer.html`, `docs/research/flash-nmminer-script.js`.
- Official version manifest: `docs/research/devices_version.json`.
- NMController Web snapshots: `docs/research/nmcontroller-web-*`.
