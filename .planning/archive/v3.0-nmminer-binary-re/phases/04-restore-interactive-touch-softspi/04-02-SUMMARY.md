---
phase: 04-restore-interactive-touch-softspi
plan: 02
subsystem: touch-nav-ui
tags: [touch, gesture, onebutton, lvgl, carousel, backlight, idle-dim, cyd]
requires:
  - "Plan 04-01 nm_touch SoftSPI bus reader (pressed/raw/point + nm_touch_init)"
  - "Phase 3 nm_uimanager carousel (goto_page_ex/next_page) + nm_display ledc backlight"
  - "wManager reset_configuration() — config-mode entry"
provides:
  - "nm_display runtime backlight API: nm_display_set_brightness/wake/dim (D-45)"
  - "nm_uimanager input handlers: on_touch (short/long/wake) + on_button_next/config/wake"
  - "Minimal idle->dim timer (NM_UI_IDLE_MS=60s); shared NM_TOUCH_LONG_MS=10s define"
  - "CYD boot button re-pointed to nm_ui; SoftSPI touch polled+classified in loop()"
  - "NM_UI_AUTOADVANCE default OFF (input now drives nav)"
affects:
  - "firmware/custom/NerdMiner-CYD-Custom (nested fork repo)"
tech-stack:
  added: []
  patterns:
    - "Non-blocking gesture classifier (press-edge/hold/release-edge over millis() deltas, no delay)"
    - "Shared #define for button + touch long-press window (no drift between config paths)"
    - "Env-guarded input wiring (#if ESP32_2432S028R||ESP32_2432S028_2USB) keeps other boards legacy"
    - "Wake-consumes-event (tap on dim screen wakes only, does not also navigate)"
key-files:
  created: []
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
decisions:
  - "D-45: touch short=next page (persists), long(10s)=config, tap-on-dim=wake; boot button double=next/long=config/single=wake"
  - "NM_TOUCH_LONG_MS=10000 single shared define in nm_uimanager.h drives BOTH button setPressMs and the loop touch classifier"
  - "NM_UI_IDLE_MS=60s minimal idle->dim only; full screensaver state machine deferred to Phase 5"
  - "nm_display_wake re-ramps current->target over ~120ms reusing the existing ledc channel 0 (no second ledcAttachPin)"
  - "nm_touch_log_raw NOT wired into the default loop — kept as the on-demand Phase-7 evidence hook (no serial spam in shipping nav)"
  - "Dormant legacy HSPI touch object (esp23_2432s028r.cpp) left unedited; confirmed unreached (initDisplay skipped for CYD)"
metrics:
  duration: "~10 min"
  completed: 2026-06-18
  tasks: 3
  files: 5
---

# Phase 4 Plan 02: Touch/Button Navigation Wiring (nm_ui) Summary

The Phase-3 carousel is now interactive. The SoftSPI `nm_touch` bus (Plan 04-01) and the boot
button (OneButton GPIO0) drive `nm_uimanager`: a short touch / button double-click advances the
carousel one page (persisting LastPage), a 10s long-press / long-hold enters config mode
(`reset_configuration` → AP portal), and a tap / single-click on a dimmed screen wakes it
(re-ramping the backlight). A minimal idle→dim timer drops the backlight after 60s. The Phase-3
debug 6s auto-advance is OFF by default now that input drives nav. CYD env builds SUCCESS
(RAM 35.3%, Flash 87.4% — +0.1% vs the 04-01 baseline).

## What Was Built

| Task | Name | Status | Commit | Files |
|------|------|--------|--------|-------|
| 1 | Runtime wake/brightness API on nm_display | Done | `f7adcd1` | src/ui_nm/nm_display.{h,cpp} |
| 2 | UIManager touch/button nav + idle-dim + autoadvance OFF | Done | `a776028` | src/ui_nm/nm_uimanager.{h,cpp} |
| 3 | Re-point boot button + poll touch in ino.cpp; build gate + VALIDATION | Done | `8432982` | src/NerdMinerV2.ino.cpp |

Commits are in the **nested fork repo** `firmware/custom/NerdMiner-CYD-Custom`. The parent
gitlink bump is the orchestrator's responsibility.

## Gesture / Button → nm_ui Map (D-45)

| Input | Gesture | nm_ui action |
|---|---|---|
| Touch tap (release < 800ms) | SHORT | `nm_uimanager_next_page()` (persists LastPage) |
| Touch hold ≥ 10s | LONG (edge-fired while held) | `reset_configuration()` (config/AP mode) |
| Touch tap while dimmed | WAKE (consumed) | `nm_display_wake()` only — no nav |
| Boot button double-click | next | `nm_uimanager_next_page()` |
| Boot button long-press (10s) | config | `reset_configuration()` |
| Boot button single-click | wake | `nm_display_wake()` if dimmed |
| No input > 60s | idle | `nm_display_dim()` (minimal; Phase-5 deferred) |

Button `setPressMs(NM_TOUCH_LONG_MS)` and the loop classifier's long-press threshold are the
**same** `#define` (10000ms) in `nm_uimanager.h` — the two config-entry windows cannot drift.

## API Added

**nm_display (Task 1):** `nm_display_set_brightness(uint8_t)`, `nm_display_wake(void)`,
`nm_display_dim(void)` — all reuse the existing ledc channel 0 / TFT_BL attached at boot (no
second `ledcAttachPin`). Boot brightness target stored in a file static so `wake` re-ramps to it.

