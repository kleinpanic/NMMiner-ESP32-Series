---
phase: 06-wire-custom-features-share-accept-seam
reviewed: 2026-06-18T00:00:00Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/include/speaker.h
  - firmware/custom/NerdMiner-CYD-Custom/src/speaker.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp
  - firmware/custom/NerdMiner-CYD-Custom/include/nm_led.h
  - firmware/custom/NerdMiner-CYD-Custom/src/nm_led.cpp
  - firmware/custom/NerdMiner-CYD-Custom/include/sdstore.h
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/monitor.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
findings:
  critical: 1
  warning: 5
  info: 4
  total: 10
status: issues_found
---

# Phase 6: Code Review Report

**Reviewed:** 2026-06-18
**Depth:** standard (C++/Arduino/ESP32-aware)
**Files Reviewed:** 9
**Status:** issues_found

## Summary

HW-04 is satisfied: `mining.cpp`'s only change is the single seam-line swap (`playShareBeep` -> `beepShareAccepted`) at line 453; disconnect alarm (line 263), `shares++`, and all stratum/hashing/pool code are untouched. `beepShareAccepted()` is genuinely non-blocking (arms `playTone` and returns; no `vTaskDelay`), so the mining/stratum task is not stalled and the ESP32 watchdog is not at risk from the beep. SD logging is correctly `isReady()`-gated and uses the existing rotation-bounded `appendCSVLog` — graceful when no card is present.

One Critical finding: the new `nm_led` module is a **second, persistent writer** of the same three GPIOs (4/16/17) that the stock CYD display driver already drives every frame from the mining/stat task — the two fight continuously, not just transiently as the summary claims. Several Warnings concern `volatile`-qualifier inconsistency on the shared `shares` counter, torn 64-bit reads of `upTime`, `Serial.printf` now executing on the stratum task path, and the LED `now_ms - s_last_repaint` rollover gate.

## Critical Issues

### CR-01: nm_led_tick contends with the stock display driver for the RGB pins (4/16/17) on every cycle

**File:** `src/nm_led.cpp:31-46, 64-86` and `src/drivers/displays/esp23_2432s028r.cpp:619-640` (called via `doLedStuff` from `src/mining.cpp:1280`)
**Issue:** The summary documents only a *transient* FindMe pin-share and calls it "acceptable." That is incomplete. The device's OWN stock display driver, `esp32_2432S028R_DoLedStuff()` (`esp23_2432s028r.cpp:619-640`), drives `LED_PIN`/`LED_PIN_G`/`LED_PIN_B` based on `mMonitor.NerdStatus` — and it is wired into the driver vtable (`esp23_2432s028r.cpp:659`) and called as `doLedStuff(frame)` from the mining/stat task loop at `mining.cpp:1280` (and `NerdMinerV2.ino.cpp:160`).

So on the CYD env there are now TWO persistent writers of pins 4/16/17:
- stock `doLedStuff` from the mining/stat task (`mining.cpp:1280`), and
- new `nm_led_tick` from the main Arduino loop (`NerdMinerV2.ino.cpp:446`).

Both key off the same `mMonitor.NerdStatus`, but with different polarity conventions and blink phases. The stock driver in `NM_Connecting` does `digitalWrite(LED_PIN, HIGH); digitalWrite(LED_PIN_B, !digitalRead(LED_PIN));` while nm_led writes red-blink via `writeRGB(((now_ms/500)&1)==0, false, false)` and steady blue for `NM_hashing`. The two race every ~200ms vs every-frame, producing flicker / wrong colors / indeterminate LED state — the visible HW-03 behavior is not what the summary asserts ("steady blue", "red ~1Hz"). This is a functional defect in the headline feature of the phase, and it is a cross-task GPIO race (both tasks issuing `digitalWrite` to the same pad without coordination).

**Fix:** Pick a single owner. Either (a) disable the stock `doLedStuff` RGB writes on the CYD env (guard the `switch (mMonitor.NerdStatus)` block in `esp32_2432S028R_DoLedStuff` so nm_led is the sole writer), or (b) drop nm_led's status writes and only do the green accepted-share flash, letting the stock driver own steady status. Example for (a):
```cpp
// esp23_2432s028r.cpp, inside esp32_2432S028R_DoLedStuff, before the switch:
#if !defined(NM_LED_OWNS_RGB)   // nm_led.cpp owns pins 4/16/17 on this build
    switch (mMonitor.NerdStatus) { ... existing LED writes ... }
#endif
```
and define `NM_LED_OWNS_RGB` for the CYD env. Do not leave two writers on the same pads.

## Warnings

