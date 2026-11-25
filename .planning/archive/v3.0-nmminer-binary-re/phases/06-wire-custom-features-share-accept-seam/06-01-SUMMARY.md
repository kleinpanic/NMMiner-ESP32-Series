---
phase: 06-wire-custom-features-share-accept-seam
plan: 01
subsystem: firmware-custom-hw
tags: [speaker, led, share-accept-seam, hw-01, hw-03, hw-04]
requires:
  - "Speaker class (init/playTone/update/playShareBeep/playDisconnectAlarm) — pre-existing fork driver"
  - "mMonitor.NerdStatus (monitor.h NMState) — mining vs connecting state"
  - "shares counter (mining.cpp) — accepted-share signal, read-only"
  - "LED_PIN/LED_PIN_G/LED_PIN_B device macros (esp322432s028r.h)"
provides:
  - "Speaker::beepShareAccepted() — named accepted-share seam tone (D-62)"
  - "nm_led module (nm_led_init/nm_led_tick/nm_led_signal_share) — custom RGB status (HW-03)"
affects:
  - "src/mining.cpp share-accept seam (single seam-line swap, HW-04 preserved)"
  - "src/NerdMinerV2.ino.cpp setup()/loop() CYD env guards"
tech-stack:
  added: []
  patterns:
    - "Non-blocking millis()-gated loop tick (no delay/vTaskDelay) on the non-mining loop"
    - "State derived from existing monitor status — zero new mining.cpp hook for the LED"
key-files:
  created:
    - firmware/custom/NerdMiner-CYD-Custom/include/nm_led.h
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_led.cpp
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/include/speaker.h
    - firmware/custom/NerdMiner-CYD-Custom/src/speaker.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
decisions:
  - "beepShareAccepted = single 2000Hz/80ms blip — distinct from the 700Hz two-blip disconnect alarm by frequency, duration, and pattern (single vs double)."
  - "LED state map: blue steady = hashing, red ~1Hz slow-blink = connecting/disconnect, brief 400ms green flash = accepted share."
  - "Accepted-share flash auto-detected from the shares counter delta in nm_led_tick (so it works on the stratum task path); nm_led_signal_share() also exposed for explicit arming."
  - "FindMe and status share pins 4/16/17 with no exposed FindMe-active query; FindMe wins transiently during its momentary blink, status repaints after — acceptable."
metrics:
  duration: "~15m"
  completed: "2026-06-18"
  tasks: 2
  files: 6
---

# Phase 6 Plan 01: Wire Custom Features at Share-Accept Seam Summary

Named the accepted-share seam beep per D-62 (`Speaker::beepShareAccepted()`, 2000Hz/80ms single blip) and pointed the single intentional mining.cpp seam line at it; added a non-blocking `nm_led` RGB status module (blue=mining / red-blink=disconnect / green-flash=accepted share) driven from the CYD loop — mining hashing/stratum logic untouched (HW-04).

## What Was Built

### Task 1 — accepted-share beep named per D-62 (HW-01)
- `include/speaker.h`: added `static void beepShareAccepted();` next to the existing event helpers; `playShareBeep()` kept declared+defined (additive, no removal).
- `src/speaker.cpp`: `Speaker::beepShareAccepted()` = `playTone(2000, 80)` — a single short high-pitched blip. **Distinctness from the disconnect alarm** (`playDisconnectAlarm`, 700Hz × two blips with vTaskDelay): higher frequency (2000 vs 700 Hz), shorter (80 vs 120ms), and single-tone vs two-blip pattern. **Non-blocking**: arms the tone via `playTone()` and returns; no `vTaskDelay`, so the stratum/mining loop is never stalled. `update()` (already in the CYD loop) clears the tone.
- `src/mining.cpp`: the ONLY change is the single seam line right after `shares++;` (line 453): `Speaker::playShareBeep();` → `Speaker::beepShareAccepted();`. `shares++`, the is32bit/isValid block, line 263 disconnect alarm, and all stratum/hashing code are untouched.

### Task 2 — nm_led RGB status module (HW-03)
- `include/nm_led.h`, `src/nm_led.cpp`: small non-blocking status driver on the silkscreen RGB header via the `LED_PIN`/`LED_PIN_G`/`LED_PIN_B` macros (no raw 4/16/17 literals). Active-LOW polarity (`LED_ON = LOW`), matching the display driver idle state (esp23_2432s028r.cpp:104-106) and the FindMe flash (nm_screensaver.cpp:339) so it never fights the driver.
- **State → color mapping (D-64 discretion):**
  - `NM_hashing` → steady **blue** (calm mining/connected status)
  - `NM_Connecting` / `NM_waitingConfig` → **red ~1Hz slow-blink** (distinct disconnect/connecting state)
  - accepted share → brief **400ms green flash** (overrides status while active)
- The accepted-share flash is **auto-detected** from the `shares` counter delta read inside `nm_led_tick` (so it fires even though the share path runs on the stratum task) — this means **no new hook was added to mining.cpp** for the LED (HW-04). `nm_led_signal_share()` is also exposed for explicit arming if a future caller wants it.
- State derived from `mMonitor.NerdStatus`; `nm_led_tick` is millis()-gated (200ms status repaint cadence) with no `delay()`/`vTaskDelay()`.
- `src/NerdMinerV2.ino.cpp`: `#include "nm_led.h"`; `nm_led_init()` in `setup()` inside the CYD env guard (next to `Speaker::init()`); `nm_led_tick(millis())` in `loop()` inside the CYD env guard next to `nm_uimanager_tick()`.

## FindMe / status pin-sharing note
FindMe (Phase 4) directly drives pins 4/16/17 (`nm_screensaver.cpp findme_led`). There is no exposed "FindMe active" query in any header (`s_findme_active` is file-static in nm_uimanager.cpp), so `nm_led_tick` cannot cleanly yield to it. The two therefore share the pins: **FindMe wins transiently** during its momentary user-triggered blink, and the next `nm_led_tick` (≤200ms later) repaints the status color. This is acceptable — FindMe is a deliberate momentary override, not a persistent state.

## Deviations from Plan
None — plan executed exactly as written. (The `nm_led_signal_share()` auto-detect via the shares-counter delta is the plan's stated fallback for "if reachable from the loop ... otherwise leave the share flash driven purely by nm_led_tick reading the share counter delta".)

## HW-04 anchor
`/tmp/nmm-phase6-base-sha` = `8b9814d444123eeecc1b607d865670278e6f76cc` (recorded as Task 1's first action). `git diff 8b9814d -- src/mining.cpp` is exactly the one seam-line swap — verified after both tasks. The full-diff gate runs in 06-02.

## Verification Results
- Task 1: `BEEP_OK` — speaker.h declares beepShareAccepted; speaker.cpp defines it; no vTaskDelay in its body; the line after `shares++;` calls `Speaker::beepShareAccepted();`.
- Task 2: `LED_OK` — nm_led.h declares nm_led_init; nm_led.cpp uses LED_PIN macros; no GPIO_NUM_4/raw-4-pinMode; ino.cpp calls nm_led_init + nm_led_tick. No `delay`/`vTaskDelay`/raw pin literals in nm_led.cpp.
- mining.cpp diff vs base = single seam line (confirmed after both commits).
- Build gate (`pio run`) is owned by Plan 06-02 (file-ownership), not run here.

## Commits (fork repo: firmware/custom/NerdMiner-CYD-Custom)
- `dc5ebd8`: feat(06-01): name accepted-share seam beep per D-62
- `22fe287`: feat(06-01): add nm_led RGB status module + CYD loop wiring (HW-03)

## Self-Check: PASSED
