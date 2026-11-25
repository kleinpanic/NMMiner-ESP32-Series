---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 05
subsystem: ui
tags: [lvgl, clock, sprice, weather, ntp, rgb565, palette, cyd, esp32-2432s028r, nm_screen_t, stub]
requires:
  - "Plan 03-02: nm_screen_t contract + NM_PAGE_CLOCK/SPRICE/WEATHER enum + UIManager page registry"
  - "Plan 03-01: recovered/nmminer_palette.h (PRICE_UP/DOWN, CLOCK_GRAY/LTGRAY, ORANGE, GREY) + recovered_fonts.h"
  - "SPEC.md §Clock320x240 9-widget geometry + align enum + 500ms cadence"
provides:
  - "nm_screen_clock: full Clock320x240 — 9 SPEC widgets, NTP time (12/24h + AM/PM + date format), green-up/red-down BTC price recolor (D-36)"
  - "nm_screen_sprice: SPrice320x240 page stub (title + loading placeholder) — completes the 6-page list"
  - "nm_screen_weather: Weather320x240 page stub (title + loading placeholder) — no fabricated geometry"
affects:
  - "Plan 03-06 (final link/build: all 6 nm_screen_t descriptors now exist)"
  - "Phase 5 (DATA-01 live BTC price feed into Clock + full SPrice; DATA-02 Weather data screen)"
tech-stack:
  added: []
  patterns:
    - "getLocalTime()/strftime read-only time render (no new NTP client) into LVGL labels"
    - "price recolor by prev-value delta -> lv_obj_set_style_text_color with palette PRICE_UP/DOWN"
    - "honest page stub: valid nm_screen_t descriptor with title + placeholder, update=NULL"
    - "build-time format prefs (NM_CLOCK_24H / NM_CLOCK_DATE_YMD) as Settings-field stand-in"
key-files:
  created:
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_clock.cpp"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_sprice.cpp"
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_weather.cpp"
  modified: []
decisions:
  - "Clock 12-hour + AM/PM default (per stock '00:00'/'AM' DROM seeds); 24h + YMD date are -D build flags since the fork Settings struct exposes no clock-format field"
  - "Time rendered via getLocalTime()/strftime (ESP32 system clock, read-only) instead of reusing monitor.cpp NTPClient internals — satisfies 'no new NTP client' without coupling the screen to monitor.cpp"
  - "BTC price seeded (last-known 68000) with prev-value delta recolor to prove the green-up/red-down branch; live feed is Phase 5 (DATA-01) per plan"
  - "Weather stub renders NO geometry (SPEC marks the static frame NOT AVAILABLE; reconstructing would violate UI-06)"
metrics:
  duration: "~4 min"
  tasks_completed: 2
  files_created: 3
  files_modified: 0
  completed: 2026-06-18
---

# Phase 3 Plan 05: Clock Screen (Full) + SPrice/Weather Page Stubs Summary

The Clock320x240 screen is rebuilt natively on the `nm_screen_t` contract: 9 widgets placed at exact SPEC recovered coordinates, NTP time rendered via `getLocalTime()`/`strftime` (12/24h with AM/PM and selectable date format), and the BTC price label recolored green-up / red-down using the resolved D-36 palette constants. SPrice320x240 and Weather320x240 are registered as honest page stubs (class-name title + "loading…" placeholder) so the UIManager's 6-page list is now complete and links — their market/weather data is deferred to Phase 5.

## What Was Built

### Task 1 — Clock320x240 full screen (`df78bd9`)
`src/ui_nm/nm_screen_clock.cpp` — `const nm_screen_t nm_screen_clock = { "Clock320x240", clock_build, clock_update, clock_destroy };` (NM_PAGE_CLOCK).

