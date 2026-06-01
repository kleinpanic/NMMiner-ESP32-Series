# NMMiner Customization Audit

Date: 2026-06-01.

## Current Miner Health

Live API checks on `http://192.168.1.136` show:

- Firmware: `v2.0.02`
- Hardware target: `esp32-2432s028r-st7789`
- Wi-Fi: `Homebase`
- RSSI during audit: about `-65 dBm`
- Timezone: `-4`
- Primary pool: `stratum+tcp://pool.nerdminers.org:3333`
- Fallback pool: `stratum+tcp://pool.nerdminers.org:3333`
- Shares during audit: accepted shares increasing, rejected shares `0`

The laptop remained connected to `Homebase5`; no AP-mode Wi-Fi switch was used.

## Pool Audit

The Amazon/manual PDF lists these BTC low-difficulty pools:

| Pool | Port | Manual status | Live test from this network |
| --- | ---: | --- | --- |
| `pool.nerdminers.org` | `3333` | BTC, minimum difficulty `0.001` | Works; returns `mining.subscribe` and difficulty `0.001`. |
| `public-pool.io` | `21496` | BTC, minimum difficulty `0.001` | TCP connects, but returned no subscribe response during audit. |
| `pool.tazmining.ch` | `33333` | BTC, minimum difficulty `0.001` | Timed out during audit. |

The current primary and fallback pool URLs are manual-correct. The fallback duplicates the only manual-listed pool that completed live subscribe testing. This is less redundant than using a separate fallback pool, but it is more reliable than configuring a manual-listed fallback that did not subscribe from this network.

The newer official NMMiner defaults are:

```text
Primary:  stratum+tcp://solobtc.nmminer.com:3333
Fallback: stratum+tcp://au.solobtc.nmminer.com:3333
```

Those are working official options, but they are not the pool URLs listed in the Amazon manual PDF.

## Source Availability

### Main firmware

Audited repo: `https://github.com/NMminer1024/NMMiner`

Latest GitHub release:

- Tag: `v2.0.02`
- Published: 2026-05-09
- Release asset: `firmware.rar`
- Current default branch commit cloned for audit: `3bc28303637081996f2d7cd85085ac34f6a9c943`

Findings:

- The repo contains README/API docs, images, drivers, tool binaries, and many prebuilt firmware binary folders.
- The repo does not contain a PlatformIO/Arduino firmware project, `src/`, `include/`, `platformio.ini`, `.ino`, or firmware `.cpp/.h` source files for the ESP32 miner.
- No repo license was reported by the GitHub API.
- The actual mining math, LVGL on-device pages, stratum implementation, and display logic are therefore not source-auditable from the published main repo.

Conclusion: treat the on-device firmware as binary-only unless upstream provides source separately.

### Official flasher

Audited site: `https://flash.nmminer.com/`

Current flasher files match our saved snapshots:

- `js/script.js` SHA-256 matched `docs/research/flash-nmminer-script.js`.
- `/firmware/devices_version.json` SHA-256 matched `docs/research/devices_version.json`.
- Manifest confirms `v2.0.02` supports `esp32-2432s028r-st7789` and `esp32-2432s028r-ili9341`.

The flasher uses this serial config schema:

```json
{
  "ssid": "...",
  "password": "...",
  "address": "...",
  "address2": "...",
  "primary_pool_url": "...",
  "primary_pool_password": "x",
  "fallback_pool_url": "...",
  "fallback_pool_password": "x"
}
```

For this board/version, the flasher places:

| Offset | File |
| --- | --- |
| `0x1000` | `bootloader.bin` |
| `0x8000` | `partitions.bin` |
| `0xe000` | `boot_app0.bin` |
| `0x390000` | `littlefs.bin` |
| `0x10000` | `firmware.bin` |

The flasher defaults to:

- Primary: `stratum+tcp://solobtc.nmminer.com:3333`
- Fallback: `stratum+tcp://au.solobtc.nmminer.com:3333`

## Embedded Web UI And LittleFS

The `v2.0.02` `littlefs.bin` was extracted successfully with 4 KB blocks and 96 blocks. It contains:

```text
index.html.gz
apidoc.html.gz
ss/saver_320_240_001.ss
ss/saver_320_240_002.ss
```

After decompression:

- `index.html`: 96,361 bytes
- `apidoc.html`: 52,734 bytes
- Existing screensavers: 69,975 bytes and 83,057 bytes

The embedded web UI exposes:

