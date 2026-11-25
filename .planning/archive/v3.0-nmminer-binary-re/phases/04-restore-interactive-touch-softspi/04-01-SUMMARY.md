---
phase: 04-restore-interactive-touch-softspi
plan: 01
subsystem: touch-bus
tags: [touch, xpt2046, softspi, bit-bang, cyd, calibration]
requires:
  - "Phase 3 nm_ui/nm_uimanager (CYD UIManager carousel) — touch will drive it in 04-02"
  - "[env:ESP32-2432S028R] touch pin flags (ETOUCH_CS/TOUCH_CLK/TOUCH_MISO/TOUCH_MOSI)"
provides:
  - "nm_touch SoftSPI XPT2046 bus reader (raw Z/X/Y + calibrated pixel + serial diagnosis)"
  - "TOUCH_ENABLE + NM_TOUCH_SOFTSPI enabled for the CYD env"
  - "VSPI left free for SD (Phase 6)"
affects:
  - "firmware/custom/NerdMiner-CYD-Custom (nested fork repo)"
tech-stack:
  added: []
  patterns:
    - "Direct GPIO bit-bang SPI (MODE0, MSB-first) — no hardware SPI peripheral bound"
    - "Env-guarded TU (#if defined(ESP32_2432S028R)||defined(ESP32_2432S028_2USB))"
    - "NM_TOUCH_SOFTSPI gate re-points legacy TOUCH_ENABLE seams away from the VSPI handler"
key-files:
  created:
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
    - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.h
decisions:
  - "D-42: XPT2046 read by hand-written bit-bang on CLK25/MOSI32/MISO39/CS33; NO SPIClass/VSPI/<xpt2046.h>"
  - "liangyingy arduino_xpt2046_library rejected for the bus layer (hardware-SPI-only constructor)"
  - "Legacy VSPI TouchHandler gated out for the CYD env (NM_TOUCH_SOFTSPI), kept dormant not deleted"
  - "Polled pressure (no PENIRQ): GPIO36 input-only/no pull on CYD (D-41)"
metrics:
  duration: "~6 min"
  completed: 2026-06-18
  tasks: 2
  files: 6
---

# Phase 4 Plan 01: SoftSPI XPT2046 Touch Driver (nm_touch) Summary

Bit-bang XPT2046 touch reader on the CYD touch pins (CLK25/MOSI32/MISO39/CS33) with raw-Z/X/Y diagnosis and rotation-1 320x240 calibration; the hardware VSPI peripheral is deliberately untouched so SD can claim it in Phase 6. Touch is enabled for the `ESP32-2432S028R` env and the legacy VSPI-bound `TouchHandler` is gated dormant. CYD env builds SUCCESS (RAM 35.3%, Flash 87.3% — no regression vs the Phase-3 baseline).

## What Was Built

| Task | Name | Status | Commit | Files |
|------|------|--------|--------|-------|
| 1 | Bit-bang XPT2046 SoftSPI driver (nm_touch) + raw-Z diagnosis | Done | `7ca0ed5` | src/ui_nm/nm_touch.{h,cpp} |
| 2 | Enable touch for CYD env + build gate | Done | `d2be48c` | platformio.ini, NerdMinerV2.ino.cpp, TouchHandler.{cpp,h} |

Commits are in the **nested fork repo** `firmware/custom/NerdMiner-CYD-Custom`. The parent gitlink bump is the orchestrator's responsibility.

### nm_touch API (extern "C")
- `nm_touch_init()` — pinMode CLK/MOSI/CS OUTPUT, MISO INPUT, CS idle HIGH, CLK idle LOW (MODE0).
- `nm_touch_pressed()` — raw-Z proxy above threshold (polled, no IRQ).
- `nm_touch_raw_z()` — Z1(0xB0)-Z2(0xC0) pressure proxy, clamped >= 0.
- `nm_touch_read_raw(rx,ry)` — median-of-5 12-bit X(0x90)/Y(0xD0) reads (jitter rejection).
- `nm_touch_read_point(px,py)` — calibrated pixel point, rotation 1, clamped to [0,319]/[0,239].
- `nm_touch_log_raw()` — Serial.printf raw Z/X/Y + mapped pixel (D-43 gate evidence; called from a 04-02 debug seam).

## Bit-Bang Confirmation (D-42)

`nm_touch.cpp` drives the bus exclusively via `pinMode`/`digitalWrite`/`digitalRead`:
- `xpt_write_byte` clocks 8 bits MSB-first (MOSI set, CLK rising/falling manually).
- `xpt_xfer` issues a command byte then clocks 16 bits back, `>> 3` to the 12-bit result.
- **No `SPIClass`, no `SPI`/`VSPI` instance, no `#include <xpt2046.h>`.** Grep gate `NM_TOUCH_SOFTSPI_OK` confirms `digitalRead(TOUCH_MISO` present AND no VSPI/xpt2046 binding. VSPI stays free for SD (Phase 6).

## Calibration Constants (D-44 — rotation 1, 320x240)