- **9 SPEC widgets** at exact recovered coords (SPEC.md §Clock320x240, align enum 1/2/3/4/6/9):
  1. `+0x04` header line 1 — TOP_MID (0,0), CLOCK_GRAY
  2. `+0x08` header line 2 — TOP_LEFT (0,22), CLOCK_GRAY
  3. `+0x0c` date — BOTTOM_LEFT, CLOCK_LTGRAY
  4. `+0x14` AM/PM — CENTER (80,-31), seed "AM"
  5. `+0x18` time field 1 — TOP_LEFT (125,170), montserrat_26, seed "00:00"
  6. `+0x1c` time field 2 — TOP_LEFT (67,170)
  7. `+0x20` accent — TOP_MID, ORANGE (#EF7D31)
  8. `+0x24` "kH/s" unit — TOP_RIGHT (0,20), CLOCK_LTGRAY
  9. `+0x30` BTC price — BOTTOM_RIGHT (-2,0), seeded CLOCK_LTGRAY, recolored by direction
- **Time:** `clock_update_time()` calls `getLocalTime(&tm, 0)` (non-blocking; falls back to `localtime(time(NULL))`), formats `%I:%M`+`%p` (12h, default) or `%H:%M` (24h, AM/PM hidden) via `strftime`, and writes the date as `%d/%m/%Y` (default, matches the fork `getDate()` order) or `%Y-%m-%d`. Keeps the "00:00" seed until system time is set. NO new NTP client created.
- **Price recolor (D-36):** `clock_update_price()` renders `$<value>` and compares to `s_prev_price`; `>` → `lv_obj_set_style_text_color(..., NMMINER_COLOR_PRICE_UP_HEX)` green #84DB31, `<` → `NMMINER_COLOR_PRICE_DOWN_HEX` red #FF0000. Seeded value proves the branch; live feed is Phase 5.
- `clock_destroy()` `lv_obj_del`s the screen and nulls all handles. Whole file env-guarded to `ESP32_2432S028R`/`_2USB`.

### Task 2 — SPrice + Weather page stubs (`92e0b24`)
- `src/ui_nm/nm_screen_sprice.cpp` — `const nm_screen_t nm_screen_sprice = { "SPrice320x240", sprice_build, NULL, sprice_destroy };` (NM_PAGE_SPRICE). build() = "SPrice" title (palette GREY) + centered "loading…" placeholder; update NULL; destroy frees.
- `src/ui_nm/nm_screen_weather.cpp` — `const nm_screen_t nm_screen_weather = { "Weather320x240", weather_build, NULL, weather_destroy };` (NM_PAGE_WEATHER). Same shape, "Weather" title. NO SPEC geometry reconstructed (frame NOT AVAILABLE → UI-06).
- Both env-guarded; both are valid `nm_screen_t` descriptors → the UIManager `pages[]` registry now resolves all 6 entries.

## Contract / Wiring Reference
- All 3 descriptors satisfy `nm_screen_t` from `nm_ui.h` (`{name, build, update, destroy}`); the wave-3 screen set (`nm_screen_clock/sprice/weather`) is now defined, joining `loading/miner/config` for the 03-06 link.
- Clock time: ESP32 system clock (read-only). Clock price + SPrice/Weather data: Phase 5.
- Palette: `recovered/nmminer_palette.h` `_HEX` forms via `lv_color_hex(...)`; fonts: LVGL Montserrat 14/16/26 (all enabled in `src/ui_nm/lv_conf.h`).

## Key Links Established
- `nm_screen_clock.cpp` → `nmminer_palette.h` PRICE_UP/DOWN via `lv_obj_set_style_text_color` on the price label by direction (verified `NMMINER_COLOR_PRICE_(UP|DOWN)`).
- `nm_screen_clock.cpp` → NTP/system time via `getLocalTime`/`localtime`/`strftime` (verified).
- `nm_screen_sprice.cpp` / `nm_screen_weather.cpp` → UIManager `pages[]` as `nm_screen_sprice` / `nm_screen_weather` (verified descriptor symbols + class-name strings).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Time via getLocalTime() not the fork's NTPClient internals**
- **Found during:** Task 1
- **Issue:** The plan's `<read_first>` referenced "configTime/ntpServer", but the fork actually uses an `NTPClient timeClient` object in `src/monitor.cpp` (not `configTime`), exposing only `getTime()`/`getDate()` String helpers with no AM/PM or 12/24h support and no clean header API to call from the screen.
- **Fix:** Rendered time read-only via the ESP32 Arduino `getLocalTime(struct tm*)`/`strftime` (system clock) — satisfies the plan's hard constraint "do not add a new NTP client" and the verify gate (`getLocalTime|localtime|strftime`) without coupling the screen module to monitor.cpp internals. `NerdMinerV2.ino.cpp` already defines `ntpServer`; wiring the system clock from the NTP epoch is a Phase 4/5 follow-up if `getLocalTime` returns false at runtime (the screen keeps the "00:00" seed until then).
- **Files modified:** `src/ui_nm/nm_screen_clock.cpp`
- **Commit:** `df78bd9`

**2. [Rule 3 - Blocking] Clock format prefs as build flags (no Settings field)**
- **Found during:** Task 1
- **Issue:** The plan said "read whatever Settings field the fork exposes; if none exists, default 12h and note it." The fork `Settings` struct has no 12/24h or date-format field.
- **Fix:** Defaulted to 12-hour + AM/PM (matching the stock "00:00"/"AM" DROM seeds) and added `-D NM_CLOCK_24H` / `-D NM_CLOCK_DATE_YMD` build-flag overrides as the Settings-field stand-in (a Phase 4/5 wiring point). Noted in-file.
- **Files modified:** `src/ui_nm/nm_screen_clock.cpp`
- **Commit:** `df78bd9`

## Threat Surface
Plan `<threat_model>` mitigations applied:
- **T-03-11 (Info disclosure, Clock geometry):** only numeric coordinate/color/font facts are committed (from the gitignored SPEC); no closed bytes embedded (RE-04).
- **T-03-12 (Tampering, NTP):** accepted — no new network surface; time is read from the existing system clock, no second NTP client; price is a seeded constant (live network feed is Phase 5).
- **T-03-13 (Spoofing, stub screens):** accepted — SPrice/Weather render only static placeholders; no input.

No new threat surface beyond the register.

## Known Stubs
- **`nm_screen_sprice` / `nm_screen_weather`** — intentional page stubs (title + "loading…"; `update=NULL`). They are honest stubs per D-33: the full data screens are explicitly Phase 5 scope (DATA-01 SPrice market data, DATA-02 Weather). Documented in each file header. They are valid `nm_screen_t` descriptors so the 6-page list is structurally complete now; no fabricated geometry.
- **Clock BTC price value** — seeded last-known constant (`s_seed_price`) with a `s_prev_price` delta to exercise the green-up/red-down recolor branch. The live price feed is Phase 5 (DATA-01); the recolor logic and all 9 widgets are real this phase. Documented in-file.

These stubs do NOT block this plan's goal (Clock screen full + 6-page list complete); they are the planned Phase-3/Phase-5 boundary per D-33.

## Notes for Next Plans
- **03-06 (link/build):** all 6 `nm_screen_*` descriptors now exist; `pio run -e <CYD env>` is the structural-compile gate. Watch for `getLocalTime` declaration (Arduino core `esp32-hal-time.h`, pulled by `<Arduino.h>`; `<time.h>` is included for `struct tm`/`localtime`/`strftime`).
- **Phase 5:** replace `s_seed_price` with the live `bitcoin_price`/`getBTCprice()` source (DATA-01) and feed the SPrice/Weather stub `build()` with real data + geometry; wire `getLocalTime`'s system clock from the NTP epoch if not already set.
- Parent gitlink bump for `firmware/custom/NerdMiner-CYD-Custom` is the orchestrator's job; the 2 task commits (`df78bd9`, `92e0b24`) live in the nested fork repo.

## Self-Check: PASSED
- All 3 fork files present on disk; SUMMARY.md created in parent .planning.
- Both nested-repo commits (`df78bd9`, `92e0b24`) confirmed in fork git log.
- Both grep gates pass (GATE-1: nm_screen_clock + PRICE_UP/DOWN + getLocalTime/localtime/strftime; GATE-2: nm_screen_sprice + SPrice320x240 + nm_screen_weather + Weather320x240).
- mining.cpp / STATE / ROADMAP untouched by this plan's commits.
