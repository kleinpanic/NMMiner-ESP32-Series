---
phase: 07-on-device-validation
reviewed: 2026-06-19T03:15:43Z
fixed: 2026-06-19T03:20:07Z
depth: narrow-follow-up
status: fixed_in_source_unflashed
superseded_by: "2026-06-19 webfix2 app flash/readback, live app SHA-256 f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5"
files_reviewed: 5
files_reviewed_list:
  - AGENTS.md
  - .planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-02.md
  - .planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
supporting_files_checked:
  - scripts/nmminer_screensaver.py
  - scripts/nmminer_ss_decode.py
  - docs/nmminer-re-pipeline.md
findings:
  critical: 1
  warning: 2
  info: 3
  total: 6
original_blocks_flash: yes
blocks_flash: no
blocks_physical_probe: no
build_required: satisfied
post_fix_build:
  env: ESP32-2432S028R
  command: ".venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R"
  result: success
  firmware_bin_bytes: 2827504
  firmware_bin_sha256: "32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b"
  factory_bin_bytes: 2895872
  factory_bin_sha256: "8baddbe21a2272fba1cc0fc2f9af9fc4037b7db1497b7636f84388220d032510"
---

# Phase 07 Current Firmware Review 03

**Supersession:** This review is historical. Its local unflashed artifact and the then-live `6adc7771...` app were superseded by later fallback-pool/source-blocker/review-05/webfix2 app flashes. Current live authority is `07-VALIDATION.md`, `07-VERIFICATION.md`, and `07-CURRENT-FAILURE-STATE.md`; the live app is 2,840,400 bytes with SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

**Reviewed:** 2026-06-19T03:15:43Z  
**Depth:** narrow follow-up  
**Status:** fixed_in_source_unflashed

## Summary

Reviewed only the Phase 07 follow-up scope: the `readCustomAPName()` fix, the `.ss` upload hardening, patch-regression risks, and the named planning docs. No hardware commands, flash/read/write/verify operations, serial monitor, Wi-Fi/AP mutation, source edits, commits, or build commands were run.

The dangerous raw flash over-read in `readCustomAPName()` is fixed, and the function no longer logs the raw webflasher config body. At review time, the AP name loaded from that same flash config could still be printed to Serial through the WiFiManager AP callback, so CR-01 was not fully fixed under the requested "no raw config/AP payload leakage to Serial" criterion. That follow-up leak is now fixed in source and rebuilt.

At review time, the upload path buffered split 8-byte headers correctly and accepted canonical 320x240 / `0xA4` files, but the header predicate was still not a full canonical-header check and the final LittleFS destination was opened before validation. Those upload hardening warnings are now fixed in source and rebuilt.

## Post-Review Fix Status

Fixed after this review:

- `src/wManager.cpp`: config portal serial logging now redacts the portal SSID, and WiFiManager debug output is disabled for this build path.
- `src/webapi.cpp`: `.ss` upload validation now checks header byte 6 (`0x01`), stages uploads under `/ss/.upload.tmp`, checks `File::write()` byte counts, and promotes to the final carousel path only after a complete valid upload.

Verification:

- `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R` passed.
- Local unflashed `firmware.bin`: 2,827,504 bytes, SHA-256 `32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b`.
- Local unflashed merged factory image: 2,895,872 bytes, SHA-256 `8baddbe21a2272fba1cc0fc2f9af9fc4037b7db1497b7636f84388220d032510`.
- Historical note: at this review timestamp, the live device remained on the previously flashed and verified app SHA-256 `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1`. That was later superseded by intentional fresh-backup app writes, ending with the webfix2 live app SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

## Critical

