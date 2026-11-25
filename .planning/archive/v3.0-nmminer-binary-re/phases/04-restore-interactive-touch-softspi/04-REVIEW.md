---
phase: 04-restore-interactive-touch-softspi
reviewed: 2026-06-18T00:00:00Z
depth: standard
files_reviewed: 8
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
findings:
  critical: 2
  warning: 5
  info: 4
  total: 11
status: issues_found
---

# Phase 4: Code Review Report

**Reviewed:** 2026-06-18
**Depth:** standard
**Files Reviewed:** 10 (8 in named diff + nm_touch.cpp/.h which the diff added under src/ui_nm/)
**Status:** issues_found

## Summary

The bit-bang XPT2046 driver (`nm_touch.cpp`) is well-formed: pin directions correct, CS framing correct, MODE0 MSB-first clocking correct, 12-bit assembly correct (`>> 3 & 0x0FFF`), no SPIClass/VSPI/`<xpt2046.h>` reference — D-42 is honored, and the touch pins (CLK25/MOSI32/MISO39/CS33) are fully disjoint from the display HSPI pins (SCLK14/MOSI13/CS15), so there is no bus contention. Env-guarding is consistent across `platformio.ini`, the `.ino.cpp`, `TouchHandler.{cpp,h}`, and the `nm_*` modules; the legacy VSPI `TouchHandler` is correctly gated out by `NM_TOUCH_SOFTSPI` and is never instantiated.

Two Critical issues dominate. (1) The phase gate itself — D-43's raw-Z diagnosis — cannot be produced: `nm_touch_log_raw()` is never called from anywhere, and neither is the entire calibration path (`nm_touch_read_point`/`read_raw`/`raw_z`). The whole D-44 calibration apparatus is dead code. (2) `nm_display_wake()` does a blocking `delay(15)*8 = 120 ms` and is invoked synchronously from the loop touch poll and OneButton callbacks, directly violating the repeatedly-asserted "non-blocking, no delay() (T-04-04)" contract and stalling `wifiManagerProcess()` / button ticks on every wake.

A separate concern worth the reviewer's attention: `reset_configuration()` is not a "config-mode entry" — it `deleteConfig()` + `resetSettings()` + `ESP.restart()`, i.e. a destructive factory wipe of WiFi creds and wallet. It is now reachable from a 10 s touch hold in addition to the boot-button long-press.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: Phase gate (raw-Z diagnosis) and the entire calibration path are dead code

**File:** `src/ui_nm/nm_touch.cpp:106-173`, `src/NerdMinerV2.ino.cpp:278-313`
**Issue:** D-43 makes the raw-Z / raw-X/Y serial diagnosis "the gate evidence" for this phase (on-device tap is Phase 7). `nm_touch_log_raw()` is defined but **never called** from any translation unit (verified by grep across `src/`). The gesture path uses only `nm_touch_pressed()`; it never calls `nm_touch_read_point()`, `nm_touch_read_raw()`, or `nm_touch_raw_z()` publicly. Consequently:
- The phase's stated gate (a documented raw-Z reading proving the bus works) is not obtainable at runtime — nothing prints it.
- `nm_touch_read_point()` and all D-44 calibration (`map_clamp`, `NM_TOUCH_RAW_*`, `NM_TOUCH_W/H`, `median5`, the invert macros) are unreachable dead code. Nav is position-independent (tap = next page), so coordinates are never consumed.

This means the phase deliverable "documented raw-Z diagnosis" has no code path that produces it, and ~half of `nm_touch.cpp` ships unexercised. A compiler with `-Werror=unused-function` could also fail the build on `median5`/`map_clamp`/`nm_touch_log_raw` if any guard changes.
**Fix:** Call the diagnostic on boot (or behind a serial command) so the gate produces evidence, and either wire `read_point` into a future calibrated path or drop the unused calibration code until Phase 7 needs it:
```cpp
// in setup(), after nm_touch_init():
nm_touch_init();
for (int i = 0; i < 200; ++i) {       // ~Phase-4 raw-Z gate evidence (D-43)
    nm_touch_log_raw();
    delay(50);
}
```
(Or gate behind a `-D NM_TOUCH_DIAG` flag so it is opt-in.) If calibration stays unused this phase, prefer deleting `read_point`/`map_clamp`/`median5`/RAW constants over shipping dead code.

### CR-02: nm_display_wake() blocks loop() for ~120 ms, violating the T-04-04 non-blocking contract

