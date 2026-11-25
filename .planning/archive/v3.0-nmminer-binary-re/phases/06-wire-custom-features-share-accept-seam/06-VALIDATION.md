# Phase 6 Validation — Wire Custom Features at the Share-Accept Seam

> **RESULT: PENDING** (run after 06-02 integration gate).
>
> This is a firmware phase — the gate is **PlatformIO build SUCCESS** + presence/diff gates.
> On-device audible (beep), visible (LED), and SD-card-logging confirmation is **Phase 7**
> (BLOCKED on /dev/ttyUSB0). RESEARCH/CONTEXT: Phase 6 gate is BUILD SUCCESS only.

**Build command (canonical):**
```
cd firmware/custom/NerdMiner-CYD-Custom && \
  /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R
```

All presence gates run from `firmware/custom/NerdMiner-CYD-Custom/`. Each token gate filters
comments before counting where relevant so header prose does not self-satisfy.

---

## 1. Build Gate (phase blocker)

| Gate | Command | Pass condition | Result |
|------|---------|----------------|--------|
| Full build | `pio run -e ESP32-2432S028R 2>&1 \| tail -6` | `SUCCESS` present, no `error:` | PENDING |
| Flash budget | `ls -la .pio/build/ESP32-2432S028R/firmware.bin` | size recorded; headroom vs 3,145,728 B computed; overflow surfaced, no HW req dropped | PENDING |

Baseline for the budget delta: 2,852,288 B bin / 90.5% of 3MB / 299,743 B (9.5%) headroom
(Phase-5 05-VALIDATION.md). The beep + LED + at-seam SD log hooks are tiny; expect ~90.5% +/- a hair.

---

## 2. Requirement → Presence/Diff Gate Map

| Req | Behavior | Gate command | Pass | Result |
|-----|----------|--------------|------|--------|
| HW-01 | Speaker beeps on accepted share + alarms on disconnect, named seam hook (D-62) | `grep -q 'beepShareAccepted' include/speaker.h && grep -q 'Speaker::beepShareAccepted' src/speaker.cpp && grep -A1 'shares++;' src/mining.cpp \| grep -q 'Speaker::beepShareAccepted();' && grep -q 'Speaker::playDisconnectAlarm' src/mining.cpp` | exit 0 | PENDING |
| HW-01b | Accepted-share tone is non-blocking (no vTaskDelay) and distinct from the disconnect alarm | `! grep -q 'vTaskDelay' <(awk '/void Speaker::beepShareAccepted/,/^}/' src/speaker.cpp)` | exit 0 | PENDING |
| HW-02 | SD mounts on VSPI + logs accepted shares, graceful when absent (D-63) | `grep -q 'vspi.begin' src/sdstore.cpp && grep -q 'SD_FREQUENCY_MHZ 4' include/sdstore.h && grep -q 'logShareEvent' src/NerdMinerV2.ino.cpp && grep -A40 'void logShareEvent' src/NerdMinerV2.ino.cpp \| grep -q 'isReady' && grep -A40 'void logShareEvent' src/NerdMinerV2.ino.cpp \| grep -q 'appendCSVLog'` | exit 0 | PENDING |
| HW-03 | Custom LED status on silkscreen RGB pins + kleinpanic accent | `grep -q 'nm_led_init' include/nm_led.h && grep -q 'LED_PIN' src/nm_led.cpp && grep -q 'nm_led_init' src/NerdMinerV2.ino.cpp && grep -q 'nm_led_tick' src/NerdMinerV2.ino.cpp && grep -q 'kleinpanic' src/ui_nm/nm_screen_loading.cpp` | exit 0 | PENDING |
| HW-03b | LED module uses pin macros, not raw GPIO numbers, and matches the driver polarity | `grep -Eq 'LED_PIN_G\|LED_PIN_B' src/nm_led.cpp && ! grep -Eq 'pinMode\([[:space:]]*4[[:space:]]*,' src/nm_led.cpp` | exit 0 | PENDING |
| HW-04 | Mining untouched except the one seam line (D-65 evidence) | `test "$(git -C . diff <phase-base>..HEAD -- src/mining.cpp \| grep -E '^[+-]' \| grep -vE '^[+-]{3} ' \| grep -vc 'Speaker::')" = "0"` | exit 0 | PENDING |

`<phase-base>` = the fork HEAD SHA recorded before 06-01 (the executor records it in 06-02 SUMMARY).

---

## 3. Mining-Sacrosanct Gate (HW-04, load-bearing)

The whole-phase mining.cpp diff MUST be exactly:
```
-                                            Speaker::playShareBeep();
+                                            Speaker::beepShareAccepted();
```
No change at the disconnect-alarm line (mining.cpp:263), no change to `shares++`, the
is32bit/isValid block, stratum parsing, pool primary/fallback, or hashing. The full diff is pasted
into 06-02-SUMMARY.md as the HW-04 evidence. Stratum core / pool switch-back ≤10s logic is unchanged.

---

## 4. Notes

- **Nyquist:** every gate above has an automated command; no manual-only checks at this stage.
- **Deferred to Phase 7:** beep audible on a real accepted share, LED visibly changing on
  mining/share/disconnect, SD card physically logging rows — all require /dev/ttyUSB0 + hardware.
- **D-61 confirm-not-rewrite:** disconnect alarm (mining.cpp:263) and SD mount (.ino:166) were
  already built; this phase confirms they link in the integrated build and adds only the named
  beep, the at-seam SD log, and the LED status module.
- **Security:** local HW hooks only, no new network/credential surface, no package installs; D-11
  (no closed-firmware bytes linked) holds.
