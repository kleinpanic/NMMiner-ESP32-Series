---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 01
subsystem: display
tags: [lvgl, tft_espi, cyd, esp32-2432s028r, black-screen-fix, palette, reverse-engineering]
requires:
  - "firmware/custom/NerdMiner-CYD-Custom recovered/fonts/recovered_fonts.h + recovered/nmminer_palette.h (Phase 2)"
  - "TFT_eSPI tft instance in src/drivers/displays/esp23_2432s028r.cpp"
provides:
  - "LVGL v8 render surface for the CYD env (lv_conf.h + nm_display bridge)"
  - "nm_display_init() / nm_display_tick() — LVGL-over-TFT_eSPI bridge + boot brightness ramp"
  - "resolved Clock price up/down RGB565 colors in nmminer_palette.h"
affects:
  - "all Phase 3 wave-2/3 screen modules (they render onto this LVGL surface)"
tech-stack:
  added:
    - "lvgl/lvgl@^8.4.0 (CYD env lib_deps)"
  patterns:
    - "LVGL registration structure mirrored from in-fork wt32DisplayDriver.cpp; flush body rewritten for TFT_eSPI"
    - "env-local lv_conf.h selected via -D LV_CONF_PATH (no -I lib inheritance)"
    - "boot backlight ramp 0->target over ~300ms as the black-screen fix"
key-files:
  created:
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/lv_conf.h"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp"
  modified:
    - "firmware/custom/NerdMiner-CYD-Custom/platformio.ini"
    - "firmware/custom/NerdMiner-CYD-Custom/recovered/nmminer_palette.h"
decisions:
  - "0xf01d was disasm noise (retw.n opcode at 0x400f24a8), not a color; spurious magenta removed"
  - "price-delta recolor branch absent from dumped Clock corpus -> PRICE_DOWN = canonical red 0xf800"
  - "LV_MEM_CUSTOM 0 + LV_MEM_SIZE 48KB (bounded) instead of hansolov2's custom malloc, for the no-PSRAM DRAM budget"
  - "LV_COLOR_16_SWAP 0 because the TFT_eSPI flush swaps via pushColors(...,true)"
metrics:
  duration: "~5 min"
  tasks_completed: 3
  files_created: 3
  files_modified: 2
  completed: 2026-06-18
---

# Phase 3 Plan 01: LVGL Display Surface + Black-Screen Fix + Price Colors Summary

LVGL v8 now links into the CYD `ESP32-2432S028R` env behind an env-local `lv_conf.h`, an LVGL-over-TFT_eSPI display bridge renders to the shared `tft` instance and ramps the backlight 0->target on boot (the concrete D-35 black-screen fix), and the Clock BTC price up/down colors are resolved in the palette (the Phase-2 magenta was disassembly noise, not a color).

## What Was Built

### Task 1 — Link LVGL into the CYD env + env-local lv_conf.h (`854ea27`)
- `platformio.ini [env:ESP32-2432S028R]`: added `lvgl/lvgl@^8.4.0` to `lib_deps`; added `-D LV_LVGL_H_INCLUDE_SIMPLE` and `-D LV_CONF_PATH="${PROJECT_DIR}/src/ui_nm/lv_conf.h"` to `build_flags`. `HANSOLOminerv2` stays in `lib_ignore` (D-32).
- `src/ui_nm/lv_conf.h`: derived from `lib/hansolov2/lv_conf.h`. `LV_COLOR_DEPTH 16`; Montserrat 12/14/16/22/24/26 enabled (recovered sizes); `LV_TICK_CUSTOM 1` via `millis()`; `LV_USE_LOG 0`.
- No-PSRAM budget (M2): `LV_MEM_CUSTOM 0` + `LV_MEM_SIZE 48KB`; no `BOARD_HAS_PSRAM`. `LV_COLOR_16_SWAP 0` (flush handles the swap).

### Task 2 — LVGL-over-TFT_eSPI bridge + boot brightness ramp (`faad2b2`)
- `src/ui_nm/nm_display.{h,cpp}`: `nm_display_init()` does `tft.init()/setRotation(1)/setSwapBytes(true)`, `lv_init()`, two 320x40 partial draw buffers, `lv_disp_drv_register`.
- `flush_cb` uses the TFT_eSPI API (`startWrite`/`setAddrWindow`/`pushColors((uint16_t*)&color_p->full, w*h, true)`/`endWrite`/`lv_disp_flush_ready`) on the `extern TFT_eSPI tft` — **not** LovyanGFX, **no** second instance.
- D-35 fix: `ledcSetup(0,5000,8)`/`ledcAttachPin(TFT_BL,0)`, then ramp duty 0->`Settings.Brightness` (fallback 200) over ~300ms (20 steps x 15ms), painting the first frame as it lights up.
- File guarded `#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)`. `nm_display_tick()` calls `lv_timer_handler()`.