### CR-01: Custom AP Name Still Reaches Serial Logs

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:224`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:226`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:228`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:251`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:256`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:257`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:630`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:631`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:684`

**Issue:** `readCustomAPName()` now bounds the flash read, but it still returns the `apname` value from the raw webflasher config. `init_WifiManager()` passes that value to WiFiManager as the config portal SSID, and `configModeCallback()` prints `myWiFiManager->getConfigPortalSSID()` to Serial. That means a custom AP name sourced from the raw flash config can still enter captured Serial logs. This does not preserve the full CR-01 guarantee requested for this follow-up.

**Fix:** Keep the AP name usable for the portal/UI, but do not print the value to Serial.

```cpp
void configModeCallback(WiFiManager* myWiFiManager)
{
    Serial.println("Entered Configuration Mode");
    showConfigPortalScreen(myWiFiManager->getConfigPortalSSID().c_str());

    Serial.println(F("Config SSID: [redacted]"));
    Serial.print("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
}
```

Also check WiFiManager debug output for this build profile; if library debug is enabled and emits AP SSIDs, disable or redact it before flashing a post-fix artifact.

## Warning

### WR-01: `.ss` Header Check Ignores The Canonical Flag Byte

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:658`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:663`  
**Supporting evidence:** `scripts/nmminer_screensaver.py:118`  
**Supporting evidence:** `scripts/nmminer_ss_decode.py:4`  
**Supporting evidence:** `docs/nmminer-re-pipeline.md:26`

**Issue:** The upload predicate validates magic, width, height, and `hdr[7] == 0xA4`, but it ignores `hdr[6]`. The repo's generator and docs define the 8-byte header as `"SN" + width + height + flags(0x01,0xA4)`. A file with `SN`, 320x240, arbitrary byte 6, and `0xA4` in byte 7 is accepted even though it is not the canonical full header.

**Fix:** Validate byte 6 explicitly.

```cpp
#define API_SS_FLAG        0x01

static bool apiSsHeaderValid(const uint8_t* hdr, size_t len) {
  if (!hdr || len < API_SS_HEADER_LEN) return false;
  if (hdr[0] != 0x53 || hdr[1] != 0x4E) return false;
  uint16_t w = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
  uint16_t h = (uint16_t)hdr[4] | ((uint16_t)hdr[5] << 8);
  return w == API_SS_WIDTH &&
         h == API_SS_HEIGHT &&
         hdr[6] == API_SS_FLAG &&
         hdr[7] == API_SS_HS_PARAMS;
}
```

### WR-02: Upload Opens The Final LittleFS Path Before Validation

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:648`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:655`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:714`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:757`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:758`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:774`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:779`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:784`

**Issue:** `UPLOAD_FILE_START` computes the final carousel path and opens it with `"w"` before any header byte has been validated. In the normal empty-slot case, an invalid upload creates and later removes only a new partial file. But when all 000..999 slots exist, `apiSsNextPath()` returns the existing `999.ss` path, so opening with `"w"` can truncate a valid screensaver before validation; an invalid upload then removes that path. The code also does not check `File::write()` return values, so a full or failing LittleFS write can still be reported as a successful upload.

**Fix:** Stage uploads under a non-carousel temporary path after the header validates, check every write result, and move/rename to the final `ss_saver_320_240_NNN.ss` path only after a complete valid upload. Do not remove or overwrite slot `999` until the new staged file is fully valid.

## Info/Notes

### IN-01: Bounded Flash Read Portion Of CR-01 Is Fixed

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:203`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:204`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:206`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:212`

`readCustomAPName()` now allocates `DATA_SIZE + 1`, zeroes the whole buffer, reads only `DATA_SIZE`, and constructs the `String` from a guaranteed-terminated buffer. I did not find the prior C-string over-read in the current source.

### IN-02: Split `.ss` Header Chunks Are Buffered Correctly

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:769`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:770`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:771`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:773`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:779`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:783`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:786`

The current state machine accumulates up to eight header bytes across `UPLOAD_FILE_WRITE` calls, validates only after all eight are present, writes the buffered header once, then writes the remaining bytes from the current chunk. Valid canonical files are still accepted by this path.

### IN-03: Named Docs Do Not Add Accidental Live-Device Write Instructions

**File:** `.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-02.md:75`  
**File:** `.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-02.md:91`  
**File:** `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md:90`  
**File:** `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md:94`  
**File:** `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md:96`

The named planning docs preserve the unflashed status of the local build and warn against flashing without a fresh backup. I did not find a new instruction to run a live-device write, reset, serial monitor, Wi-Fi/AP mutation, or ILI9341 flash as part of these fixes.

## Verdict

**BLOCKS_FLASH? no for current source, yes for any stale pre-fix artifact.** The source-side over-read, AP-name serial leak, canonical header gap, and final-path-before-validation upload behavior are fixed in source and rebuilt. Still preserve a fresh full flash backup before any firmware write.

**BLOCKS_PHYSICAL_PROBE? no.** These findings do not change the current raw no-contact touch classification or block non-mutating physical touch investigation.

**BUILD_REQUIRED? satisfied.** The required fixes touched firmware source and the `ESP32-2432S028R` build passed.

---

_Reviewed: 2026-06-19T03:15:43Z_  
_Reviewer: the agent (gsd-code-reviewer)_  
_Depth: narrow follow-up_
