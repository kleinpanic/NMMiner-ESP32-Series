---
phase: 04-restore-interactive-touch-softspi
plan: 02
kind: validation
nyquist_compliant: true
build_gate: PASS
build_command: "cd firmware/custom/NerdMiner-CYD-Custom && /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R"
build_result: SUCCESS
ram_used_pct: 35.3
flash_used_pct: 87.4
fork_commit: 8432982
date: 2026-06-18
---

# Phase 4 Validation — Restore Interactive Touch (SoftSPI) (CYD)

This is the Phase 4 acceptance gate. On-device tap / long-press confirmation is **Phase 7**
(out of scope here). This phase validates that touch is restored at the BUS layer on
software SPI (D-42, freeing hardware VSPI for SD in Phase 6), wired into the Phase-3
`nm_uimanager` carousel, and that the firmware **compiles and links**, plus that a raw-Z
diagnosis path exists for the future on-device session (D-43).

## Gate A — Build SUCCESS (the phase gate) — PASS

```
cd firmware/custom/NerdMiner-CYD-Custom && \
  /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R
```

Result (full log `/tmp/nm04_build_final.log`):

```
RAM:   [====      ]  35.3% (used 115768 bytes from 327680 bytes)
Flash: [========= ]  87.4% (used 2747897 bytes from 3145728 bytes)
========================= [SUCCESS] Took 81.15 seconds =========================

Environment      Status    Duration
---------------  --------  ------------
ESP32-2432S028R  SUCCESS   00:01:21.146
```

Artifacts produced:
- `.pio/build/ESP32-2432S028R/firmware.bin`
- `.pio/build/ESP32-2432S028R/firmware.elf`

Flash headroom vs the 87.3% Phase-3 / Plan 04-01 baseline: **+0.1%** (2746781 → 2747897 bytes,
+1116 bytes for the wake/dim API + nav handlers + gesture classifier). No regression; still
~12.6% Flash and ~64.7% RAM free.

Fork commit at gate: `8432982` (`feat(04-02): re-point CYD boot button + poll SoftSPI touch into nm_ui`).

## Gate B — Wiring facts (grep) — PASS

Run against `firmware/custom/NerdMiner-CYD-Custom/`:

| Fact (D-45 / TOUCH-02/03) | Check | Result |
|---|---|---|
| SoftSPI bus (no VSPI bind in nm_touch) — D-42 | bit-bang `digitalRead(TOUCH_MISO)` present; no SPIClass instance; no `#include <xpt2046.h>` | PASS¹ |
| Touch enabled for CYD env — D-41 | `TOUCH_ENABLE` set under `[env:ESP32-2432S028R]` | PASS |
| Short touch → next page | `nm_uimanager_next_page` reached by `nm_uimanager_on_touch(NM_TOUCH_SHORT)` | PASS |
| Long-press → config mode | `reset_configuration` reached by `NM_TOUCH_LONG` / `on_button_config` | PASS |
| Tap → wake (re-ramp brightness) | `nm_display_wake` called when dimmed (event consumed) | PASS |
| Boot button → nm_ui (CYD) | `nm_uimanager_on_button_next/_config/_wake` attached in ino.cpp | PASS |
| Auto-advance OFF by default | `#ifndef NM_UI_AUTOADVANCE` → `#define NM_UI_AUTOADVANCE 0` | PASS |
| nm_touch is the sole ACTIVE CYD touch path | `nm_touch_init()` at setup + `nm_poll_touch_gesture()` in loop; legacy `touchHandler.isTouched()` is in the `#elif !NM_TOUCH_SOFTSPI` branch → NOT compiled for CYD | PASS |
| Shared long-press constant (no drift) | `NM_TOUCH_LONG_MS` in nm_uimanager.h; `setPressMs(NM_TOUCH_LONG_MS)` in ino.cpp | PASS |

¹ **Fact-1 grep note:** the template's `! grep -q 'SPIClass\|#include <xpt2046'` produces a
false negative because `nm_touch.cpp`'s header comment *names* `SPIClass` / `xpt2046.h` to
document what it deliberately does NOT use (lines 5-6, 13). Excluding comment text, the code
contains **zero** `SPIClass` instantiations and **zero** `xpt2046.h` includes; the bus is pure
bit-bang (`digitalRead(TOUCH_MISO)` at nm_touch.cpp:88). Corrected checks:
`grep -nE '^[^*/]*SPIClass[[:space:]]+[A-Za-z]'` → NONE; `grep -nE '^\s*#include.*xpt2046'` → NONE.

### Touch/button gesture → nm_ui map (D-45)

| Input | Source | Gesture | nm_ui action |
|---|---|---|---|
| Touch tap (release < 800ms) | `nm_poll_touch_gesture` → `NM_TOUCH_SHORT` | short | `nm_uimanager_next_page()` (persists LastPage) |
| Touch hold ≥ 10s | `nm_poll_touch_gesture` → `NM_TOUCH_LONG` (edge-fired while held) | long | `enter_config_portal() [shipped; was reset_configuration in pre-fix draft]` (config/AP mode) |
| Touch tap while dimmed | any gesture, `s_dimmed==true` | wake | `nm_display_wake()` only — consumed, no nav |
| Boot button double-click | OneButton `attachDoubleClick` → `on_button_next` | next | `nm_uimanager_next_page()` |
| Boot button long-press (10s) | OneButton `attachLongPressStart` + `setPressMs(NM_TOUCH_LONG_MS)` → `on_button_config` | config | `enter_config_portal() [shipped; was reset_configuration in pre-fix draft]` |
| Boot button single-click | OneButton `attachClick` → `on_button_wake` | wake | `nm_display_wake()` if dimmed |
| (idle) no input > 60s | `nm_uimanager_tick` idle check | dim | `nm_display_dim()` (minimal; full screensaver = Phase 5) |

