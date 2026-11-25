---
phase: 07-on-device-validation
reviewed: 2026-06-19T03:00:11Z
fixed: 2026-06-19T03:04:45Z
superseded_by: ".planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-03.md"
depth: standard
status: superseded_by_review_03
files_reviewed: 37
files_reviewed_list:
  - AGENTS.md
  - .planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md
  - .planning/phases/07-on-device-validation/07-VERIFICATION.md
  - .planning/phases/07-on-device-validation/07-VALIDATION.md
  - .planning/debug/touch-no-press.md
  - .planning/phases/07-on-device-validation/07-TOUCH-PROBE-RUNBOOK.md
  - .planning/phases/07-on-device-validation/07-CURRENT-CODE-REVIEW.md
  - .planning/phases/07-on-device-validation/07-TOUCH-SOFTWARE-REVIEW.md
  - .planning/phases/07-on-device-validation/07-CODEX-REVIEW.md
  - firmware/custom/NerdMiner-CYD-Custom/.gitignore
  - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
  - firmware/custom/NerdMiner-CYD-Custom/include/webapi.h
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/SDCard.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/storage.h
  - firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
  - firmware/custom/NerdMiner-CYD-Custom/src/version.h
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
findings:
  critical: 1
  warning: 2
  info: 5
  total: 8
original_blocks_flash: yes
blocks_flash: no
blocks_physical_probe: no
local_build_recommended_after_source_change: satisfied
post_fix_build:
  superseded: true
  env: ESP32-2432S028R
  command: ".venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R"
  result: success
  firmware_bin_bytes: 2826256
  firmware_bin_sha256: "1d81348f8015b02e26f039aa3f1893010840afc046e0111a08bad9691d985945"
  factory_bin_bytes: 2895872
  factory_bin_sha256: "d9be52d142451f65dd854afbffec71d2ef7bcf681b0509b039a99555aca819c0"
---

# Phase 07 Current Firmware Review 02

**Reviewed:** 2026-06-19T03:00:11Z  
**Depth:** standard  
**Status:** fixed_in_source_unflashed

## Summary

Reviewed the Phase 07 firmware recovery source and planning context for the ST7789 CYD target, recovery AP/config portal behavior, touch raw path, storage ownership, web API mutation gates, redaction, LED/display side effects, and the recent `doLedStuff()` no-op.

No hardware commands, serial monitor, flash reads/writes/verifies, uploads, resets, Wi-Fi/AP mutations, or build commands were run. This review was source/docs only, with this artifact as the only write.

At review time, the main remaining blocker was a boot-time raw flash read that was converted to an unterminated C string and printed to Serial. That created both an out-of-bounds read risk and a serial-log data leakage risk. This was fixed in source after the review and rebuilt successfully. The current touch failure still matches a raw no-contact condition below UI/LVGL/calibration; no reviewed source issue blocks the physical touch probe.

## Post-Review Fix Status

**Superseded:** `07-CURRENT-FIRMWARE-REVIEW-03.md` found follow-up issues after
this artifact was written. The current local unflashed build is now
2,827,504 bytes, SHA-256
`32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b`.

Fixed after this review:

- `src/wManager.cpp`: `readCustomAPName()` now reads into a terminated buffer, parses only bounded data, and logs redacted status instead of raw flash/config payloads.
- `src/webapi.cpp`: screensaver upload now buffers and validates the full 8-byte `.ss` header before writing, and accepts only the intended 320x240 / `0xA4` header for this panel path.

Verification:

- `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R` passed.
- Superseded local unflashed `firmware.bin`: 2,826,256 bytes, SHA-256 `1d81348f8015b02e26f039aa3f1893010840afc046e0111a08bad9691d985945`.
- Superseded local unflashed merged factory image: 2,895,872 bytes, SHA-256 `d9be52d142451f65dd854afbffec71d2ef7bcf681b0509b039a99555aca819c0`.
- Historical note: at this review timestamp, the live device remained on the previously flashed and verified app SHA-256 `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1`. That was later superseded by intentional fresh-backup app writes, ending with the webfix2 live app SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

## Critical

