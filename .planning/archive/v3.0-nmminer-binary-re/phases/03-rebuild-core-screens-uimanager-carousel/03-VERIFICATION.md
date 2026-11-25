---
phase: 03-rebuild-core-screens-uimanager-carousel
verified: 2026-06-18T01:30:00Z
status: gaps_found
status_detail: code_build_passed_hardware_failed
hardware_verified: false
hardware_status: failed_on_device_before_07_02_repair
superseded_by: ".planning/phases/07-on-device-validation/07-VERIFICATION.md"
score: "6/6 code/build must-haves verified; 0 physical UI truths verified here"
overrides_applied: 0
build_gate: PASS
build_command: "cd firmware/custom/NerdMiner-CYD-Custom && /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R"
build_result: SUCCESS
ram_used_pct: 35.3
flash_used_pct: 87.3
criticals_fixed:
  - id: CR-01
    status: fixed
    evidence: "display.cpp drawCurrentScreen/animateCurrentScreen are env-gated no-ops for CYD"
  - id: CR-02
    status: fixed
    evidence: "nm_uimanager goto_page builds+loads new screen before destroying old (load-then-destroy)"
  - id: mining-untouched
    status: confirmed
    evidence: "git -C diff HEAD -- src/mining.cpp is empty"
deferred:
  - truth: "AP nmap-2.4g + captive portal at 192.168.4.1 (SC4 second clause)"
    addressed_in: "Phase 5 / wManager"
    evidence: "Captive-portal SSID/IP are HTTP/wManager behavior, not Config-screen widgets (VALIDATION Gate B confirms 0 SSID widgets is correct). Live data wiring deferred per ROADMAP Plan 03-05 notes + Phase 5."
  - truth: "Miner network-diff / pool-diff / all-time fields populated with live values"
    addressed_in: "Phase 5"
    evidence: "Widgets exist (v_netdiff/v_pooldiff at SPEC coords); per-tick struct does not expose these, network/pool wiring is Phase 5 (VALIDATION 'Deferred to Phase 7/5')."
---

# Phase 3: Rebuild Core Screens + UIManager Carousel — Verification Report

**Phase Goal:** Booting the fork reproduces the stock NMMiner structural identity — the 6 named screens, the page carousel, and last-page persistence — driven by recovered evidence (SPEC.md geometry + Phase-2 fonts/palette), with kleinpanic branding layered on. Build SUCCESS is the gate.
**Verified:** 2026-06-18
**Status:** passed for code/build scope only; hardware-visible behavior later failed in Phase 7 and remains unverified
**Re-verification:** No — initial verification

> Current evidence boundary (2026-06-19): this report verifies compile/source structure only. It does not prove the CYD rendered these screens. Phase 7 later exposed on-device failures including boot-loop, clobbering, config crash, white-screen history, and touch no-contact. Use `07-CURRENT-FAILURE-STATE.md` and `07-VERIFICATION.md` for live hardware truth.

## Goal Achievement

### Build Gate (load-bearing)

```
cd firmware/custom/NerdMiner-CYD-Custom && \
  /home/broklein/.../.venv/bin/pio run -e ESP32-2432S028R
→ [SUCCESS] Took 102.42 seconds  (1 succeeded)
RAM:   35.3% (115760 / 327680 bytes)
Flash: 87.3% (2746797 / 3145728 bytes)
```

Verifier ran the build in its own process (exit code 0). Matches VALIDATION.md
(RAM 35.3 / Flash 87.3). **PASS.**

### Source-Structure Truths (UI-01 .. UI-06)

These truths were verified from source and build output, not from physical display observation.

