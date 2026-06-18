---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 03
subsystem: ui
tags: [lvgl, screens, loading, config, cyd, esp32-2432s028r, spec-geometry, kleinpanic-branding]
requires:
  - "Plan 03-02: nm_screen_t contract + NM_PAGE_LOADING/NM_PAGE_CONFIG enum + UIManager extern refs"
  - "Plan 03-01: nm_display LVGL surface; recovered_fonts.h (Montserrat) + nmminer_palette.h (Phase 2)"
  - "LVGL v8 in the CYD env (linked at plan 03-06)"
provides:
  - "const nm_screen_t nm_screen_loading — Loading320x240 boot splash (6 SPEC widgets + NMMiner title + kleinpanic accent)"
  - "const nm_screen_t nm_screen_config — Config320x240 config-mode indicator (2 SPEC widgets)"
affects:
  - "Plan 03-06 (final link/build once all 6 descriptors exist — loading+config now defined)"
  - "Wave-2 UIManager page registry (its loading/config extern refs are now satisfied)"
tech-stack:
  added: []
  patterns:
    - "Per-screen module: static s_scr + dynamic-widget statics, build()/update()/destroy(), one const nm_screen_t descriptor"
    - "SPEC align enum -> LVGL LV_ALIGN_* mapping (1/3/4/6/7/9), recovered Montserrat by line-height, palette colors"
    - "fork CURRENT_VERSION used for version labels (not stock hardcoded v2.0.02)"
    - "whole file env-guarded #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)"
key-files:
  created:
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp"
  modified: []
decisions:
  - "kleinpanic branding rendered as an orange accent text label (not the kp_logo bitmap): kp_logo.h is a raw uint16_t RGB565 array (TFT_eSPI-style), not an lv_img_dsc_t — a label is the faithful, low-risk additive (D-37) choice the plan explicitly permits"
  - "Config countdown label uses LVGL default auto-size instead of a manual lv_txt_get_width width call: default auto-size already fits the SPEC 300s sizing reference, and the manual-measure API could not be verified against fetched LVGL source this phase (LVGL is fetched at 03-06)"
  - "Version labels use the fork's CURRENT_VERSION (V1.8.3) macro, not the stock v2.0.02 string, per D-37/DATA-06 (keep NMMiner identity, stamp the fork version)"
metrics:
  duration: "~5 min"
  tasks_completed: 2
  files_created: 2
  files_modified: 0
  completed: 2026-06-18
---

# Phase 3 Plan 03: Loading + Config Screens (LVGL from SPEC) Summary

Two of the six wave-3 screens are now defined natively in LVGL v8 from the recovered SPEC geometry: `nm_screen_loading` reproduces the stock Loading320x240 boot splash (status, progress bar, percent, IP, fw version, "Make it better" tagline) with the "NMMiner" title and a kleinpanic accent layered additively (D-37), and `nm_screen_config` reproduces the Config320x240 config-mode indicator (fw-version top-right + status/countdown bottom-right) with the captive-portal strings deliberately absent as widgets. Both fill the Wave-2 `nm_screen_t` contract and satisfy the UIManager's `nm_screen_loading` / `nm_screen_config` extern references for the `NM_PAGE_LOADING` / `NM_PAGE_CONFIG` slots.

## What Was Built

### Task 1 — Loading320x240 + kleinpanic branding (`3529c0c`)
- `src/ui_nm/nm_screen_loading.cpp`: `const nm_screen_t nm_screen_loading = { "Loading320x240", build, update, destroy };`
- The 6 stock SPEC widgets at exact geometry (320x240 panel, screen default width 320):
  1. **status** label — `LEFT_MID (4)` x0 y0, width 320, montserrat_16, white, seed `""` (runtime-filled).
  2. **progress bar** (`lv_bar`) — `BOTTOM_MID (9)` x0 y-20, width `320*0.9`, range 0–100, value 0, white indicator (`LV_PART_INDICATOR`).
  3. **percent** label — `BOTTOM_LEFT (7)` x=`320*0.02` y-10, montserrat_16, white, `"0%"`.
  4. **IP** label — `BOTTOM_MID (9)` x0 y60, montserrat_22, green `lv_color_hex(0x07E0)` (SPEC 0x7e0), scroll long-mode, seed `"192.168.124.222"`.
  5. **fw version** label — `TOP_RIGHT (3)` x0 y2, montserrat_16, white, `CURRENT_VERSION`.
  6. **tagline** label — `BOTTOM_MID (9)` x0 y20, montserrat_22, white, `"Make it better"` (DATA-06 identity preserved).
- Additive branding (D-37): **"NMMiner"** title (SPEC row 7, model-identity string preserved) at `TOP_MID` y30 montserrat_24 white, and a **"kleinpanic"** orange accent (`NMMINER_COLOR_ORANGE_HEX`) at `TOP_MID` y60 montserrat_14. Stock strings untouched.
- `update()` minimal no-op this phase (no trivially-available monitor state); `destroy()` `lv_obj_del` + null the statics. Whole file env-guarded.
- **Widget creates: 8** (`lv_label_create` x7 + `lv_bar_create` x1).

### Task 2 — Config320x240 config-mode indicator (`280c0d4`)
- `src/ui_nm/nm_screen_config.cpp`: `const nm_screen_t nm_screen_config = { "Config320x240", build, update, destroy };`
- Zero-pad / zero-border / zero-radius container (SPEC FUN_400f2b58 lines 18-26), then exactly the 2 SPEC widgets:
  1. **fw version** label — `TOP_RIGHT (3)` x-45 y10, montserrat_22 (SPEC font 0x3f4f9fbc, lh22), near-white tint `lv_color_hex(0xFFEBE6)` (SPEC 0xffffebe6 low word), `CURRENT_VERSION`. SPEC width arg 0x27d1 is a suspect packed value → label auto-size used.
  2. **status/countdown** label — `BOTTOM_RIGHT (6)` x-50 y-25, montserrat_22, white, seed `"300s"` (the SPEC sizing reference; stock seeds `" "` then runtime-updates).
