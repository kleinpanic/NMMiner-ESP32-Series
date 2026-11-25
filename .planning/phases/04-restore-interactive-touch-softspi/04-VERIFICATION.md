---
phase: 04-restore-interactive-touch-softspi
verified: 2026-06-18T02:10:00Z
status: human_needed
score: 8/8 must-haves verified (build + code-structure layer)
current_runtime_authority: ".planning/phases/07-on-device-validation/07-VERIFICATION.md"
supersession_note: "Historical SoftSPI verification only; current Phase 7 runtime uses NM_TOUCH_NO_VSPI HSPI/TFT_eTouch/direct probes and still reports no contact."
overrides_applied: 0
build_gate: PASS
ram_used_pct: 35.3
flash_used_pct: 87.4
re_verification:
human_verification:
  - test: "On-device single tap on the panel"
    expected: "Carousel advances one page; LastPage persists across reboot"
    why_human: "Physical touch behavior only confirmable on real hardware; current Phase 7 evidence shows no raw contact"
  - test: "On-device 10s touch hold"
    expected: "Device opens WiFiManager AP captive portal (192.168.4.1) WITHOUT wiping wallet/WiFi creds"
    why_human: "Config-portal entry + non-destructiveness needs runtime + NVS inspection on device"
  - test: "On-device tap while screen is dimmed"
    expected: "Backlight ramps back up (wake), no page change on that tap"
    why_human: "Backlight PWM + idle-dim timing observable only on hardware"
  - test: "Boot button double-click / long-press / single-click"
    expected: "double=next page, long(10s)=config portal, single=wake"
    why_human: "OneButton GPIO0 timing + physical button behavior is hardware-only"
diagnosed_runtime:
  - test: "Raw touch serial diagnosis"
    result: "Ran in Phase 7 and repeatedly reported irq=1, miso=0, directX=0, directY=4095, directPressed=0, libPressed=0"
    status: "diagnosed_no_contact"
    next_action: "Physical XPT2046/FPC/PENIRQ probe or touch-variant re-identification; not another generic serial wait"
---

# Phase 4: Restore Interactive Touch (SoftSPI) Verification Report

## 2026-06-19 Supersession Note

This report is preserved as the historical Phase 4 build/code-structure verification of the earlier software-SPI touch implementation. It is not the current runtime touch authority.

The live Phase 7 custom firmware is built with `TOUCH_ENABLE` and `NM_TOUCH_NO_VSPI`, gates out the legacy VSPI `TouchHandler`, and uses HSPI `TFT_eTouch` plus direct HSPI probes on the observed `esp32-2432s028r-st7789` pins. Live serial evidence still reports no touch contact (`directPressed=0` / `libPressed=0`). Any Phase 4 claims below that touch is "pure bit-bang", "SoftSPI", or hardware-verified must be read as historical and superseded by Phase 7 validation and `.planning/debug/touch-no-press.md`.

**Phase Goal:** The dead CYD touch works on software SPI (VSPI free for SD) + drives the nm_uimanager carousel; boot button too.
**Verified:** 2026-06-18
**Status:** human_needed (all build + code-structure must-haves VERIFIED; on-device tap/hold/wake confirmation is Phase 7 by design)
**Re-verification:** No — initial verification

## Verifier Note: SUMMARY/VALIDATION docs predate the fix commits

The 04-01/04-02 SUMMARYs and 04-VALIDATION.md describe the PRE-review state — they
state long-press → `reset_configuration()` (destructive), raw-Z `nm_touch_log_raw`
NOT called, and a blocking `delay()` wake. **The codebase has since been fixed**
(commits `1e6ce9c`, `d31d156`, `824961f`, `fe30e8e` — all `fix(04): ...`). This
report verifies the ACTUAL code in HEAD, not the stale docs. Every claimed Critical /
WR-01 fix is confirmed present in source.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | TOUCH-01: touch on SoftSPI, raw-Z diagnosis path | ✓ VERIFIED | nm_touch.cpp is pure bit-bang (digitalWrite/digitalRead on CLK25/MOSI32/MISO39/CS33); no SPIClass/VSPI/xpt2046.h in code (only in doc comments); `nm_touch_log_raw` CALLED at ino.cpp:338 (CR-01 fix) |
| 2 | TOUCH-01: rotation-1 calibration constants present + exercised | ✓ VERIFIED | NM_TOUCH_RAW_X/Y_MIN/MAX, W/H, INVERT, Z_THRESHOLD in nm_touch.cpp:47-64; `nm_touch_read_point` mapped + CALLED at ino.cpp:340 (no longer dead code) |
| 3 | TOUCH-02: short tap→next, long(10s)→config, tap-while-dim→wake | ✓ VERIFIED | nm_poll_touch_gesture (ino.cpp:306-363) classifies SHORT/LONG/WAKE; on_touch routes SHORT→next_page, LONG→enter_config_portal, dim→wake (nm_uimanager.cpp:137-160); NM_TOUCH_LONG_MS=10000 |
| 4 | TOUCH-02: debounce/cooldown (WR-04) | ✓ VERIFIED | NM_TOUCH_TAP_COOLDOWN_MS=600 + NM_TOUCH_PRESS_SAMPLES=2 consecutive-sample streak (ino.cpp:293-325) |
| 5 | TOUCH-03: boot button double=next/long=config/single=wake | ✓ VERIFIED | CYD inner guard (ino.cpp:98-108): attachDoubleClick→on_button_next, attachLongPressStart→on_button_config, attachClick→on_button_wake; setPressMs(NM_TOUCH_LONG_MS) |
| 6 | Safety WR-01: long-press → enter_config_portal (non-destructive), NOT reset_configuration | ✓ VERIFIED | nm_uimanager.cpp:155,172 call `enter_config_portal()`; enter_config_portal (wManager.cpp:136) starts WiFiManager AP with NO deleteConfig/resetSettings/nvs-erase; reset_configuration NOT bound to any CYD long-press path |
| 7 | CR-02: nm_display_wake non-blocking (no delay in loop/touch/button path) | ✓ VERIFIED | nm_display_wake (nm_display.cpp:136-143) only arms s_bl_ramping flag; ramp pumped step-by-step in nm_display_tick (152-165); no delay() in wake/loop path |
| 8 | VSPI free + AUTOADVANCE off + mining.cpp untouched + build green | ✓ VERIFIED | No VSPI bind in active touch path; NM_UI_AUTOADVANCE default 0 (nm_uimanager.cpp:34); mining.cpp not in any 04 commit; pio build SUCCESS |