### WR-01: `shares` is `volatile` at definition but declared non-volatile in three consumers — UB / inconsistent codegen

**File:** `src/mining.cpp:53` (`volatile uint32_t shares;`) vs `src/monitor.cpp:22` (`extern uint32_t shares;`), `src/NerdMinerV2.ino.cpp:302` (`extern uint32_t shares;` in `logShareEvent`), `src/NerdMinerV2.ino.cpp:267` (in `logCSVStats`)
**Issue:** The definition is `volatile uint32_t`. `webapi.cpp:26` and `nm_led.cpp:22` correctly redeclare it `extern volatile uint32_t`. But `monitor.cpp:22` and both `extern uint32_t shares;` lines in `NerdMinerV2.ino.cpp` (including the new `logShareEvent` at line 302) omit `volatile`. Declaring an object with a type that differs in the `volatile` qualifier from its definition is ill-formed; in practice the non-volatile reads may be cached/hoisted by the optimizer, so `logShareEvent`'s delta check (`cur == lastSharesLogged`) can miss updates written by the stratum task. The new code in this phase (`logShareEvent`) inherits this defect.
**Fix:** Make the `extern` in `logShareEvent` (and ideally all consumers) match the definition:
```cpp
extern volatile uint32_t shares;
```
A 32-bit aligned word is atomic on Xtensa, so this is sufficient for correctness once the qualifier matches; no lock needed.

### WR-02: torn 64-bit read of `upTime` across tasks

**File:** `src/NerdMinerV2.ino.cpp:303, 322` (`extern uint64_t upTime;` then `snprintf(... "%llu", upTime)`)
**Issue:** `upTime` (`mining.cpp:51`, `uint64_t`) is incremented on the mining/stat task (`mining.cpp:1251 upTime++`) and read in `logShareEvent` (and `logCSVStats`) on the main loop. A 64-bit read on the 32-bit Xtensa core is not atomic — it is two 32-bit loads. If the writer increments between the two loads (e.g., a low-word carry into the high word), the logged value can be momentarily corrupt. Low practical impact (a once-per-second increment, log-only), but it is a real torn-read.
**Fix:** Snapshot with interrupts/scheduler awareness is overkill for a log field; acceptable to leave with a comment, OR read into a local once and accept the cosmetic risk. If correctness in the log matters, copy under a brief `portENTER_CRITICAL`/`portEXIT_CRITICAL` or use an atomic load. Minimum: add a comment noting the accepted torn-read so a future reader does not assume the value is coherent.

### WR-03: `beepShareAccepted` now runs `Serial.printf` on the stratum task on every accepted share

**File:** `src/speaker.cpp:62` (`Serial.printf("[SPEAKER] Tone: ...")` inside `playTone`), reached from `mining.cpp:453`
**Issue:** `beepShareAccepted()` -> `playTone()` unconditionally does a `Serial.printf` of the tone params. This call is on the stratum task path (the seam at `mining.cpp:453`). `Serial.printf` can block on the UART TX buffer when the USB-CDC/serial host is slow or absent, introducing a variable stall on the share-accept path the phase was explicitly trying to keep clean (HW-04 "non-blocking on mining path"). The beep itself is non-blocking, but this logging is not guaranteed so. Note: `playShareBeep` had the same printf, so this is pre-existing — but the phase moved attention to this exact line and it remains a stratum-path blocking risk.
**Fix:** Gate the per-tone serial log behind a debug flag, or drop it. It is debug noise on a hot path:
```cpp
#ifdef SPEAKER_DEBUG
  Serial.printf("[SPEAKER] Tone: %d Hz for %d ms at %d%% volume\n", freq_hz, duration_ms, volume_percent);
#endif
```

### WR-04: `nm_led_tick` repaint gate `now_ms - s_last_repaint < REPAINT_INTERVAL_MS` is fine, but the share-flash arming uses `millis()` in one path and `now_ms` in another

**File:** `src/nm_led.cpp:54` (`nm_led_signal_share` uses `millis() + SHARE_FLASH_MS`) vs `src/nm_led.cpp:73` (`nm_led_tick` uses `now_ms + SHARE_FLASH_MS`, and compares `now_ms < s_share_flash_end`)
**Issue:** `nm_led_signal_share()` arms `s_share_flash_end` from `millis()`, but `nm_led_tick` compares it against its `now_ms` parameter. The caller passes `nm_led_tick(millis())`, so they currently agree — but this is an implicit coupling: if any caller ever passes a different time base to `nm_led_tick` (a monotonic offset, a test harness clock), the explicitly-armed flash would compare against the wrong clock and either never expire or expire instantly. The subtraction `now_ms - s_last_repaint` also wraps correctly only because both are unsigned `uint32_t` and `millis()` rollover (~49 days) yields a correct modular difference — that part is OK, but the dual time source is a latent inconsistency.
**Fix:** Have `nm_led_signal_share()` take no time and let `nm_led_tick` arm/expire purely against its own `now_ms`, or make `nm_led_signal_share(uint32_t now_ms)`. Single clock source for the module.