### Task 3 — Resolve Clock price up/down colors (`2d6c8e3`)
- Disassembly of `irom.asm` at `0x400f24a8` shows bytes `f0 1d` = the `retw.n` instruction (tail of layout fn `FUN_400f24ac`), not a color. The Phase-2 tentative `0xf01d`/`#F700EF` was the return opcode misread at the code/literal-pool boundary — **confirmed spurious and removed**.
- `FUN_400f24ac` is the label-layout fn (no up/down constant branch). The live price-delta recolor is absent from the dumped Clock vtable corpus, so the two exact RGB888 constants are unrecoverable from available decompiled output.
- Per Task 3 fallback: `NMMINER_COLOR_PRICE_DOWN_565 = 0xf800` (`#FF0000`), `NMMINER_COLOR_PRICE_UP_565 = 0x86c6` (`#84DB31`). Evidence + IROM address recorded in-header; tentative markers gone. Only numeric color facts + addresses committed (RE-04).

## Key Links Established
- `platformio.ini [env:ESP32-2432S028R] lib_deps` -> `lvgl/lvgl@^8.4.0` (verified `lvgl/lvgl@^8`).
- `src/ui_nm/nm_display.cpp` -> `TFT_eSPI tft` via `tft.pushColors`/`setAddrWindow` in `flush_cb` (verified `lv_disp_drv_register` + `tft.pushColors`).

## Verification
- All three task grep gates passed.
- TFT_eSPI flush API verified against the vendored header `.pio/libdeps/ESP32-2432S028R/TFT_eSPI/TFT_eSPI.h`: `pushColors(uint16_t*, uint32_t, bool=true)` and `setAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h)` match the bridge usage exactly. `setRotation`/`setSwapBytes`/`startWrite`/`endWrite` confirmed present.
- LVGL v8 registration symbols (`lv_init`, `lv_disp_draw_buf_init`, `lv_disp_drv_init/_register`, `lv_disp_flush_ready`, `lv_timer_handler`, `lv_color_t.full`) match the in-fork wt32 driver against lvgl@^8.4.0.
- Full `pio run -e ESP32-2432S028R` was NOT run: PlatformIO is not installed in this environment, and lvgl is not yet vendored in libdeps (it is pulled by the new lib_deps entry at build time). Per the plan, the full build SUCCESS gate is **plan 03-06** (later wave); this plan's requirement (no compile error introduced in `nm_display.*`) is met via the focused static API verification above.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Disabled LV_COLOR_16_SWAP to avoid double byte-swap**
- **Found during:** Task 1
- **Issue:** hansolov2's lv_conf.h sets `LV_COLOR_16_SWAP 1`, but the CYD flush already byte-swaps via `tft.pushColors(..., true)` (and `setSwapBytes(true)`). Inheriting `=1` would double-swap and produce wrong colors on the CYD panel.
- **Fix:** Set `LV_COLOR_16_SWAP 0` in the env-local lv_conf.h, documented inline.
- **Files modified:** `src/ui_nm/lv_conf.h`
- **Commit:** `854ea27`

**2. [Rule 2 - Missing critical functionality] Bounded LVGL heap for the no-PSRAM budget**
- **Found during:** Task 1
- **Issue:** hansolov2's lv_conf uses `LV_MEM_CUSTOM 1` (system malloc, unbounded). On the no-PSRAM CYD, LVGL allocations would compete unbounded with WiFi/mining/monitor tasks in internal DRAM — an OOM/black-screen risk the plan's M2 constraint exists to prevent.
- **Fix:** `LV_MEM_CUSTOM 0` + hard-capped `LV_MEM_SIZE 48KB` (within the plan's <=48KB budget), with the budget math documented in the header.
- **Files modified:** `src/ui_nm/lv_conf.h`
- **Commit:** `854ea27`

## Threat Surface
No new threat surface beyond the plan's `<threat_model>`. T-03-01 (info disclosure) honored: only numeric color constants + IROM addresses committed to `nmminer_palette.h`; no firmware bytes. T-03-SC (lvgl supply chain): lvgl@^8.4.0 is the same already-vetted version the wt32 env uses — no new package, no legitimacy checkpoint required.

## Notes for Next Plans
- Wave-2/3 screen modules call `nm_display_init()` once at boot, then `nm_display_tick()` from the UI loop; do not touch `src/mining.cpp`/stratum.
- The price up/down colors are a documented fallback (canonical red + clean green). If a later RE pass dumps the live price-delta handler, the two exact RGB888 constants can replace the fallback; the evidence block in `nmminer_palette.h` records why the current values were chosen.
- Parent gitlink bump for `firmware/custom/NerdMiner-CYD-Custom` is left to the orchestrator (the three task commits live in the nested fork repo).

## Self-Check: PASSED
- All 5 fork files + SUMMARY.md found on disk.
- All 3 nested-repo commits (854ea27, faad2b2, 2d6c8e3) confirmed in fork git log.
