# NMMiner ESP32 Series

## Purpose

Bring up, preserve, upgrade, and operate an NM-CYD / ESP32-2432S028 NMMiner novelty miner using official documentation and repeatable local procedures.

## Current Milestone: v3.0 Reverse-Engineer NMMiner + Re-integrate Custom Features

**Goal:** Recover the original stock NMMiner v2.0.02 on-device UI, screens, screensavers, market data, and interactive touch by reverse-engineering the closed binary (full 4 MB backup in hand) — then re-add our speaker (LEDC), SD card (VSPI), LED, and kleinpanic branding on top. Result = original NMMiner look + functionality + interactivity, PLUS our custom upgrades.

**Target features:**
- Reverse-engineer the NMMiner binary: recover screen/LVGL assets, map the render + share-accept + screensaver/market code paths.
- Restore the original on-device look (screens, screensavers, market screens) and interactive touch.
- Speaker / audio integration (LEDC, GPIO26) — beep on share, alarm on disconnect.
- SD card integration (VSPI, GPIO5/18/19/23).
- kleinpanic branding (boot splash + accents) over the recovered UI.
- Keep mining fully functional.

**Approach:** Path B (reverse-engineer) from `docs/firmware-customization-feasibility.md`, now chosen because the NMMiner look became non-negotiable. The open-source NerdMiner_v2 fork (`firmware/custom/NerdMiner-CYD-Custom`, milestone v2.0.02) is retained as a working fallback only.

**Foundation already done:** ESP32 image parsed into 5 segments; DROM (`0x3f400020`) and IROM (`0x400d0020`) isolated as `hardware/recovered-assets/{drom,irom}.bin`; Xtensa `objdump` from the PlatformIO toolchain works; confirmed LVGL (`lvgl_color_buf`), named screen resources (`Clock320x240`), and NMMiner's own market/screensaver APIs (`/api/market/pairs`, `/api/update/screensaver`).

## Context

The device was previously configured for `TheBrickMansion`, which is no longer the target Wi-Fi. It is plugged into this laptop over USB serial as `/dev/ttyUSB0`.

Current baseline setup:

- Firmware: official `v2.0.02`.
- Board target: `esp32-2432s028r-st7789`.
- Wi-Fi: `Homebase` 2.4 GHz.
- LAN API: `http://192.168.1.136`.
- Primary pool: `stratum+tcp://pool.nerdminers.org:3333`.
- Fallback pool: `stratum+tcp://pool.nerdminers.org:3333`.
- Timezone: `-4`; NTP sync observed.
- Wallet: recovered BTC mainnet P2SH address configured and validated.

The original device boot log identified:

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