- No `nmap-2.4g` / `192.168.4.1` widgets (SPEC: HTTP/HTML layer, never `lv_label_set_text`).
- `update()` minimal no-op (no config-mode timeout state available this phase); `destroy()` frees the tree. Env-guarded.
- **Widget creates: 2** (`lv_label_create` x2).

## Descriptor symbols defined
- `const nm_screen_t nm_screen_loading` — name `"Loading320x240"`, 8 widget creates.
- `const nm_screen_t nm_screen_config` — name `"Config320x240"`, 2 widget creates.

Both match the Wave-2 `nm_screen_t` signature `{ const char*, lv_obj_t*(*build)(void), void(*update)(void), void(*destroy)(void) }` and the declared extern names (`nm_screen_loading` / `nm_screen_config`) for `NM_PAGE_LOADING` / `NM_PAGE_CONFIG`.

## Key Links Established
- `nm_screen_loading.cpp` → `recovered_fonts.h` / `nmminer_palette.h` via `lv_obj_set_style_text_font(&lv_font_montserrat_*)` + `lv_color_hex(NMMINER_COLOR_ORANGE_HEX)` / `lv_color_hex(0x07E0)` (verified `lv_font_montserrat` + palette macro present).
- `nm_screen_config.cpp` → `nm_ui.h` `nm_screen_t` via `const nm_screen_t nm_screen_config = {...}` (verified `nm_screen_t nm_screen_config` present).
- Both → `../version.h` `CURRENT_VERSION` for the fw-version labels.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] kleinpanic branding as accent label, not kp_logo bitmap**
- **Found during:** Task 1
- **Issue:** `src/media/kp_logo.h` is a raw `const uint16_t kpLogo[54400] PROGMEM` RGB565 array (TFT_eSPI `pushImage` format), not an `lv_img_dsc_t`. Using it in LVGL would require wrapping it in an `lv_img_dsc_t` with a color-format descriptor — a fragile, unverified step inside a structural-compile-only phase.
- **Fix:** Rendered the kleinpanic branding as an orange accent text label (the plan explicitly permits "a small label OR the kp_logo"). Additive (D-37): the stock "NMMiner" / "Make it better" strings are untouched.
- **Files modified:** `src/ui_nm/nm_screen_loading.cpp`
- **Commit:** `3529c0c`

**2. [Rule 3 - Blocking] Config countdown width via label auto-size, not lv_txt_get_width**
- **Found during:** Task 2
- **Issue:** SPEC pre-sizes the countdown label to the text width of `"300s"`. The natural LVGL v8 call is `lv_txt_get_width(...)`, but LVGL is not fetched into libdeps until plan 03-06, so the exact signature could not be verified against source this phase (research-defaults: don't assert an unverified API shape).
- **Fix:** Dropped the explicit width call. An LVGL label's default auto-size already fits its text exactly, which IS the SPEC "300s" sizing reference — behavior-equivalent without an unverified API call.
- **Files modified:** `src/ui_nm/nm_screen_config.cpp`
- **Commit:** `280c0d4`

## Threat Surface
Plan `<threat_model>` dispositions honored:
- **T-03-06 (Info disclosure, recovered geometry in source):** only numeric coordinates, font names, and color constants (facts) are in source; no closed firmware bytes committed. SPEC backups remain gitignored.
- **T-03-07 (Tampering, runtime label text):** accepted — IP/version/countdown text is device-local state rendered to screen via `lv_label_set_text` with bounded literals this phase; no injection sink.

No new threat surface beyond the plan's register.

## Known Stubs
None that block the plan goal. `update()` is intentionally minimal on both screens (status/percent/bar/IP on Loading, countdown on Config are seeded in `build()` with SPEC default values and will be driven from monitor/config-mode state in a later phase) — this is the documented phase boundary, not a hidden stub. Seeds are the stock SPEC values, so the screens render meaningful content as-is.

## Notes for Next Plans
- The Wave-2 UIManager's `nm_screen_loading` / `nm_screen_config` extern references are now satisfied; the remaining unresolved descriptors are `nm_screen_miner` / `nm_screen_clock` / `nm_screen_sprice` / `nm_screen_weather` (plans 03-04/05).
- Full `pio run` link is plan 03-06 (the structural-compile boundary); per-screen widget-presence is re-checked in 03-06 VALIDATION.
- Later wiring: replace the build() seeds with live state in `update()` — Loading status/percent/bar/IP from the connection monitor, Config countdown from the config-mode timeout.
- Parent gitlink bump for `firmware/custom/NerdMiner-CYD-Custom` is the orchestrator's job; the 2 task commits (`3529c0c`, `280c0d4`) live in the nested fork repo.

## Self-Check: PASSED
- Both fork files (nm_screen_loading.cpp, nm_screen_config.cpp) present on disk; SUMMARY.md created in parent .planning.
- Both nested-repo commits (3529c0c, 280c0d4) confirmed in fork git log.
- Widget counts verified: Loading 8 creates (6 SPEC + NMMiner title + kleinpanic accent), Config 2 creates.
- mining.cpp / mining.h / STATE / ROADMAP confirmed untouched (git status clean for those paths).
- Grep gates for both tasks return OK.