| # | Truth (req) | Status | Evidence |
|---|-------------|--------|----------|
| UI-01 | Loading screen: NMMiner / "Make it better" / fw version / progress + kleinpanic accent | ✓ VERIFIED | nm_screen_loading.cpp: `lv_bar_create` (1), `lv_label_set_text(... "Make it better")`, `lv_label_set_text(ver, CURRENT_VERSION)`, `"NMMiner"` title, `"kleinpanic"` accent label |
| UI-02 | UIManager registers all 6 pages + LastPage NVS + resume-on-boot | ✓ VERIFIED | nm_ui.h enum LOADING/MINER/CLOCK/SPRICE/WEATHER/CONFIG + NM_PAGE_COUNT; nm_uimanager.cpp `pages[NM_PAGE_COUNT]` with all 6 descriptors; Preferences ns `"nm_ui"` key `"lastpage"`; init reads `getUInt` and calls `goto_page(last)` to resume |
| UI-03 | Miner screen full field set (~23 widgets) | ✓ VERIFIED | nm_screen_miner.cpp: 24 `mk_label` widgets — hr_big/hr_sub hashrate, v_shares, v_netdiff, v_pooldiff, v_bestdiff, v_status, v_temp, v_rssi, uptime, "H/s"; 5 FA glyphs (WIFI/DOWNLOAD/GLOBE/CHART/BESTDIFF); RSSI from `WiFi.RSSI()` |
| UI-04 | Clock: NTP time (12/24h) + date + BTC price green-up/red-down | ✓ VERIFIED | nm_screen_clock.cpp: `getLocalTime`/strftime, NM_CLOCK_24H + AM/PM, NM_CLOCK_DATE_YMD; price recolor `CLOCK_C_UP`(#84DB31)/`CLOCK_C_DOWN`(#FF0000) via `s_prev_price` delta + `lv_obj_set_style_text_color` |
| UI-05 | Config screen config-mode indicator | ✓ VERIFIED | nm_screen_config.cpp: `Config320x240` descriptor, config-mode indicator label (countdown). AP SSID/portal IP correctly NOT screen widgets (HTTP/wManager) — see deferred |
| UI-06 | All screens native from SPEC.md (recovered evidence, not imagination) | ✓ VERIFIED | All 6 *.cpp `#include "../../recovered/nmminer_palette.h"`; SPEC.md cited in 6 files; recovered/fonts/recovered_fonts.h + fa_nmminer_subset.c present; NMMINER_SYMBOL_* defined in recovered_fonts.h |

**Score:** 6/6 code/build truths verified; 0 physical UI truths verified in this report

### Critical Fixes (from 03-REVIEW.md)

| ID | Issue | Status | Evidence |
|----|-------|--------|----------|
| CR-01 | Legacy Monitor renderer races LVGL on shared tft/SPI | ✓ FIXED | display.cpp:112-135 — `drawCurrentScreen`/`animateCurrentScreen` `#if ESP32_2432S028R... return;` no-op for CYD |
| CR-02 | Teardown deletes active screen inside lv_timer_handler | ✓ FIXED | nm_uimanager.cpp:57-95 — `goto_page_ex` builds new_obj, `lv_scr_load(new_obj)`, THEN `prev->destroy()` (load-then-destroy) |
| — | mining.cpp untouched | ✓ CONFIRMED | `git -C firmware/custom/NerdMiner-CYD-Custom diff HEAD -- src/mining.cpp` → empty |

### Required Artifacts

| Artifact | Status | Details |
|----------|--------|---------|
| src/ui_nm/nm_uimanager.cpp | ✓ VERIFIED | 6-page registry, NVS persist, CR-02 fix, WR-01/02/03 guards |
| src/ui_nm/nm_screen_loading.cpp | ✓ VERIFIED | bar + NMMiner/Make it better/version/kleinpanic |
| src/ui_nm/nm_screen_miner.cpp | ✓ VERIFIED | 24 widgets, FA glyphs, live RSSI/temp/shares/hashrate |
| src/ui_nm/nm_screen_clock.cpp | ✓ VERIFIED | NTP + 12/24h + date + price recolor |
| src/ui_nm/nm_screen_config.cpp | ✓ VERIFIED | Config320x240 config-mode indicator |
| src/ui_nm/nm_screen_sprice.cpp | ✓ VERIFIED | honest stub descriptor (Phase 5) |
| src/ui_nm/nm_screen_weather.cpp | ✓ VERIFIED | honest stub descriptor (Phase 5) |
| recovered/fonts/recovered_fonts.h | ✓ VERIFIED | NMMINER_SYMBOL_* defs, included by all screens |
| recovered/nmminer_palette.h | ✓ VERIFIED | PRICE_UP/DOWN + orange/gray palette |
| drivers/displays/display.cpp | ✓ VERIFIED | CR-01 env-gated no-ops |

### Key Link Verification

| From | To | Via | Status |
|------|----|----|--------|
| nm_uimanager pages[] | 6 nm_screen_t descriptors | static const array, all 6 defined | WIRED |
| nm_uimanager_init | NVS resume | s_prefs.getUInt → goto_page(last) | WIRED |
| nm_uimanager_goto_page | screen build/destroy | next->build()/prev->destroy() | WIRED |
| miner_update | mining_data | getMiningData(0) → label_set_text | WIRED |
| clock update | price color | s_prev_price delta → set_style_text_color | WIRED |
| display.cpp CYD path | LVGL only | drawCurrentScreen no-op (CR-01) | WIRED |

### Anti-Patterns Found

No blocker anti-patterns. Documented INFO-level deviations carried from REVIEW
(IN-01 stale buffer comment, IN-03 1-LSB gray, IN-04 seeded loading IP/TENTATIVE
title coords) are non-blocking and self-documented; WR-01..05 addressed in
commit c609385. SPrice/Weather are honest stubs (Phase 5 scope).

### Human Verification Required

Required in Phase 7 for milestone closure. No hardware test was part of this
original code/build verifier, but the live CYD did not later prove this report's
display assumptions. Treat on-device pixel parity, carousel-swap runtime
behavior, and touch-driven navigation as open Phase 7 hardware work.

### Gaps Summary

No gaps for the original Phase 3 code/build scope. Hardware gaps remain and are
superseded by Phase 7: display rendering, touch navigation, AP recovery behavior,
and physical UI parity must be judged from `07-VERIFICATION.md`, not this report.
All 6 UI requirements (UI-01..UI-06) were verified in code; both code-review
Criticals (CR-01, CR-02) fixed and confirmed in source; mining.cpp untouched;
build SUCCESS reproduced by the verifier (Flash 87.3% / RAM 35.3%). Two items are
deferred to Phase 5 (live network/pool diff data; captive-portal AP behavior) -
these are out of Phase 3's structural-identity scope per ROADMAP and do not block
the code/build goal.

---

_Verified: 2026-06-18_
_Verifier: Claude (gsd-verifier)_
