---
phase: 07-on-device-validation
reviewed: 2026-06-19T03:25:12Z
depth: narrow-follow-up
status: clean
files_reviewed: 4
files_reviewed_list:
  - AGENTS.md
  - .planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-03.md
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
blocks_flash: no
blocks_physical_probe: no
build_required: no
live_device_flashed: no
---

# Phase 07 Current Firmware Review 04

**Reviewed:** 2026-06-19T03:25:12Z  
**Depth:** narrow follow-up  
**Status:** clean

## Summary

Reviewed only the requested closure questions against the current source. No hardware commands, flash/read/write/verify operations, serial monitor, Wi-Fi/AP mutation, build commands, source edits, or commits were run. The local build result and firmware hash referenced here are taken from the review prompt and prior artifact; this review did not rebuild.

No BLOCKER or WARNING finding remains in the requested patch areas. A future firmware write is not blocked by these source changes, but it must still be gated by a fresh full flash backup per `AGENTS.md`.

## Narrative Findings (AI reviewer)

No BLOCKER findings.

No WARNING findings.

## Closure Answers

### 1. Config Portal Serial Output

PASS. `readCustomAPName()` now logs only generic status strings and does not print the custom AP value. `configModeCallback()` still uses the active portal SSID for the on-device config screen, but the Serial line is redacted as `Config SSID: [redacted]`.

WiFiManager debug output is disabled on the setup path: `wm.setDebugOutput(false)` is set before the AP callback is registered and before either setup-time config portal loop is entered.

Evidence: `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:224`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:227`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:255`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:256`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:681`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:684`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:702`, `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:743`.

### 2. Canonical `.ss` Header Validation

PASS. The upload validator now requires the full canonical 8-byte header: magic `SN`, 320x240 little-endian dimensions, byte 6 `0x01`, and byte 7 `0xA4`.

Evidence: `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:623`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:624`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:659`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:664`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:665`.

### 3. `.ss` Upload Staging And Promotion

PASS. Upload start now opens `/ss/.upload.tmp`, not the final carousel path. Header bytes are accumulated and validated before buffered header data is written. Every staged write checks the returned byte count. The final carousel path is computed and renamed only from `handleScreensaverUploadDone()` after a complete, header-valid, non-oversized upload.

The only final-path removal path is the overwrite fallback during promotion, after the staged upload is already complete and valid. I did not find a path that opens, truncates, or removes a final `ss_saver_320_240_NNN.ss` file before validation.

Evidence: `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:714`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:722`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:724`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:731`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:739`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:742`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:755`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:762`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:795`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:797`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:813`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:818`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:823`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:829`.

### 4. New Flash-Blocking Patch-Area Issues

PASS. I did not find a new BLOCKER or WARNING in the reviewed patch areas that should block a future fresh-backup-gated flash.

The live device was not flashed by this review. The prior local unflashed `firmware.bin` build remains the referenced artifact: 2,827,504 bytes, SHA-256 `32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b`.

BLOCKS_FLASH? no  
BLOCKS_PHYSICAL_PROBE? no  
BUILD_REQUIRED? no
