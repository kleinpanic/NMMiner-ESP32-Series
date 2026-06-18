---
phase: 06-wire-custom-features-share-accept-seam
verified: 2026-06-18T12:30:00Z
status: human_needed
score: 4/4 must-haves verified (static/build); on-device confirm deferred to Phase 7
re_verification:
  previous_status: null
overrides: []
human_verification:
  - test: "Submit/wait for a real accepted share on hardware; listen for the 2000Hz/80ms blip"
    expected: "Single short high-pitched beep on each accepted share; distinct two-blip 700Hz alarm on pool/WiFi disconnect"
    why_human: "Audible output requires powered hardware on /dev/ttyUSB0 (Phase 7). Static path verified."
  - test: "Observe the CYD RGB LED while mining, on an accepted share, and during disconnect"
    expected: "Steady blue while hashing; brief green flash on accepted share; red ~1Hz blink while connecting/disconnected"
    why_human: "Visible LED behavior requires powered hardware (Phase 7). Sole-writer + logic verified statically."
  - test: "Insert a real FAT SD card, mine, then read /sd/mining_stats.csv"
    expected: "Periodic stats rows AND 'event=share,...' rows appended; no card present = no crash, mining unaffected"
    why_human: "Physical SD card I/O on real hardware (Phase 7). Mount/log/graceful-absent path verified statically."
  - test: "Confirm mining still accepts shares + pool primary/fallback switch-back <=10s on hardware"
    expected: "Mining functional throughout; stratum/pool behavior identical to pre-phase"
    why_human: "On-device mining confirmation is Phase 7 (VALID-02). Stratum core proven unchanged by diff statically."
---

# Phase 6: Wire Custom Features at Share-Accept Seam — Verification Report

**Phase Goal:** Light up custom hardware (speaker beep on accepted share + disconnect alarm, SD logging on the now-free VSPI, custom RGB LED status + kleinpanic accents) wired at the NerdMiner share-accept seam as an in-process call, with mining staying fully functional. Build SUCCESS is the gate; on-device confirm is Phase 7.
**Verified:** 2026-06-18
**Status:** human_needed (all static/build gates PASS; on-device behavior DEFERRED→Phase 7 per scope)
**Re-verification:** No — initial verification
**Phase commit range:** fork base 8b9814d .. HEAD 9a476e7

## Goal Achievement

### Per-Requirement Verdict