### CR-01: Raw Flash Config Read Can Over-Read And Leak Data To Serial

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:210`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:217`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:220`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:226`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:239`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:301`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:653`

**Issue:** `readCustomAPName()` reads 128 bytes from raw flash at `0x3F0000`, overwrites the entire fixed buffer, then constructs `String((char*)buffer)` without guaranteeing a trailing NUL. If those 128 bytes do not contain a zero byte, the C-string conversion can read beyond the buffer. The same function also prints the raw flash string, parsed JSON substring, and AP name to Serial on boot and config-portal entry. In this recovery repo, serial logs are likely to be captured during Phase 07, and the project explicitly treats serial logs and device/user configuration as sensitive. This is a real source-side leakage and robustness blocker before flashing a new firmware built from this code.

**Fix:** Add an explicit terminator, stop logging raw payloads/field values, and only log redacted status.

```cpp
String readCustomAPName() {
    static constexpr size_t DATA_SIZE = 128;
    char buffer[DATA_SIZE + 1];
    memset(buffer, 0, sizeof(buffer));

    esp_err_t result = esp_flash_read(NULL, buffer, 0x3F0000, DATA_SIZE);
    if (result != ESP_OK) {
        Serial.println(F("[CFG] Webflasher config read failed"));
        return "";
    }

    String data(buffer);
    if (!data.startsWith("WEBFLASHER_CONFIG:")) {
        return "";
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, data.substring(18));
    if (error != DeserializationError::Ok || !doc.containsKey("apname")) {
        return "";
    }

    String customAP = doc["apname"].as<String>();
    customAP.trim();
    if (customAP.length() == 0 || customAP.length() >= 32) {
        return "";
    }

    Serial.println(F("[CFG] Custom AP name loaded from webflasher config"));
    return customAP;
}
```

## Warning

### WR-01: Prior API Mutation Finding Is A Product Behavior Warning, Not A Security Blocker

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:107`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:120`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:150`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:175`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:81`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:761`

**Issue:** The old WR-02 concern is not an active unauthenticated mutation window: `suspendWebApiForPortal()` sets config mode and stops the API server, and `resumeWebApiAfterPortal()` clears config mode before restarting the API. That means protected POST/upload/delete/restart routes are not accepted during the recovery portal. However, it also means these protected mutation endpoints have no reachable success path in the current product behavior. This is not a flash blocker for recovery, but it remains a behavior warning if Phase 07 expects API-based settings, upload, delete, restart, speaker, weather, or swarm mutations to work.

**Fix:** Pick and document one intended behavior. Either keep mutations intentionally disabled outside a future authenticated/admin flow and remove them from Phase 07 UAT, or implement a deliberate mutation window that keeps the API server active only after explicit local/physical intent and with an authorization mechanism.

### WR-02: Screensaver Upload Validates Magic And Size But Not Render Dimensions

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:740`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:744`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:748`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.h:34`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp:167`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp:171`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp:193`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp:212`

**Issue:** The upload handler only checks the two-byte screensaver magic and a 200 KB cap before storing the file. The decoder accepts headers up to 800x600, clamps only the render row width to 320, and decodes `w * h` pixels. If a mutation path is made reachable later, a malformed but magic-valid upload can be stored and later drive excessive/off-panel decode behavior on a 320x240 display. This is not currently reachable through the stopped-and-locked mutation gate above, but it becomes a correctness and robustness issue as soon as uploads are intentionally enabled.

**Fix:** Validate the complete 8-byte header before accepting the upload, and reject dimensions that are not intended for this panel and filename family. Mirror the decoder check as a defensive fallback.

```cpp
static bool isValidScreensaverHeader(const uint8_t* hdr, size_t n) {
    if (n < 8) return false;
    if (hdr[0] != 0x53 || hdr[1] != 0x4E) return false;
    uint16_t w = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
    uint16_t h = (uint16_t)hdr[4] | ((uint16_t)hdr[5] << 8);
    uint8_t params = hdr[7];
    return w == 320 && h == 240 && params == 0xA4;
}
```

## Info/Notes

### IN-01: ST7789 Target Selection Matches The Observed Board

**File:** `firmware/custom/NerdMiner-CYD-Custom/platformio.ini:14`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/platformio.ini:961`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/platformio.ini:979`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/platformio.ini:983`

The default environment is `ESP32-2432S028R`, with `ESP32_2432S028R=1` and `ST7789_DRIVER=1`. I did not find the observed board being redirected to an ILI9341 target in the reviewed source.

### IN-02: `doLedStuff()` No-Op Is Sufficient For The Current CYD Runtime Path

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp:163`

The wrapper now returns immediately for the CYD targets, so the legacy monitor-task LED/touch path is dormant in the rebuilt UI. This closes the prior second-touch-reader risk for source built from the current tree.

### IN-03: Touch Failure Still Looks Below UI/LVGL/Calibration

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp:121`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp:128`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp:281`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp:293`

The active touch path initializes HSPI directly, avoids the formatting side effect from `TFT_eTouch::init()`, and logs both direct and library raw state. Nothing in the reviewed source contradicts the current raw no-contact observation (`irq=1`, `miso=0`, direct sample not pressed). The next touch gate remains physical power-cycle confirmation and hardware probing.

### IN-04: LittleFS Runtime Formatting Is Still Fail-Closed

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp:272`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:730`

The reviewed LittleFS users mount with `begin(false)` and preserve the decoded asset partition on mount failure. I did not find a regression to runtime formatting in the reviewed Phase 07 source paths.

### IN-05: NVS/LittleFS/SD Ownership Looks Consistent For The CYD Target

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:69`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:82`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:184`  
**File:** `firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp:6`

The legacy `SDCard` global remains compiled out for the CYD target, `SDStore` owns the runtime SD path, and SD uses VSPI while the current touch path uses HSPI. I did not find an active storage ownership regression that would explain the current touch no-contact failure.

## Verdict

**BLOCKS_FLASH? no for current source, yes for any stale pre-fix artifact.** CR-01 is fixed in source and rebuilt. Still preserve a fresh full flash backup before any firmware write.

**BLOCKS_PHYSICAL_PROBE? no.** The current next gate can remain the no-write physical hard power-cycle confirmation and touch hardware probing.

**LOCAL_BUILD_RECOMMENDED_AFTER_SOURCE_CHANGE? yes.** After CR-01 and any chosen warning fixes, run a local build for `ESP32-2432S028R` before any future fresh-backup-gated flash.