- Dashboard/status polling through `/api/system/info` and `/alive`
- Wi-Fi settings
- Pool/wallet settings
- Timezone/date format settings
- Brightness, rotation, LED, screensaver mode
- Market symbols and K-line chart settings
- Weather city/coordinates/unit settings
- Custom screensaver upload

The screensaver uploader is the safest on-device visual customization path. The web UI converts normal images client-side into NMMiner `.ss` files:

- RGB565
- RLE/Huffman compression
- Header magic `0x4E53`
- File name pattern like `saver_320_240_003.ss`
- Upload endpoint: `/api/update/screensaver`
- Preflight endpoint: `/api/update/screensaver/preflight?size=<bytes>`
- Firmware maximum upload size: 200 KB

Live preflight for an 80 KB image returned enough space with no overwrite required.

## Controller Code

### NMController Web

Audited repo: `https://github.com/NMminer1024/NMController_web`

Latest cloned commit:

- `ba048864f247ff4678ed75a47c00c6d88a6c23fb`
- Date: 2025-03-19

Source is available:

- Python Flask app
- Waitress server on port `7877`
- UDP listener on port `12345`
- Jinja HTML monitor template
- Hashrate formatting utilities
- Firmware latest-version check

Customization difficulty: low.

Good customization targets:

- Better local dashboard layout
- Rename/rebrand the monitor
- Add richer status cards for this one miner
- Add pool health and API readbacks
- Add a “configure Homebase miner” page using the documented HTTP API

### NMController Client

Audited repo: `https://github.com/NMminer1024/NMController_client`

Latest cloned commit:

- `0d16a22f048d2ca2449af8352ba7fdb235570986`
- Date: 2025-03-11

Source appears to be a C#/WPF Windows app. It is less useful on this Linux laptop than the Python web controller or a new local web dashboard.

## Amazon/Product Listing Audit

The Amazon page itself returned a generic Amazon page to direct `curl` access, so product details are based on the previously isolated product-description notes plus the official manual PDF hosted on Amazon media.

Product/listing implications:

- The hardware is an ESP32-D0WD-V3 CYD-style 2.8 inch display board.
- The seller positions it as an open-source/development style novelty miner, but that does not mean this exact firmware source is published.
- Stable power matters; the listing warns that weak power can cause restart loops and suggests a 5 V / 2 A supply.
- The official manual says the TF slot and external I/O are reserved for a future NMMiner version, so no SD-card customization path exists for this firmware.

## Customization Options

Low-risk, recommended:

- Configure/rename via HTTP API: hostname, pools, timezone, market list, weather, brightness, rotation, screensaver mode.
- Upload custom screensaver images through the embedded web UI or scripted `.ss` generation.
- Build a local dashboard against the NMMiner HTTP API.
- Customize NMController Web source locally.

Medium-risk:

- Repack `littlefs.bin` with edited `index.html.gz` or `apidoc.html.gz`, then flash only the LittleFS partition after validating a restore path.
- Build tooling to generate `.ss` screensavers outside the browser.
- Automate pool health testing and fallback selection.

High-risk / not recommended without upstream source:

- Patch `firmware.bin` directly.
- Modify on-device LVGL pages, mining math, stratum internals, or wallet validation.
- Flash a display target other than `esp32-2432s028r-st7789`.
- Erase full flash without first preserving NVS/license/config backup.

## Recommended Next Customization Plan

1. Keep the miner on `pool.nerdminers.org:3333` while it is submitting accepted shares.
2. Create a small local dashboard that polls `http://192.168.1.136/api/system/info`.
3. Add controls for safe settings only: hostname, display prefs, weather, market list, and pool config.
4. Add a screensaver generator/upload path for personalized images.
5. Only after that is stable, decide whether LittleFS web UI repacking is worth the risk.

## Sources

- Official flasher: `https://flash.nmminer.com/`
- Official main repo: `https://github.com/NMminer1024/NMMiner`
- Official latest release: `https://github.com/NMminer1024/NMMiner/releases/tag/v2.0.02`
- Official API reference: `https://github.com/NMminer1024/NMMiner/blob/main/docs/api-reference.md`
- NMController Web: `https://github.com/NMminer1024/NMController_web`
- NMController Client: `https://github.com/NMminer1024/NMController_client`
- Amazon manual PDF: `https://m.media-amazon.com/images/I/A1e9fPR2b%2BL.pdf?ref=dp_product_quick_view`
- Amazon listing: `https://www.amazon.com/dp/B0FX2SFCKV`