**File:** `src/ui_nm/nm_display.cpp:118-128`, called via `src/ui_nm/nm_uimanager.cpp:126-135,162,174`
**Issue:** `nm_display_wake()` runs `for (i=1..8) { ledcWrite(...); delay(15); }` — a synchronous **120 ms busy block**. It is invoked from `nm_input_wake_if_dimmed()`, which runs inside:
- `nm_poll_touch_gesture()` → `nm_uimanager_on_touch()` (called every loop iteration), and
- the OneButton `attachClick` callback `nm_uimanager_on_button_wake()` (fired from `button1.tick()` in loop).

The same loop also calls `wifiManagerProcess()` and `button1.tick()`, both explicitly documented as needing to avoid `delay()`. The UIManager file repeatedly asserts "Non-blocking ... no delay()/busy-wait (T-04-04)" (lines 215, 226, comments at 121) — `nm_display_wake()` breaks that guarantee. Mining itself is on a separate FreeRTOS task so hashrate is unaffected, but WiFi servicing and subsequent button/touch edges are stalled for 120 ms on every wake, and OneButton can miss the release edge of the very gesture that triggered the wake.
**Fix:** Make wake non-blocking — either a single immediate `ledcWrite(kBlChannel, s_bl_target)` (no ramp on wake), or an `lv_timer`/millis-stepped ramp pumped from `nm_display_tick()`:
```cpp
void nm_display_wake(void) {
    ledcWrite(kBlChannel, s_bl_target);   // immediate; no delay() in the loop path
}
```
If a ramp is desired, drive it from `nm_display_tick()` with a millis-based step counter instead of `delay()`.

## Warnings

### WR-01: Long-press touch fires a destructive factory wipe, not a "config mode" entry

**File:** `src/ui_nm/nm_uimanager.cpp:152-153,168-169`; sink at `src/wManager.cpp:123-131`
**Issue:** D-45 describes long-press (10 s) as "enter config mode (the fork's WiFiManager/config path)." The bound function `reset_configuration()` actually does `nvMem.deleteConfig(); resetStat(); wm.resetSettings(); ESP.restart();` — it **erases saved WiFi credentials and miner/wallet settings** and reboots into the unprovisioned AP. This is the fork's pre-existing boot-button semantics, so it is not newly introduced, but Phase 4 now also reaches it from a 10 s on-screen hold (`NM_TOUCH_LONG`). A user holding the screen to "open settings" will silently lose their wallet/pool config. The naming throughout the new code ("config-mode entry", "force config") understates that this is a destructive reset.
**Fix:** Confirm intent. If "config mode" should be non-destructive (start the WiFiManager portal without wiping NVS), bind a portal-only entry instead of `reset_configuration()`. If a wipe is genuinely intended, rename comments to "factory reset" so the destructive semantics are explicit, and consider requiring the 10 s hold + an on-screen confirm before wiping.

### WR-02: Boot-button single-click never wakes when the screen is not dimmed — and double-click is the only way to navigate, which delays every press

**File:** `src/NerdMinerV2.ino.cpp:104-107`; `src/ui_nm/nm_uimanager.cpp:160-175`
**Issue:** With OneButton, `attachClick` fires only after the double-click timeout elapses (so the lib can rule out a double-click), and `attachClick` is bound to `wake`, `attachDoubleClick` to `next`. Net behavior: a single press on a lit screen does nothing useful (wake is a no-op when not dimmed — `nm_input_wake_if_dimmed()` returns false and `on_button_wake` ignores it), and the only way to change pages with the button is a double-click, which always incurs the click-timeout latency. This matches D-45's literal mapping but yields a button whose single-click is functionally inert on a lit screen. Worth confirming this is the intended UX vs. single-click = next.
**Fix:** Either accept and document (single-click intentionally wake-only), or remap single-click to `next` and drop the wake/next split (wake-on-any-input is already handled because `on_button_next` calls `nm_input_wake_if_dimmed()` first and returns).

### WR-03: Redundant NVS write of the unchanged last-page on every boot

**File:** `src/ui_nm/nm_uimanager.cpp:57,104-107,200`
**Issue:** `s_persisted` initializes to `0xFFFFFFFF`. On boot, `nm_uimanager_init()` calls `nm_uimanager_goto_page(last)` with `persist=true`. Since `(uint32_t)index != s_persisted` is `last != 0xFFFFFFFF` (always true on first nav), `putUInt(NM_UI_KEY, last)` runs even when `last` already equals the value just read from NVS. Result: one redundant flash write of the identical value at every boot. The WR-01 dedup guard the code documents (lines 67-68) does not cover the boot restore.
**Fix:** Seed `s_persisted` with the restored value so the first nav is a no-op write:
```cpp
uint32_t last = s_prefs_ready ? s_prefs.getUInt(NM_UI_KEY, NM_PAGE_LOADING) : NM_PAGE_LOADING;
if (last >= NM_PAGE_COUNT) last = NM_PAGE_LOADING;
s_persisted = last;                 // restore == already-persisted; suppress redundant write
nm_uimanager_goto_page((uint8_t)last);
```