**nm_uimanager (Task 2):** `nm_touch_gesture_t {NONE,SHORT,LONG,WAKE}`,
`nm_uimanager_on_touch(g)`, `nm_uimanager_on_button_next/_config/_wake(void)`. Internal:
`s_dimmed` + `s_last_input_ms` state; `nm_input_wake_if_dimmed()` consumes the first event when
dimmed; idle check in `nm_uimanager_tick()`. Shared `NM_TOUCH_LONG_MS` + `NM_UI_IDLE_MS` defines.

**ino.cpp (Task 3):** `nm_poll_touch_gesture()` non-blocking classifier (press-edge / held-edge
config-fire / release-edge short); CYD inner guard re-points `button1` to the nm_ui callbacks;
`nm_touch_init()` added at CYD setup.

## NM_UI_AUTOADVANCE OFF — Confirmation

`nm_uimanager.cpp` now defines `#ifndef NM_UI_AUTOADVANCE` → `#define NM_UI_AUTOADVANCE 0`
(was `1`). The `lv_timer_create(nm_autoadvance_cb, ...)` block stays behind `#if NM_UI_AUTOADVANCE`
so a build flag (`-D NM_UI_AUTOADVANCE=1`) can still re-enable the debug cycle. Grep gate
`NM_UI_NAV_OK` confirmed the `define NM_UI_AUTOADVANCE 0`.

## Reuse-vs-Rewrite Decision

**Considered & reused:** `nm_uimanager_next_page()` / `goto_page_ex(persist)` (Phase 3 — already
persists, so D-45's "persist=true" is met without new persist logic); the existing ledc channel 0
boot setup in `nm_display_init` (wake/dim/set just `ledcWrite` it — no duplicate `ledcSetup`);
`reset_configuration()` (wManager — the exact stock config-mode entry the legacy boot long-press
used); the existing OneButton `button1` object (only the attached callbacks changed for CYD);
`nm_touch_*` bus reader (Plan 04-01) for the raw press read. **Not reused:** the legacy
`TouchHandler` gesture/debounce shape — it is VSPI-bound and gated dormant (Plan 04-01); the new
classifier is a ~30-line non-blocking state machine over `nm_touch_pressed()`, not a parallel
re-implementation of the VSPI handler. **Zero files created** — all five targets are edits.

## Deviations from Plan

### Auto-fixed Issues

None requiring a rule. One micro-cleanup during Task 3: an initial gesture-classifier draft had a
no-op ternary (`held < kShortMaxMs ? NM_TOUCH_SHORT : NM_TOUCH_SHORT`); simplified to a plain
`NM_TOUCH_SHORT` on the release edge (config is fired on the held-edge path, which consumes the
gesture, so a release without `long_fired` is always SHORT). No behavior change vs intent; caught
before commit, so not logged as a Rule deviation.

### Scope notes
- The `nm_touch_log_raw()` debug seam was intentionally **not** wired into the default loop (the
  plan made it optional/behind-a-flag). It stays the on-demand Phase-7 evidence hook — wiring it
  into the shipping nav loop would spam serial. Documented in 04-VALIDATION Gate C with the exact
  serial-line shape.
- `esp23_2432s028r.cpp` (dormant HSPI touch object): **not edited** (out of scope). Confirmed
  unreached for the CYD env (`initDisplay()` is in ino.cpp's `#else` branch; the only caller of
  `esp32_2432S028R_Init` is the driver-table function pointer reached via `initDisplay()`).
  Recorded in 04-VALIDATION Gate B; flagged for Phase 6 (SD) double-bind risk.

## Constraints Honored

- mining.cpp, Phase-3 ui_nm screens, STATE.md, ROADMAP.md, esp23_2432s028r.cpp: **NOT touched.**
- Pre-existing uncommitted fork changes (sdstore.h, monitor.cpp, wManager.cpp, branding/logo_test.png):
  **left alone** — unrelated to this plan; staged only the 5 task files by name.
- Nested-repo commits via `git -C` in the fork; SUMMARY + VALIDATION in parent.
- nm_touch stays bit-bang; VSPI not bound; legacy VSPI TouchHandler not instantiated (its
  `isTouched()` lives in the `#elif !NM_TOUCH_SOFTSPI` branch, not compiled for CYD).

## Build Result (Gate A)

```
Environment      Status    Duration
ESP32-2432S028R  SUCCESS   00:01:21
RAM:   35.3% (115768 / 327680 bytes)
Flash: 87.4% (2747897 / 3145728 bytes)
```
+0.1% Flash vs the Plan 04-01 baseline (87.3%) for the wake/dim API + nav handlers + classifier.
Full log: `/tmp/nm04_build_final.log`. Sentinels printed: `NM_DISPLAY_WAKE_OK`, `NM_UI_NAV_OK`,
`NM04_PHASE_BUILD_OK`.

## Known Stubs

None. The nav path is fully wired (real handlers, real ledc writes, real gesture classification).
Idle→dim is intentionally minimal (single timeout, no multi-state screensaver) — the full idle
state machine is explicitly Phase 5, not a stub. On-device tap/long-press/wake confirmation is
Phase 7 (gated on hardware), by design (D-45/D-43).

## Self-Check: PASSED

- FOUND: src/ui_nm/nm_display.{h,cpp}, nm_uimanager.{h,cpp}, NerdMinerV2.ino.cpp (modified in fork)
- FOUND: fork commits f7adcd1, a776028, 8432982 (fork git log)
- Build sentinel NM04_PHASE_BUILD_OK + grep sentinels NM_DISPLAY_WAKE_OK / NM_UI_NAV_OK printed.