**Score:** 8/8 truths verified (build + code-structure layer)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/ui_nm/nm_touch.{h,cpp}` | SoftSPI bit-bang XPT2046 + raw-Z + calibration | ✓ VERIFIED | 176 lines; full bit-bang impl, no hardware SPI |
| `src/ui_nm/nm_display.{h,cpp}` | runtime wake/dim/set_brightness, non-blocking | ✓ VERIFIED | wake/dim/set_brightness on shared ledc ch0; CR-02 non-blocking ramp |
| `src/ui_nm/nm_uimanager.{h,cpp}` | on_touch + on_button_* nav, idle-dim, autoadvance off | ✓ VERIFIED | enter_config_portal (WR-01), wake-consume, idle tick, AUTOADVANCE 0 |
| `src/NerdMinerV2.ino.cpp` | button re-point + touch poll + nm_touch_init | ✓ VERIFIED | CYD-guarded button block + nm_poll_touch_gesture + nm_touch_init + log_raw call |
| `platformio.ini [env:ESP32-2432S028R]` | TOUCH_ENABLE + NM_TOUCH_SOFTSPI | ✓ VERIFIED | (build links nm_touch; legacy VSPI handler gated dormant) |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| ino.cpp loop() | nm_uimanager_on_touch | nm_poll_touch_gesture → classify | ✓ WIRED | ino.cpp:381 polls; :348/:359 route LONG/SHORT |
| nm_uimanager long-press | enter_config_portal() | NM_TOUCH_LONG → on_touch | ✓ WIRED | nm_uimanager.cpp:155 (NON-destructive, WR-01) |
| nm_uimanager wake | nm_display_wake() | dim tap consumed | ✓ WIRED | nm_uimanager.cpp:130 |
| ino.cpp CR-01 gate | nm_touch_log_raw() | first N press edges | ✓ WIRED | ino.cpp:338 (was dead code in pre-fix docs) |
| boot button | nm_ui callbacks | OneButton attach* (CYD guard) | ✓ WIRED | ino.cpp:106-108 |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| CYD env compiles + links with touch+nav | `pio run -e ESP32-2432S028R` | [SUCCESS] 108s | ✓ PASS |
| nm_touch_log_raw has a call site (CR-01) | `grep nm_touch_log_raw src/` | ino.cpp:338 | ✓ PASS |
| No reset_configuration on CYD long-press | grep CYD button/touch paths | only #else/two-button branches | ✓ PASS |
| enter_config_portal non-destructive | inspect wManager.cpp:136-146 | startConfigPortal, no wipe | ✓ PASS |
| No delay() in nm_display_wake | inspect nm_display.cpp:136-143 | flag-armed, no delay | ✓ PASS |
| mining.cpp untouched | `git diff` 04 commits | not present | ✓ PASS |

### Build Gate

```
RAM:   35.3% (used 115784 bytes from 327680 bytes)
Flash: 87.4% (used 2748445 bytes from 3145728 bytes)
ESP32-2432S028R  SUCCESS  00:01:48
```

### Requirements Coverage

| Requirement | Source Plan | Status | Evidence |
|-------------|-------------|--------|----------|
| TOUCH-01 | 04-01 | ✓ SATISFIED | SoftSPI bit-bang + raw-Z diag (called) + rotation-1 cal |
| TOUCH-02 | 04-02 | ✓ SATISFIED | short/long/wake gestures + debounce/cooldown |
| TOUCH-03 | 04-02 | ✓ SATISFIED | boot button double/long/single mapped to nm_ui |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (none) | — | No debt markers (TBD/FIXME/XXX) in 04 files; no dead-code stubs (calibration now exercised) | — | All review Criticals + WR-01 closed in HEAD |

### Human Verification Required

On-device confirmation is Phase 7 by design. `/dev/ttyUSB0` is no longer the blocker;
the device has since been backed up, flashed, verified, and serial-observed. The four
remaining items in frontmatter `human_verification` are runtime-only checks:
tap→next, 10s-hold→non-destructive portal, dim-tap→wake, and boot-button mapping.

The raw touch serial diagnosis is no longer generic pending UAT. It ran in Phase 7 and
consistently reported no raw press/contact. The remaining action is physical
XPT2046/FPC/PENIRQ probing or touch-variant re-identification.

### Gaps Summary

No gaps. All eight must-haves verified at the build + code-structure layer. The two
code-review Criticals (CR-01 dead raw-Z diagnosis, CR-02 blocking wake delay) and the
safety Warning WR-01 (destructive long-press) are confirmed FIXED in HEAD — the
codebase supersedes the stale SUMMARY/VALIDATION narrative. Status is `human_needed`
rather than `passed` because the goal's user-observable behaviors (actual tap
navigation, wake, config entry, boot-button behavior) still require hardware runtime
confirmation; the touch path is currently diagnosed as no-contact below the UI layer.

---

_Verified: 2026-06-18T02:10:00Z_
_Verifier: Claude (gsd-verifier)_