| Req | Verdict | Static/Build Evidence |
|-----|---------|-----------------------|
| HW-01 | PASS (on-device DEFERRED→P7) | `beepShareAccepted()` declared + defined non-blocking (playTone 2000Hz/80ms, no vTaskDelay), distinct from disconnect alarm (700Hz two-blip); seam call right after `shares++`; disconnect alarm still hooked at line 263; both are in-process C++ calls |
| HW-02 | PASS (on-device DEFERRED→P7) | SD mounts on dedicated `SPIClass vspi(VSPI)` (5/18/19/23); `logShareEvent()` loop-driven, `isReady()`-gated, appends via existing `appendCSVLog` (no new method); zero SD work in mining.cpp |
| HW-03 | PASS (on-device DEFERRED→P7) | `nm_led` uses LED_PIN/LED_PIN_G/LED_PIN_B macros, active-LOW, millis()-gated, no delay; CR-01 closed — stock per-frame RGB writer compiled out via `#ifndef NM_LED_OWNS_RGB` (define present); kleinpanic accent present on loading screen |
| HW-04 | PASS | mining.cpp whole-phase diff = exactly one line (`playShareBeep()`→`beepShareAccepted()`); 0 non-Speaker changed lines; no blocking call introduced on mining path; BUILD SUCCESS at 90.5% flash |

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Accepted-share beep is wired in-process at the seam | ✓ VERIFIED | `mining.cpp:452 shares++; 453 Speaker::beepShareAccepted();` — additive C++ call, not Xtensa hook |
| 2 | Accepted-share tone is non-blocking + distinct from alarm | ✓ VERIFIED | `beepShareAccepted()` = `playTone(2000,80)`, no vTaskDelay; alarm = 700Hz x2 (speaker.cpp:129-145) |
| 3 | Disconnect alarm remains hooked | ✓ VERIFIED | `mining.cpp:263 Speaker::playDisconnectAlarm();` unchanged |
| 4 | SD mounts on dedicated VSPI bus, graceful when absent | ✓ VERIFIED | `sdstore.cpp:6 static SPIClass vspi(VSPI)`, `:28 vspi.begin(18,19,23,5)`, `:39 CARD_NONE → SD.end()` returns false (no crash) |
| 5 | Accepted shares logged to SD off the mining path | ✓ VERIFIED | `logShareEvent()` in .ino:301, called from loop:448, `isReady()`-gated, delta on `shares`, `appendCSVLog` (no mining.cpp SD work) |
| 6 | Custom RGB LED status uses silkscreen pin macros, non-blocking | ✓ VERIFIED | `nm_led.cpp` writeRGB via LED_PIN/G/B, active-LOW, REPAINT_INTERVAL_MS millis gate, no delay; tick from loop:457 |
| 7 | nm_led is the SOLE persistent RGB writer (CR-01) | ✓ VERIFIED | stock `DoLedStuff` per-frame `switch(NerdStatus)` RGB writes wrapped in `#ifndef NM_LED_OWNS_RGB` (esp23_2432s028r.cpp:627-654); `#define NM_LED_OWNS_RGB` at line 11 → block compiled out |
| 8 | kleinpanic accent present | ✓ VERIFIED | `nm_screen_loading.cpp:111 lv_label_set_text(kp, "kleinpanic")` (D-37, prior phase) |
| 9 | mining.cpp changed ONLY by the seam line (HW-04 gate) | ✓ VERIFIED | diff 8b9814d..9a476e7 = single line swap; 0 non-Speaker changed lines |
| 10 | No blocking call introduced on mining/seam path | ✓ VERIFIED | beep arms tone + returns; WR-03 fix gates per-tone Serial.printf behind `#ifdef SPEAKER_DEBUG` (speaker.cpp:66) |
| 11 | Firmware builds | ✓ VERIFIED | pio run ESP32-2432S028R → SUCCESS, 93s; Flash 90.5%, headroom 9.5% |

**Score:** 11/11 static truths verified. On-device behavior (audible/visible/physical-SD/live-mining) = DEFERRED→Phase 7 (explicit scope, not a failure).

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `include/speaker.h` | declares beepShareAccepted | ✓ VERIFIED | line 54 |
| `src/speaker.cpp` | non-blocking accepted-share tone | ✓ VERIFIED | lines 129-135, no vTaskDelay |
| `src/mining.cpp` | seam call + alarm only | ✓ VERIFIED | :453 seam, :263 alarm; single-line phase diff |
| `include/nm_led.h` + `src/nm_led.cpp` | macro-based, non-blocking RGB status | ✓ VERIFIED | full module, millis-gated |
| `src/drivers/displays/esp23_2432s028r.cpp` | stock RGB writer compiled out | ✓ VERIFIED | guard + define present |
| `include/sdstore.h` + `src/sdstore.cpp` | VSPI mount, appendCSVLog | ✓ VERIFIED | dedicated vspi, graceful absent |
| `src/NerdMinerV2.ino.cpp` | loop wiring (init+tick+log), isReady-gated | ✓ VERIFIED | :167 mount, :240/244 init, :442/448/457 loop |

### Key Link Verification

