---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 02
subsystem: ui
tags: [lvgl, uimanager, carousel, nvs, preferences, cyd, esp32-2432s028r, boot-routing, auto-advance]
requires:
  - "Plan 03-01: nm_display_init() / nm_display_tick() LVGL-over-TFT_eSPI surface + lv_conf.h"
  - "LVGL v8 (lvgl/lvgl@^8.4.0) in CYD env lib_deps"
provides:
  - "nm_screen_t builder contract + NM_PAGE_* 6-page enum (the wave-3 screen interface)"
  - "nm_uimanager: 6-page carousel, goto_page/next_page lazy LVGL lifecycle, LastPage NVS persistence, debug auto-advance"
  - "CYD boot routed to the UIManager (replaces TFT_eSPI cyclic carousel for this env)"
affects:
  - "Plans 03-03/04/05 (each defines one nm_screen_t descriptor against this contract)"
  - "Plan 03-06 (final link/build once all 6 descriptors exist)"
  - "Phase 4 touch nav (will drive nm_uimanager_goto_page; disables NM_UI_AUTOADVANCE)"
tech-stack:
  added:
    - "Preferences (NVS) dedicated namespace nm_ui for LastPage"
  patterns:
    - "nm_screen_t fn-pointer contract {name, build, update, destroy} for lazy per-page LVGL lifecycle"
    - "page registry as static const nm_screen_t* const pages[NM_PAGE_COUNT]"
    - "UI pump from Arduino loop() (non-mining path) — avoids touching mining.cpp runMonitor"
    - "env-guarded whole file + boot blocks via #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)"
key-files:
  created:
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp"
  modified:
    - "firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp"
decisions:
  - "Fork's nvMemory wrapper is SPIFFS+JSON (not Preferences); opened a dedicated nm_ui Preferences NVS namespace per the plan's documented fallback rather than retrofitting SPIFFS for a single uint"
  - "UI pump wired into loop() (50ms, non-blocking) not runMonitor — mining.cpp is off-limits and loop() is already the non-mining drive point (T-03-04)"
  - "auto-advance default ON (NM_UI_AUTOADVANCE=1, 6s) so 6 screens cycle without touch this phase; Phase 4 clears the flag"
  - "page index clamped in BOTH init (after NVS read) and goto_page (T-03-03 defense in depth)"
metrics:
  duration: "~6 min"
  tasks_completed: 3
  files_created: 3
  files_modified: 1
  completed: 2026-06-18
---

# Phase 3 Plan 02: UIManager Carousel + Screen Contract + Boot Routing Summary

A 6-page LVGL UIManager now owns the CYD's screen identity: `nm_screen_t` is the build/update/destroy contract every wave-3 screen fills, the manager lazily creates/loads/destroys page trees, persists the active page to a dedicated `nm_ui` NVS namespace (resume on last page after reboot), and auto-advances on a 6s debug timer so all screens are verifiable without touch. Boot for the `ESP32-2432S028R`/`_2USB` env routes to `nm_uimanager_init()` instead of the TFT_eSPI cyclic carousel, with the UI pump driven from the non-mining `loop()` so mining/stratum are untouched.

## What Was Built