### WR-04: No inter-press dead-time / debounce on the touch classifier — a held finger spanning a long-press boundary, or contact bounce, can mis-fire

**File:** `src/NerdMinerV2.ino.cpp:285-313`; `src/ui_nm/nm_touch.cpp:116-121`
**Issue:** Press state is derived purely from `nm_touch_raw_z() > NM_TOUCH_Z_THRESHOLD` polled at ~20 Hz (loop `vTaskDelay(50ms)`). There is no hysteresis (single threshold for both press and release) and no minimum-press filter. A press that hovers near the Z threshold will rapidly toggle `down`, generating repeated press/release edges → each release edge fires a `NM_TOUCH_SHORT` → rapid unwanted page advances. The old `TouchHandler::debounce()` enforced a 2 s gate; the new classifier has none. Bit-bang reads also have no settle/acquisition delay after asserting CS (`xpt_xfer` clocks immediately), which on a marginal panel raises jitter near threshold.
**Fix:** Add press/release hysteresis (e.g. press above `Z_THRESHOLD`, release below `Z_THRESHOLD*0.6`) and/or a minimum stable-press count before accepting a press edge. Optionally a short post-action lockout so one physical tap cannot register twice.

### WR-05: `nm_disp_flush` casts a partial-buffer pointer and assumes 16-bit color without guarding the pixel format

**File:** `src/ui_nm/nm_display.cpp:59-70`
**Issue:** `tft.pushColors((uint16_t *)&color_p->full, w*h, true)` assumes `lv_color_t` is exactly 16-bit (`LV_COLOR_DEPTH 16`) and that `.full` is the first/only member. This is true for the current `lv_conf.h`, but the cast is silent — if `LV_COLOR_DEPTH` ever changes the flush will push garbage with no compile-time guard. Pre-existing from Plan 03-01, surfaced because wake/dim now depend on this flush still rendering while dimmed.
**Fix:** Add a static assertion near the flush so a depth change fails loudly:
```cpp
static_assert(LV_COLOR_DEPTH == 16, "nm_disp_flush assumes RGB565");
```

## Info

### IN-01: Unused private helpers / unreachable calibration symbols

**File:** `src/ui_nm/nm_touch.cpp:124-163`
**Issue:** Given CR-01, `median5`, `map_clamp`, `nm_touch_read_raw`, `nm_touch_read_point`, and the `NM_TOUCH_RAW_*`/`W`/`H`/invert constants are all currently unreachable. Carrying them is defensible only if Phase 7 will wire them; otherwise they are dead weight and a `-Wunused` risk.
**Fix:** Either exercise them (CR-01 fix) or move the calibration block behind a `// Phase 7` `#if 0`/flag until needed.

### IN-02: `TOUCH_IRQ`/GPIO36 defined but unused

**File:** `platformio.ini:995`; `src/ui_nm/nm_touch.cpp:116-121`
**Issue:** `-DTOUCH_IRQ=36` is set but the driver is intentionally polled (D-41, GPIO36 input-only no pull). The macro is now unreferenced for the active path. Harmless, but a reader may assume IRQ is wired.
**Fix:** Leave a one-line note at the pin def that IRQ is intentionally unused (polled pressure), or drop the flag.

### IN-03: Defensive `#ifndef` pin defaults in nm_touch.cpp can mask a missing env flag

**File:** `src/ui_nm/nm_touch.cpp:25-36`
**Issue:** The fallback `#define TOUCH_CLK 25` etc. silently substitute pins if the env ever fails to pass them. For a board-specific driver this trades a loud build error for a silent wrong-pin runtime fault. The CYD env does define all four (platformio.ini 991-994), so the fallbacks never trigger today.
**Fix:** Consider `#error` instead of silent defaults when `defined(ESP32_2432S028R)` but a pin macro is missing, so a mis-edited env fails at compile time.

### IN-04: Header comment claims init "start[s] the auto-advance timer" but it is OFF by default

**File:** `src/ui_nm/nm_uimanager.h:51-53`; `src/ui_nm/nm_uimanager.cpp:206-209`
**Issue:** The doc comment on `nm_uimanager_init()` says it starts the auto-advance timer; with `NM_UI_AUTOADVANCE` defaulting to 0 (D-45), no timer is created. Stale comment.
**Fix:** Update the comment to "and (only if NM_UI_AUTOADVANCE) start the auto-advance timer."

---

_Reviewed: 2026-06-18_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