| From | To | Via | Status |
|------|----|-----|--------|
| mining.cpp seam | Speaker::beepShareAccepted | in-process call after shares++ | ✓ WIRED |
| mining.cpp disconnect | Speaker::playDisconnectAlarm | in-process call line 263 | ✓ WIRED |
| loop() | logShareEvent → appendCSVLog | isReady-gated, shares delta | ✓ WIRED |
| loop() | nm_led_tick(millis()) | non-mining loop, shares delta auto-flash | ✓ WIRED |
| esp23 driver | nm_led sole RGB owner | NM_LED_OWNS_RGB guard compiles out stock writer | ✓ WIRED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| HW-04 diff = single line | `git diff 8b9814d..9a476e7 -- src/mining.cpp` | only `playShareBeep`→`beepShareAccepted` | ✓ PASS |
| HW-04 non-Speaker delta = 0 | grep -vc 'Speaker::' on +/- lines | `0` | ✓ PASS |
| Build | `pio run -e ESP32-2432S028R` | SUCCESS, Flash 90.5% (2,846,633/3,145,728) | ✓ PASS |
| beepShareAccepted non-blocking | grep vTaskDelay in fn body | none | ✓ PASS |
| WR-01 volatile match | grep extern shares | all 5 consumers `extern volatile` | ✓ PASS |
| WR-03 hot-path printf gated | grep speaker.cpp:66 | `#ifdef SPEAKER_DEBUG` | ✓ PASS |
| CR-01 sole writer | grep NM_LED_OWNS_RGB | define line 11 + guard 627 | ✓ PASS |

### Code Review Closure (06-REVIEW.md)

| Finding | Severity | Fix commit | Verified in tree |
|---------|----------|-----------|------------------|
| CR-01 nm_led second persistent RGB writer | Critical | b84dca3 | ✓ guard + define present, stock block compiled out |
| WR-01 shares volatile mismatch | Warning | 0e1150a | ✓ all externs `extern volatile uint32_t` |
| WR-03 Serial.printf on stratum hot path | Warning | 1c2bdfb | ✓ gated behind SPEAKER_DEBUG |
| IN-03 Brightness unclamped | Info | 9a476e7 | ✓ `constrain(...,0,255)` |
| WR-02 torn upTime read | Warning | (accepted) | ✓ documented snapshot, log-only (accepted) |
| WR-04 dual time source nm_led | Warning | (latent) | callers all pass millis(); single-clock in practice |
| WR-05 / IN-01/02/04 | Warning/Info | carry-forward | accepted, non-blocking for this phase |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| esp23_2432s028r.cpp | 109-114 | one-shot init digitalWrite on LED_PIN/G/B outside NM_LED_OWNS_RGB guard | ℹ️ Info | One-time init write only; nm_led_init() runs after and nm_led_tick is the sole *persistent* writer. Not a second persistent writer. No blocker. |

### Requirements Coverage

| Requirement | Description | Status | Evidence |
|-------------|-------------|--------|----------|
| HW-01 | Speaker beep on accepted share + disconnect alarm at seam, in-process | ✓ SATISFIED (static); on-device→P7 | seam :453, alarm :263, non-blocking |
| HW-02 | SD on VSPI mounts + logs, graceful absent | ✓ SATISFIED (static); on-device→P7 | dedicated vspi, logShareEvent, isReady-gated |
| HW-03 | Custom LED + kleinpanic, silkscreen pins | ✓ SATISFIED (static); on-device→P7 | nm_led macros, sole writer, kleinpanic label |
| HW-04 | Mining fully functional, stratum unchanged | ✓ SATISFIED | single-line diff, build SUCCESS |

### Human Verification Required (DEFERRED→Phase 7)

Per phase scope (CONTEXT §Deferred, VALIDATION §4), on-device confirmation is explicitly Phase 7 (BLOCKED on /dev/ttyUSB0). These are NOT failures:

1. **Audible beep** on a real accepted share + disconnect alarm.
2. **Visible LED** — steady blue mining / green flash on share / red blink disconnect.
3. **SD logging** on a real FAT card (stats + event=share rows; no-card graceful).
4. **Live mining** still accepts shares + pool switch-back ≤10s on hardware (VALID-02).

### Gaps Summary

No gaps. All four phase requirements (HW-01..04) are satisfied at the static/build level — the explicit Phase 6 gate. The load-bearing HW-04 mining-sacrosanct gate passes exactly (single-line diff, 0 non-Speaker deltas, build SUCCESS at 90.5% flash / 9.5% headroom). The one Critical review finding (CR-01, second persistent RGB writer) was closed by commit b84dca3 and is verified in-tree: the stock per-frame RGB writer is compiled out under `#ifndef NM_LED_OWNS_RGB`. On-device behavioral confirmation is correctly out of Phase 6 scope and routed to Phase 7.

---

_Verified: 2026-06-18_
_Verifier: Claude (gsd-verifier)_