```
NM_TOUCH_RAW_X_MIN = 200    NM_TOUCH_RAW_X_MAX = 3700   -> pixel X [0..319]
NM_TOUCH_RAW_Y_MIN = 240    NM_TOUCH_RAW_Y_MAX = 3800   -> pixel Y [0..239]
NM_TOUCH_W = 320            NM_TOUCH_H = 240
NM_TOUCH_X_INVERT = 0       NM_TOUCH_Y_INVERT = 0
NM_TOUCH_Z_THRESHOLD = 300  (press cutoff on the raw-Z proxy)
```
CYD ESP32-2432S028R starting range (RNT/marcelrv examples). Axis orientation vs `setRotation(1)` is verified on-device in Phase 7; if X/Y appear swapped/inverted there, flip the INVERT macros or swap the RAW pairs (documented in `nm_touch.cpp`). The fork's t_hmi handler thresholds (bottom/top split @ TouchHandler.cpp:41) were deliberately NOT copied — different rotation/axis mapping.

## Reuse-vs-Rewrite Decision

**Considered:** the fork's `TouchHandler` + `arduino_xpt2046_library` (liangyingy).
**Rejected for the bus layer** because the library is hardware-SPI-only: `XPT2046(SPIClass&, cs, irq)` binds a hardware SPI peripheral and `TouchHandler::begin` calls `spi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI)` — which would remap and grab VSPI, violating D-42 (SD needs VSPI in Phase 6). There is no bit-bang mode in that library.
**Outcome:** `nm_touch` is a new ~175-line direct bit-bang bus reader. The gesture/debounce SHAPE of `TouchHandler` is mirrored in Plan 04-02, not here — nm_touch is a pure bus reader, not a parallel gesture reimplementation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Gated legacy TouchHandler compilation for the CYD SoftSPI path**
- **Found during:** Task 2 (first build attempt, `/tmp/nm04_build1.log`).
- **Issue:** Enabling `-DTOUCH_ENABLE=1` made `TouchHandler.{cpp,h}` (gated by bare `#ifdef TOUCH_ENABLE`) compile for the CYD env, pulling in `#include <xpt2046.h>` (the liangyingy lib is not installed for this env) → `fatal error: xpt2046.h: No such file or directory`. The two `touchHandler` ino.cpp references would also be undefined externs.
- **Fix:** The plan (Task 2 note) explicitly authorized minimal ino.cpp gating if the build broke on undefined `touchHandler`, with the rule "the FIX is NOT to add a VSPI-bound TouchHandler." Gated 3 ino.cpp seams (include @25, extern @45, isTouched @274) and the `TouchHandler.{cpp,h}` compile guards to `#if defined(TOUCH_ENABLE) && !defined(NM_TOUCH_SOFTSPI)`. The CYD env (which defines NM_TOUCH_SOFTSPI) now compiles nm_touch and leaves the legacy VSPI handler dormant; other TOUCH_ENABLE boards (e.g. t_hmi) are unaffected since they do not define NM_TOUCH_SOFTSPI.
- **Files modified:** src/NerdMinerV2.ino.cpp, src/TouchHandler.cpp, src/TouchHandler.h
- **Commit:** `d2be48c`

This was anticipated/sanctioned by the plan's Task 2 note (proper-scope ino.cpp re-pointing), so it is logged for fidelity rather than as unplanned scope creep.

### ino.cpp gating needed here? — YES (minimal, sanctioned)
The plan asked to record whether ino.cpp gating was required. It was: without it the build fails on `xpt2046.h` / undefined `touchHandler`. Done minimally (guard conditions only, no logic changes); full UIManager re-pointing of these seams remains Plan 04-02's job.

## Build Result (Gate A)

```
Environment      Status    Duration
ESP32-2432S028R  SUCCESS   00:01:28
RAM:   35.3% (115760 / 327680 bytes)
Flash: 87.3% (2747005 / 3145728 bytes)
```
Phase-3 baseline was 87.3% flash — **headroom unchanged**. The dormant legacy TouchHandler no longer compiles for this env, so nm_touch's addition is net-neutral on flash. Build logs: `/tmp/nm04_build1.log` (failed, pre-fix), `/tmp/nm04_build2.log` (SUCCESS).

## Constraints Honored

- mining.cpp, Phase-3 ui_nm screens, STATE.md, ROADMAP.md: **NOT touched.**
- Nested-repo commits via `git -C` in the fork; SUMMARY in parent.
- VSPI peripheral: **not bound** (D-42).
- The dormant HSPI/VSPI legacy touch object in esp23_2432s028r.cpp: **not edited** (confirmed unreached via the NM_TOUCH_SOFTSPI gate).

## Known Stubs

None. nm_touch is a complete bus reader with a real bit-bang implementation and calibration; the runtime read seam (calling `nm_touch_log_raw`/`nm_touch_read_point` from the loop) is Plan 04-02 / on-device confirmation is Phase 7, by design (D-43/D-45).

## Self-Check: PASSED

- FOUND: src/ui_nm/nm_touch.h, src/ui_nm/nm_touch.cpp (in fork)
- FOUND: commits 7ca0ed5, d2be48c (fork git log)
- Build sentinel NM04_01_BUILD_OK printed; grep sentinel NM_TOUCH_SOFTSPI_OK printed.