### WR-05: `secureClient.setInsecure()` called on a shared static `WiFiClientSecure` before every request — re-entrancy and stale-session risk

**File:** `src/monitor.cpp:40` (`static WiFiClientSecure secureClient;`) used at lines 71, 96, 136, 174, 454, 456
**Issue:** A single file-static `WiFiClientSecure` is reused across `updateGlobalData`, `getBlockHeight`, `getBTCprice`, and `getPoolData`. These run on the non-mining loop in sequence (per the loop wiring), so true concurrency is unlikely, but: (1) `HTTPClient http` is a fresh local each call and is bound to the same shared TLS client — if a prior `http.end()` did not fully release the underlying connection, a subsequent `http.begin(secureClient, ...)` can inherit a half-open TLS session (stale-session bug, observed in arduino-esp32 `WiFiClientSecure` reuse). (2) `setInsecure()` is re-invoked before each `begin` — redundant but harmless. The bigger correctness concern is reusing one TLS client object across four different hosts (mempool/coingecko/pool); the client's session/cert state is host-specific.
**Fix:** Prefer a local `WiFiClientSecure` per request (matches the local `HTTPClient` lifetime), or explicitly `secureClient.stop()` after `http.end()` to force a clean session per host. The static buys nothing here since each call already constructs a fresh `HTTPClient`.

## Info

### IN-01: TLS certificate validation disabled for all stock feeds (accepted carry-forward)

**File:** `src/monitor.cpp:71, 96, 136, 174, 454, 456`
**Issue:** `setInsecure()` skips cert/hostname validation for mempool/coingecko/pool GETs. These are public, read-only, non-authenticated APIs and the mined data is not security-sensitive, so MITM impact is limited to displaying wrong price/pool stats. This matches the documented existing project pattern (carry-forward) and is acceptable for this device class. Recorded as Info, not Warning, per the review brief.
**Fix:** No action required for v1. If hardening later: pin the API CA roots via `setCACert()`.

### IN-02: `http.end()` is on all returns in the pool-error paths — leak check passes

**File:** `src/monitor.cpp:497-512`
**Issue:** The reworked error and `catch(...)` paths both call `http.end()` before `return pData;`, and set `mPoolUpdate = millis()` to back off. No HTTPClient leak on the error path. The sentinel change from `"P"`/`"Error"` to `"0"` is a behavior change (downstream display now shows "0" instead of "P"/"Error") — verify the UI handles "0" as intended. Noted as informational.
**Fix:** None required; confirm the display layer treats `"0"` sentinels as "no data" rather than a literal zero hashrate.

### IN-03: `wManager.cpp` config-over-serial: `Brightness` parsed as raw int with no range clamp

**File:** `src/wManager.cpp:183` (`settings->Brightness = doc["Brightness"].as<int>();`)
**Issue:** `Brightness` from serial JSON is stored unclamped. If the display brightness setter expects 0-100 or 0-255, an out-of-range serial value is stored verbatim and clamped (or not) downstream. The adjacent `strncpy` calls correctly use `sizeof(field) - 1` (leaving room for the NUL that `strncpy` does not guarantee) — but note `strncpy` does NOT null-terminate when the source fills the buffer; the `- 1` only helps if the field was pre-zeroed. Low risk since these are existing fields.
**Fix:** Clamp at the boundary: `settings->Brightness = constrain(doc["Brightness"].as<int>(), 0, 100);` (use the actual valid range). For the `strncpy` lines, confirm the destination buffers are zero-initialized at struct creation; otherwise explicitly NUL-terminate.

### IN-04: `SD_FREQUENCY_MHZ` lowered 20 -> 4 with no in-code rationale

**File:** `include/sdstore.h:14`
**Issue:** The mount frequency dropped from 20 MHz to 4 MHz. The SUMMARY explains this is a conservative working-tree value, but the header has no comment. A future reader sees an unexplained 5x slowdown. Not a defect (4 MHz is safe), but undocumented.
**Fix:** Add a one-line comment: `// 4 MHz: conservative for long VSPI wiring / cheap CYD SD slots (was 20)`.

---

_Reviewed: 2026-06-18_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