### Task 1 — Screen-builder contract + page enum (`29efa00`)
- `src/ui_nm/nm_ui.h`: the contract `typedef struct { const char* name; lv_obj_t* (*build)(void); void (*update)(void); void (*destroy)(void); } nm_screen_t;` — lazy lifecycle (build creates+returns the page's `lv_obj_create(NULL)` screen, update refreshes dynamic text, destroy frees the tree for the no-PSRAM DRAM budget; update/destroy may be NULL).
- `enum { NM_PAGE_LOADING=0, NM_PAGE_MINER, NM_PAGE_CLOCK, NM_PAGE_SPRICE, NM_PAGE_WEATHER, NM_PAGE_CONFIG, NM_PAGE_COUNT };` — the 6 stock pages (UI-02 structural identity).
- `extern const nm_screen_t nm_screen_{loading,miner,clock,sprice,weather,config};` so each wave-3 module defines exactly one. Header is contract-only.

### Task 2 — UIManager carousel + LastPage NVS + auto-advance (`267c739`)
- `src/ui_nm/nm_uimanager.{h,cpp}`: `static const nm_screen_t* const pages[NM_PAGE_COUNT]` = the 6 descriptors in carousel order.
- `nm_uimanager_init()`: `nm_display_init()` (Plan 01), `s_prefs.begin("nm_ui", false)`, `getUInt("lastpage", NM_PAGE_LOADING)` (clamped), `goto_page(last)` — resume on last page; then `lv_timer_create` for auto-advance.
- `nm_uimanager_goto_page(i)`: clamps `i` to `[0,NM_PAGE_COUNT)`, calls prev page's `destroy()`, builds the target, `lv_scr_load()`, stores `current=i`, `putUInt("lastpage", i)`.
- `nm_uimanager_next_page()`: `goto_page((current+1) % NM_PAGE_COUNT)`.
- Auto-advance: `lv_timer_create(nm_autoadvance_cb, 6000, NULL)` behind `NM_UI_AUTOADVANCE` (default 1 this phase).
- `nm_uimanager_tick()`: `nm_display_tick()` + current page `update()`; non-blocking, no delay().
- Whole file env-guarded to the CYD boards.

### Task 3 — Route CYD boot to the UIManager (`73a780e`)
- `src/NerdMinerV2.ino.cpp`: env-guarded `#include "ui_nm/nm_uimanager.h"`.
- `setup()`: under `#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)` calls `nm_uimanager_init()` and skips `initDisplay()/drawLoadingScreen()`; `#else` keeps the original path for other boards. hansolov2 `ui_init` is not called for this env.
- `loop()`: env-guarded `nm_uimanager_tick()` — the non-mining pump (50ms loop), so the stratum/miner tasks are never starved (T-03-04).

## Contract / Wiring Reference

- **nm_screen_t signature:** `struct { const char* name; lv_obj_t* (*build)(void); void (*update)(void); void (*destroy)(void); }`
- **LastPage NVS:** `Preferences` namespace `"nm_ui"`, key `"lastpage"` (uint). Read+clamped in `nm_uimanager_init`, written in `nm_uimanager_goto_page` on every page change → reboot resumes on the stored page.
- **Boot routing:** `setup()` → `nm_uimanager_init()` (CYD env only) → `nm_display_init` + ramp + `goto_page(restored LastPage)`. Old TFT_eSPI carousel path is `#else`-gated out for this env.
- **Auto-advance:** `lv_timer_create(..., 6000, NULL)` → `next_page()`; gated by `NM_UI_AUTOADVANCE` (default ON). All 6 screens cycle without touch; Phase 4 clears the flag and drives `goto_page` from touch.

## Key Links Established
- `setup()` → `nm_uimanager_init()` (verified `nm_uimanager_init` in boot file, gated on the CYD env).
- `nm_uimanager.cpp` → NVS via `s_prefs.putUInt`/`getUInt` (verified `putUInt`).
- UI pump → `nm_display_tick()` inside `nm_uimanager_tick()` (verified), pumped from `loop()`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] NVS via dedicated Preferences namespace instead of nvMemory reuse**
- **Found during:** Task 2
- **Issue:** The plan's `<read_first>` suggested reusing the fork's `nvMemory` Preferences pattern, but `src/drivers/storage/nvMemory.{h,cpp}` is SPIFFS+ArduinoJson (`#define NVMEM_SPIFFS`, `SPIFFS.begin`, JSON config blob) — it has no Preferences/NVS uint API to reuse for a single LastPage index.
- **Fix:** Opened the plan's documented fallback — a dedicated `Preferences` namespace `"nm_ui"` (`putUInt`/`getUInt`) — exactly as the plan permitted ("otherwise open a dedicated 'nm_ui' Preferences namespace").
- **Files modified:** `src/ui_nm/nm_uimanager.cpp`
- **Commit:** `267c739`

**2. [Rule 3 - Blocking] UI pump wired into loop() not runMonitor**
- **Found during:** Task 3
- **Issue:** The old CYD carousel is animated via `animateCurrentScreen()` called from `runMonitor` in `src/mining.cpp:1279`, but mining.cpp is explicitly off-limits this plan.
- **Fix:** Pumped `nm_uimanager_tick()` from the Arduino `loop()` (already a non-blocking, non-mining path running every 50ms) — equivalent non-starving drive point without touching mining.cpp. `animateCurrentScreen` still routes through the per-board DisplayDriver (unchanged), but the LVGL screen is what is loaded for the CYD env.
- **Files modified:** `src/NerdMinerV2.ino.cpp`
- **Commit:** `73a780e`

## Threat Surface
Plan `<threat_model>` mitigations applied:
- **T-03-03 (Tampering, LastPage NVS):** index clamped to `[0,NM_PAGE_COUNT)` in both `nm_uimanager_init` (after the NVS read) and `nm_uimanager_goto_page` before any `pages[]` deref — defense in depth.
- **T-03-04 (DoS, UI pump vs mining):** `nm_uimanager_tick` is `lv_timer_handler`-only (no `delay()`/busy-wait), pumped from the non-mining `loop()`; mining/stratum task priorities and stacks unchanged.
- **T-03-05 (EoP):** accepted — no network surface or credentials added.

No new threat surface beyond the plan's register.

## Known Stubs
None introduced by this plan. The 6 `nm_screen_*` descriptors are intentionally `extern`-declared only (the contract); their definitions are wave-3 work (plans 03-03/04/05) and the final link is plan 03-06 — this is the planned structural-compile boundary, not a stub. The UIManager will not link in isolation until the descriptors exist; full `pio run` SUCCESS is the 03-06 gate.

## Notes for Next Plans
- Wave-3 screens: each defines one `const nm_screen_t nm_screen_<name> = { "...", build_fn, update_fn, destroy_fn };` in its own module. `build` must return a fresh `lv_obj_create(NULL)` screen; `destroy` should `lv_obj_del` it.
- Phase 4 touch: drive `nm_uimanager_goto_page` / `nm_uimanager_next_page` from touch and build with `-U NM_UI_AUTOADVANCE` (or `-D NM_UI_AUTOADVANCE=0`) to disable the debug timer.
- Parent gitlink bump for `firmware/custom/NerdMiner-CYD-Custom` is the orchestrator's job; the 3 task commits (`29efa00`, `267c739`, `73a780e`) live in the nested fork repo.

## Self-Check: PASSED
- All 4 fork files (nm_ui.h, nm_uimanager.h, nm_uimanager.cpp, NerdMinerV2.ino.cpp) present on disk; SUMMARY.md created in parent .planning.
- All 3 nested-repo commits (29efa00, 267c739, 73a780e) confirmed in fork git log.
- mining.cpp / mining.h confirmed unmodified; mining/stratum task creation intact.
