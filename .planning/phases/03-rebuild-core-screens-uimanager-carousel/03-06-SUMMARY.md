---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 06
subsystem: ui_nm (CYD LVGL firmware integration)
tags: [lvgl, esp32-2432s028r, integration, build-gate, branding]
requires:
  - "03-01 nm_display LVGL bridge + lv_conf"
  - "03-02 nm_uimanager 6-page carousel + nm_ui.h contract"
  - "03-03 Loading + Config screens"
  - "03-04 Miner screen"
  - "03-05 Clock + SPrice/Weather stubs"
provides:
  - "GREEN pio run -e ESP32-2432S028R (the Phase 3 acceptance gate)"
  - "linked CYD firmware.bin with all 6 NMMiner screens + UIManager + LVGL"
  - "03-VALIDATION.md (Nyquist build + per-screen + identity checks)"
affects:
  - "Phase 4 (touch nav), Phase 5 (live data), Phase 7 (on-device flash)"
tech-stack:
  added: []
  patterns:
    - "LV_CONF_INCLUDE_SIMPLE + -I (not LV_CONF_PATH) to dodge ESP32-macro path expansion"
    - "build_src_filter +<../recovered/> to compile the open-upstream font sibling tree"
    - "single LVGL partial draw buffer on the no-PSRAM DRAM budget"
key-files:
  created:
    - .planning/phases/03-rebuild-core-screens-uimanager-carousel/03-VALIDATION.md
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_clock.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/lv_conf.h
    - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
decisions:
  - "Task 1 branding needed no code change: Wave-3 loading screen already carries kleinpanic + NMMiner + 'Make it better'; per plan, a confirming touch only — no gratuitous rework"
  - "LV_MEM_SIZE 48K->32K + single draw buffer to clear a 33K dram0_0_seg overflow without dropping any widgets"
metrics:
  duration_min: 18
  completed: 2026-06-18
  tasks: 2
  files_changed: 5
  fork_commit: a12b1ee
  ram_used_pct: 35.3
  flash_used_pct: 87.3
---

# Phase 3 Plan 6: CYD Firmware Link + Build Gate Summary

GREEN `pio run -e ESP32-2432S028R` — the recovered NMMiner look now links into a
real CYD firmware image (the black-display fix). RAM 35.3%, Flash 87.3%.

## What was done

- **Task 1 — Branding (D-37):** confirmed the Loading splash carries kleinpanic
  branding (accent label, orange palette) layered additively over the stock
  identity. `NMMiner` and `Make it better` remain intact (DATA-06). Wave 3 had
  already placed adequate branding, so this was a confirming check, not a rework.
- **Task 2 — Build gate + validation:** iterated the firmware build from FAILED
  to SUCCESS, fixing 4 integration defects (below), then authored
  `03-VALIDATION.md` with the build-success gate, per-screen widget greps, the
  6-page completeness check, identity-string check, and price-color resolution.

## Build result (the phase gate)

```
RAM:   [====      ]  35.3% (used 115744 bytes from 327680 bytes)
Flash: [========= ]  87.3% (used 2746781 bytes from 3145728 bytes)
========================= [SUCCESS] Took 39.28 seconds =========================
ESP32-2432S028R  SUCCESS
```

Artifacts: `firmware.bin` (2753072 B), `ESP32-2432S028R_factory.bin` (2822144 B).

## Deviations from Plan

These were the cross-module errors the plan explicitly anticipated (the Wave-1/2/3
modules were written without a compile check). All fixed inline; commit `a12b1ee`
in the fork repo. No widgets dropped to pass.

### Auto-fixed Issues

**1. [Rule 3 - Blocking] nm_display.cpp bad include path**
- **Issue:** `#include "displayDriver.h"` only resolves for same-dir files
  (wt32DisplayDriver.cpp); nm_display.cpp is in src/ui_nm/ → `fatal error:
  displayDriver.h: No such file or directory`.
- **Fix:** include via the src-relative path `drivers/displays/displayDriver.h`
  (src/ is on the include root, as the .ino/mining.cpp/wManager.cpp already do).

**2. [Rule 3 - Blocking] LV_CONF_PATH macro-expansion corrupted the path**
- **Issue:** lvgl's `lv_conf_internal.h` does `__LV_TO_STR(LV_CONF_PATH)`, which
  macro-expands the path token-by-token before stringizing. The Arduino-ESP32
  predefined `ESP32` macro rewrote `NMMiner-ESP32-Series` → `NMMiner-1-Series`,
  a nonexistent path → `fatal error: .../lv_conf.h: No such file or directory`.
- **Fix:** drop `LV_CONF_PATH`; use `-D LV_CONF_INCLUDE_SIMPLE` + `-I src/ui_nm`.
  Compiler `-I` paths are not token-expanded, so the path survives intact.

**3. [Rule 3 - Blocking] fa_nmminer_subset.c never compiled (link)**
- **Issue:** `recovered/fonts/fa_nmminer_subset.c` is a sibling of `src/`, not
  matched by the default `+<*>` src filter → `fa_nmminer_subset` would be an
  undefined reference at link.
- **Fix:** `build_src_filter = +<*> +<../recovered/>` so the open-upstream font
  subset compiles into the image.

**4. [Rule 1 - Bug] nm_screen_clock.cpp missing includes**
- **Issue:** `getLocalTime` (esp32-hal) and `snprintf` were undeclared.
- **Fix:** add `#include <Arduino.h>` + `#include <cstdio>`.

**5. [Rule 1 - Bug] DRAM (.dram0.bss) overflow by 33152 bytes (link)**
- **Issue:** two 320×40 lv_color_t draw buffers (51 KB) + the 48 KB LVGL pool
  overflowed `dram0_0_seg` on the no-PSRAM ESP32-D0WD.
- **Fix:** single LVGL partial draw buffer (`buf2 = NULL`, reclaims ~25 KB) +
  `LV_MEM_SIZE` 48K→32K (reclaims 16 KB). 32 KB is ample for the label-only
  screens (no canvas/image heap). No widgets removed. Final RAM 35.3%.

## Known Stubs

Carried forward, unchanged from Wave 3 (Phase 5 scope, D-33):
- `nm_screen_sprice.cpp` / `nm_screen_weather.cpp`: class-name title + "loading…"
  placeholder. Honest page stubs to keep the 6-page list complete (UI-02); full
  data + geometry are Phase 5 (Weather geometry is NOT recoverable per SPEC).
- Miner network-diff / pool-diff: seeded "--" (not in the per-tick mining_data
  struct); wired in the data phase.
- Clock price: seeded last-known value to exercise the green/red recolor branch;
  live feed is Phase 5 (DATA-01).

These do not block the plan goal (the goal is a GREEN build of the structural UI).

## Self-Check: PASSED
- `03-VALIDATION.md` — FOUND
- fork commit `a12b1ee` — FOUND (4 files changed)
- `firmware.bin` — FOUND (2753072 bytes)
- build SUCCESS — confirmed in `/tmp/nm_build_5.log`
