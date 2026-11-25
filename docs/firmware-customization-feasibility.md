# Firmware Customization Feasibility — SD Card + Speaker

Date: 2026-06-03.
Goal: add **SD card** + **speaker** support the local website/dashboard can drive. User has physically installed a microSD card and a speaker on the CYD board.

> Current-state note, 2026-06-19: this is a feasibility snapshot from before the
> v3.0 reverse-engineering/rebuild work. The project did choose a custom
> NerdMiner-derived fork, but now uses the recovered NMMiner binary/assets as a
> read-only spec. The live unit is the custom decoded-spec ST7789 fork in
> WiFiManager recovery AP mode, not the old stock LAN miner at `192.168.1.136`.
> Use `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md` for
> the active state.

## TL;DR

The stock **NMMiner v2.0.02 firmware cannot do this and cannot be extended cleanly** — it is binary-only (no source, no license) and exposes **zero** SD or audio in its HTTP API. The realistic path is **custom firmware forked from the MIT-licensed open-source NerdMiner v2**, borrowing CYD SD + speaker code from the canonical CYD reference repo. Binary reverse-engineering of the NMMiner image is *possible* but is the worst ROI (high effort, fragile, lost on every update).

Recommendation: **Path A — fork NerdMiner v2 (MIT) + graft CYD SD/audio from witnessmenow's reference repo.**

## Verified evidence

### Stock NMMiner v2.0.02 (historical unit state at 192.168.1.136)
- Live API `storage` block = LittleFS only (`fsTotal:393216`). No SD, no audio fields.
- Official HTTP API = 21 endpoints; **none** for SD or audio (confirmed against live `NMminer1024/NMMiner/docs/api-reference.md`).
- Manual: "external I/O ports and TF slot will be used **next** NMMiner version."
- Repo `NMminer1024/NMMiner` = **binaries only**, **no LICENSE**, no `src/`/`platformio.ini` (re-confirmed 2026-06-03).
- Firmware binary fingerprint: Arduino-ESP32 on **ESP-IDF v4.4.6**, compiled Oct 2023, 2.79 MB image, 5 segments, valid SHA256.
- Binary **already links** the SD driver (`Creating new VSPI instance for SD card`, `FAT32`, FatFS `diskio.c`) and **LEDC** (`/IDF/components/driver/ledc.c`) — LEDC is the PWM peripheral used to drive a passive buzzer/speaker tone. So both peripherals' HALs are present; only the *use* code + API surface are missing.
- App partition 0x10000→0x390000 = 3.67 MB holding 2.79 MB ⇒ ~880 KB code-cave headroom.

### NerdMiner v2 (BitMaker-hub/NerdMiner_v2) — the open base NMMiner derives from
- **License: MIT** (clean to fork, modify, redistribute).
- PlatformIO + `src/`, framework = arduino (same family as NMMiner).
- Explicit `ESP32-2432S028R` build env (also `ESP32_2432S028_2USB`). Note: mainline CYD env uses **ILI9341** driver; our unit is the **ST7789** variant — must set the correct TFT_eSPI driver. Both CYD display variants are common; community ST7789 setups exist.
- Web monitor exists (mining data, clock screen, global stats). Stock web API is minimal (feature-requested in issue #223) — but we control it once forked.
- **SD card supported**: boot config via `config.json` on a FAT32 SD. S3-GEEK env exposes parameterized `SDSPI_CLK/MOSI/MISO/CS` + `SD_ID` defines ⇒ the SD SPI class is reusable; wire to CYD VSPI pins.
- **No audio** in stock NerdMiner — speaker support must be added (trivial with LEDC `tone()`).

### CYD hardware reference
- **witnessmenow/ESP32-Cheap-Yellow-Display** — canonical CYD repo: documents microSD slot, RGB LED, and **speaker connector**, with working SD + audio example sketches for this exact board. Primary source for SD file I/O and speaker driving code.
- CYD pin leads (verify against silkscreen/multimeter before wiring): speaker/buzzer commonly `GPIO26`; microSD over SPI `GPIO5(CS)/18(CLK)/19(MISO)/23(MOSI)`.

## The three paths

| | A. Fork NerdMiner v2 (recommended) | B. Reverse-engineer NMMiner binary | C. Stay on NMMiner, no firmware change |
|---|---|---|---|
| SD card | Add SD file API in C++ (driver exists) | Inject web handlers + SD glue blind | Impossible (no API) |
| Speaker | Add LEDC `tone()` in C++ | Author Xtensa audio + find call sites blind | Impossible (no audio code) |
| Web/API | We own it — add any endpoint | Patch route table + inject handlers | Rich but closed; no SD/audio |
| License | MIT — clean | No license; patching a no-license binary | N/A |
| Keep NMMiner UI/screensavers/market | No (different firmware) | Yes | Yes |
| Effort | Moderate, maintainable | Very high, fragile, lost on updates | Zero |
| Risk to miner | Recoverable (full flash backup exists) | Recoverable, but RE is error-prone | None |

Path B is documented as *possible* (tooling: Ghidra Xtensa loader, `esp32_image_parser`, Tarlogic FIDB for IDF v4.4.6 symbol recovery, BlackVS/ESP32-reversing) and kept as a fallback if NMMiner-specific features become non-negotiable. It is not the primary path.

## GitHub projects to adopt (per "gh projects to take parts of")
- `BitMaker-hub/NerdMiner_v2` (MIT) — **base fork**. Mining core, stratum, WiFiManager portal, display, SD config.
- `witnessmenow/ESP32-Cheap-Yellow-Display` — **SD + speaker + RGB LED** example code and verified CYD pinout.
- `Fr4nkFletcher/NerdMiner_v2-Cheap-Yellow-Display` / `NorthernGadgets/ESP32-2432S024C_NerdMiner_v2` — CYD-specific NerdMiner forks to crib display/driver config from.
- `NMminer1024/ESP-Miner-NMAxe` — same vendor's *open* (ASIC/NMAxe) repo; reference only, not for this CYD board.

## What this means for the website
The dashboard (`apps/nmminer-control`) currently polls NMMiner's `/api/system/info` schema. Custom firmware = a new API we define. Plan: define a small JSON status + control API on the fork (mining stats + `sd` block + `speaker` control), and re-point/extend the dashboard to it. The UI upgrade + Playwright visual review can proceed on the dashboard shell in parallel, independent of firmware path.

## Sources
- NMMiner repo (binary-only, no license): https://github.com/NMminer1024/NMMiner
- NMMiner API ref: https://github.com/NMminer1024/NMMiner/blob/main/docs/api-reference.md
- NerdMiner v2 (MIT): https://github.com/BitMaker-hub/NerdMiner_v2 — LICENSE = MIT
- CYD reference (SD + speaker): https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
- CYD pinout: https://randomnerdtutorials.com/cheap-yellow-display-esp32-2432s028r/
- ESP32 RE tooling: https://github.com/BlackVS/ESP32-reversing , https://www.tarlogic.com/blog/esp32-firmware-using-ghidra-fidb/