The button long-press window and the touch long-press window are the SAME `NM_TOUCH_LONG_MS`
(10000) `#define` in nm_uimanager.h — no drift between the two config-entry paths (plan-check
LOW finding addressed).

### Dormant legacy HSPI touch object (plan-check MEDIUM — addressed)

`src/drivers/displays/esp23_2432s028r.cpp:30-31` instantiates `SPIClass hSPI(HSPI)` +
`TFT_eTouch<TFT_eSPI> touch(tft, ETOUCH_CS, 0xFF, hSPI)` and calls `touch.init()` /
`touch.setCalibration()` / `touch.getXY()`. This TU is compiled under the CYD env macro, so the
object **links into the binary**.

**Confirmed unreached for the CYD env at runtime:**
- (a) `initDisplay()` is **NOT** called on the CYD boot path. ino.cpp setup takes the
  `#if defined(ESP32_2432S028R)||defined(ESP32_2432S028_2USB)` branch which calls
  `nm_uimanager_init()` (+ `nm_touch_init()`); `initDisplay()` is in the `#else` branch only
  (ino.cpp:141, unreached for CYD).
- (b) The only reference to `esp32_2432S028R_Init` (which calls the HSPI-bound `touch.init()`)
  is the function-pointer entry in the driver table (`esp23_2432s028r.cpp:652`), invoked solely
  via `currentDisplayDriver->initDisplay()` inside `initDisplay()` (display.cpp:71). With
  `initDisplay()` unreached, `esp32_2432S028R_Init()` and every `touch.*` call inside it never
  execute → **HSPI is never bound at runtime → D-42 not violated.**

**Scope of the "single touch path" claim:** scoped to the **active runtime path** — the SoftSPI
`nm_touch` is the only touch bus actually driven at runtime for the CYD env. We do **NOT** assert
"no HSPI/VSPI touch binding exists anywhere in the binary": the dormant `esp23_2432s028r.cpp`
object is still linked. It was deliberately **not edited** (out of scope: mining/legacy path).

**Flagged for the SD phase (Phase 6):** any future caller that resurrects `esp32_2432S028R_Init()`
on the CYD env would double-bind HSPI (legacy touch) — reconcile before adding an SD path that
shares that init. (Threat register T-04-06, disposition: accept-with-flag.)

## Gate C — Raw-Z diagnosis path (D-43 gate evidence) — DOCUMENTED

The raw-Z bus diagnosis seam is `nm_touch_log_raw()` (declared `nm_touch.h:40`, implemented
`nm_touch.cpp:165`). It is the on-device evidence path; on-device tap confirmation itself is
explicitly **Phase 7** (gated on `/dev/ttyUSB0`).

**Serial line shape** (the exact format a Phase-7 session reads — `nm_touch.cpp:171`):

```
[nm_touch] z=%u rawX=%u rawY=%u -> px=%d py=%d pressed=%d
```

e.g. on a real press the operator expects something like:

```
[nm_touch] z=742 rawX=1980 rawY=2100 -> px=158 py=121 pressed=1
```

**Bus-before-calibration logic (PITFALLS #3):** if `z` and `rawX/rawY` **change** when the panel
is pressed vs. idle, the bit-bang bus is wired and clocking correctly → any remaining mis-touch is
a *calibration* problem (flip `NM_TOUCH_*_INVERT` / adjust `NM_TOUCH_RAW_*` ), **not** a wiring/bus
problem. If `z` stays near zero / `rawX/rawY` are stuck regardless of press, the bus itself is the
fault (check CLK25/MOSI32/MISO39/CS33 / soldering).

**Calling the seam (Phase 7):** `nm_touch_log_raw()` is a ready public function; the Phase-7
on-device session calls it from a debug build (e.g. a temporary `#ifdef NM_TOUCH_DEBUG` line in
`nm_poll_touch_gesture()` or a one-off serial command) to dump the raw stream. It is intentionally
**not** wired into the default loop in Plan 04-02 (no serial spam in the shipping nav path; the
function is the documented evidence hook, invoked on demand).

**Calibration constants chosen** (rotation 1, 320x240; from Plan 04-01, RNT/marcelrv CYD range):

```
NM_TOUCH_RAW_X_MIN = 200    NM_TOUCH_RAW_X_MAX = 3700   -> pixel X [0..319]
NM_TOUCH_RAW_Y_MIN = 240    NM_TOUCH_RAW_Y_MAX = 3800   -> pixel Y [0..239]
NM_TOUCH_X_INVERT = 0       NM_TOUCH_Y_INVERT = 0       NM_TOUCH_Z_THRESHOLD = 300
```

## Nyquist note

Every Plan 04-01/04-02 task carries an `<automated>` verify (grep sentinels + the
`pio run` build gate). No on-device test is required for this phase's gate; the runtime
raw-Z read is documented here (Gate C) as the Phase-7 on-device step.

## Verdict

| Gate | Result |
|---|---|
| A — Build SUCCESS | **PASS** (RAM 35.3%, Flash 87.4%, fork `8432982`) |
| B — Wiring facts | **PASS** (touch+button → nm_ui; autoadvance OFF; single active path; dormant HSPI confirmed unreached) |
| C — Raw-Z diagnosis path | **DOCUMENTED** (`nm_touch_log_raw` seam + serial shape; on-device read is Phase 7) |

Phase 4 acceptance gate **MET** (compile/link + wiring). On-device tap/long-press/wake
confirmation is Phase 7.
