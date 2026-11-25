# NMMiner ESP32 Series

## What This Is

Hardware bring-up, recovery, and custom firmware work for an ESP32 CYD NMMiner novelty miner. The repo preserves official firmware knowledge, reverse-engineers stock behavior as a specification, and maintains a custom NerdMiner-based fork for controlled on-device experiments.

## Core Value

The active v3.0 route is official NMMiner `v2.0.02` ST7789 binary
decompilation and local personal-use custom editing. The earlier open-fork
rebuild is retained as historical experiment/failure evidence and recovery
context, not the primary path.

## Requirements

Current v3.0 requirements live in `REQUIREMENTS.md`. As of 2026-06-19,
Phase 7 is re-routed to the official `v2.0.02` ST7789 binary baseline backed up
on 2026-06-17. Fork UI/touch validation remains useful failure evidence, but
new work should target the backed-up official app, extracted DROM/IROM, Ghidra
decompilation outputs, and local patch tooling.

## Purpose

Bring up, preserve, upgrade, and operate an NM-CYD / ESP32-2432S028 NMMiner novelty miner using official documentation and repeatable local procedures.

## Current Milestone: v3.0 Reverse-Engineer Official NMMiner v2.0.02 + Local Binary Customization

**Goal:** Treat the official NMMiner `v2.0.02` `esp32-2432s028r-st7789`
firmware as the baseline, decompile it deeply enough to identify safe custom
edit seams, and produce local-only modified artifacts without touching
license/activation paths, Wi-Fi credentials, wallet values, or redistributing
closed firmware.

**Target features:**
- Preserve a proven official `v2.0.02` ST7789 provenance chain from backup to
  local bundle to extracted app.
- Normalize Ghidra/decompiler entries for all six stock screens and runtime
  update paths.
- Build a binary edit target map covering literals, functions, replacement
  constraints, image checksums, and no-touch zones.
- Implement local-only patch tooling for selected safe edits.
- Keep mining and recovery reversible, with a fresh full backup before any
  future write.

**Approach:** Reverse-engineer and customize the backed-up official binary
directly. Do not continue fork UI/touch iteration as the primary route. The
custom fork may remain on the live device from prior diagnostics, but it is no
longer the target implementation path except for restore/reversibility context.

**Foundation already done:** ESP32 image parsed into 5 segments; DROM
(`0x3f400020`) and IROM (`0x400d0020`) isolated as
`hardware/recovered-assets/{drom,irom}.bin`; Xtensa `objdump` from the
PlatformIO toolchain works; confirmed LVGL (`lvgl_color_buf`), named screen
resources, NMMiner's own market/screensaver APIs (`/api/market/pairs`,
`/api/update/screensaver`), first literal patch tooling, and current runtime
anchors for Miner binding fan-out, Clock BTC price text, SPrice renderer/data
roles, and share-accept format dispatch. The latest Phase 7 pass corrects
`FUN_400ef24c` to parser helper/no-touch, maps Miner runtime/share/hashrate
evidence in `0x400f0858..0x400f1d2e`, classifies
`0x400f0846..0x400f13e8` as `mining.notify` parsing/internal job update rather
than UI publication, strengthens Miner publisher field semantics, maps the
Clock BTC text source record at `global+0x4f4` to a runtime market/ticker cache
read from `FUN_400d57c0()` -> `runtime+0x80`, proves the SPrice cache writer at
`0x400d73be..0x400d7519` from `runtime+0x74` into
`global+0xd94/global+0x1244`, and keeps raw WatchCoins source-vector
construction, Weather patch bytes, `ss_render`, and share-seam code paths
blocked as patch targets until proof and instruction safety are complete.

## Context

The device was previously configured for `TheBrickMansion`, which is no longer the target Wi-Fi. It is plugged into this laptop over USB serial as `/dev/ttyUSB0`.

Current live device state:

- Firmware: custom decoded-spec `ESP32-2432S028R` ST7789 fork reporting recovered NMMiner `v2.0.02`; not the official stock binary.
- Board target: `esp32-2432s028r-st7789`.
- Recovery AP: `nmap-2.4g` at `192.168.4.1`.
- Recovery web UI: `/` and `/info` return HTTP 200 in portal mode.
- Custom JSON API: returns 404 in recovery portal mode; API exposure/validation remains open.
- Decoded LittleFS assets: flashed at `0x310000` and verified after the latest app reflash.
- Touch: unresolved; active HSPI/direct-probe path reports no physical press signal.
- Official stock `v2.0.02`: verified active decompilation/custom-edit baseline.
  Version text alone does not distinguish stock from the prior custom rebuilt
  firmware.

The original device boot log identified:

- NMMiner firmware `v1.8.28`.
- Board model `cyd 2.8`.
- ESP32-D0WD-V3, dual-core, 240 MHz.
- License verification passes.
- Saved stats: 0 block hits, best difficulty 1590.028.
- Saved timezone: `-4.0`.
- Saved display settings: refresh interval 5 seconds, timeout 360 seconds, brightness 100, auto brightness enabled.

The public flasher now lists `v2.0.03`, but the repo's backed-up and decompiled
official baseline is `v2.0.02`. Do not silently switch versions. The unit's
current firmware image references `esp32-2432s028r-st7789`, so the ST7789 target
remains the correct board path.

## Constraints

- Preserve secrets and flash backups outside git.
- Do not erase flash before a verified full backup exists.
- ESP32 Wi-Fi is 2.4 GHz only.
- Intended long-term SSID is `Homebase`, the 2.4 GHz network.
- Current laptop connection `Homebase5` is visible only on 5 GHz channels in local scan, so it is not usable by this ESP32.
- Official stock documentation treated the TF slot as reserved for a future NMMiner version. The current custom fork does include SD work, but SD/API/dashboard behavior remains a Phase 7 validation item.

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
