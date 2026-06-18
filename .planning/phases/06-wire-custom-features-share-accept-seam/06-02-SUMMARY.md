---
phase: 06-wire-custom-features-share-accept-seam
plan: 02
subsystem: firmware-custom-hw
tags: [sd, vspi, share-accept-seam, hw-02, hw-04, build-gate, flash-budget]
requires:
  - "SDStore on a dedicated SPIClass vspi(VSPI), mounted at boot (.ino:167) — pre-existing fork driver (D-61)"
  - "appendCSVLog(const char*) — existing rotation-bounded CSV writer (reused, D-61)"
  - "shares counter (mining.cpp, extern uint32_t) — accepted-share signal, read-only"
  - "06-01 seam swap (Speaker::beepShareAccepted at mining.cpp:453) and nm_led module already committed"
provides:
  - "logShareEvent() — loop-driven, isReady()-gated at-event accepted-share SD log (HW-02 'logs')"
  - "Green full CYD build (pio run -e ESP32-2432S028R) with recorded flash budget"
  - "HW-04 evidence: whole-phase mining.cpp diff is exactly the one beepShareAccepted seam line"
affects:
  - "src/NerdMinerV2.ino.cpp loop() — logShareEvent() call next to logCSVStats()"
  - "include/sdstore.h — SD_FREQUENCY_MHZ kept at the conservative 4 MHz"
tech-stack:
  added: []
  patterns:
    - "Loop-driven event log keyed off a counter delta (no new mining.cpp hook — HW-04)"
    - "Graceful-when-absent SD I/O: isReady()-gate returns before any file op (D-63)"
    - "Reuse existing appendCSVLog with a distinct tag column instead of a new SDStore method (D-61)"
key-files:
  created: []
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/include/sdstore.h
    - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
decisions:
  - "Accepted-share SD row tagged with a leading 'event=share' column to distinguish it from the periodic 'timestamp,hashrate,shares,uptime' stats row — both written via the same appendCSVLog (no new SDStore method, D-61)."
  - "logShareEvent keys off a static last-seen shares count delta, NOT a mining.cpp hook, so HW-04 stays intact."
  - "SD_FREQUENCY_MHZ kept at 4 (working-tree conservative value, was 20) as the VSPI mount frequency."
  - "Task 2 made no source change — the build passed clean, so there is no Task 2 fork commit."
metrics:
  duration: "~10m"
  completed: "2026-06-18"
  tasks: 2
  files: 2
---

# Phase 6 Plan 02: SD At-Event Log + Phase Build/Diff Gate Summary

Closed HW-02 by adding a graceful, loop-driven accepted-share SD log (`logShareEvent()`) that reuses the existing `appendCSVLog` and is `isReady()`-gated (no card = no log, mining unaffected), then ran the phase gate: the full CYD build is green (90.7% flash, 9.3% headroom) and the whole-phase `mining.cpp` diff is exactly the one `beepShareAccepted` seam line (HW-04 / D-65 proven).

## What Was Built

### Task 1 — at-event accepted-share SD log (HW-02 "logs")
- `src/NerdMinerV2.ino.cpp`: added `logShareEvent()` immediately after the existing `logCSVStats()`. It:
  - reads `extern uint32_t shares;` (the same counter the periodic stats writer uses) and `extern uint64_t upTime;`
  - returns immediately if `!sdStore.isReady()` — **graceful with no card, mining unaffected** (D-63 / T-06-SDABSENT)
  - keeps a `static uint32_t lastSharesLogged`; only writes when the counter advanced (one row per accepted share, no spam — T-06-SDLOG)
  - appends a **distinct** row via the existing `sdStore.appendCSVLog(...)`: `event=share,<ts>,<shares>,<uptime>` — the leading `event=share` tag column distinguishes it from the periodic `timestamp,hashrate,shares,uptime` stats row
  - is **non-blocking**: the only I/O is `appendCSVLog` (already rotation-bounded at `SD_CSV_MAX_SIZE`); no `delay`/`vTaskDelay`
  - **no new SDStore method** added — `appendCSVLog` takes an arbitrary CSV row, so it expresses the event row directly (D-61: reuse, don't add)
- Call wired in `loop()` next to `logCSVStats();` (same env-agnostic position the periodic writer uses — both gate internally on `isReady()`).
- `include/sdstore.h`: confirmed and kept `SD_FREQUENCY_MHZ 4` (conservative VSPI mount frequency; the working tree had already changed 20→4). Committed with Task 1 so the value is captured.
- **No mining.cpp hook for the SD log** — derived purely from the loop/counter delta (HW-04).

### Task 2 — phase build gate + mining.cpp diff gate (HW-04 / D-65, flash budget)
- Full integration build: `pio run -e ESP32-2432S028R` → **SUCCESS** (58.3s), no `error:`.
- **Flash budget:**
  - `firmware.bin` = **2,852,992 B**
  - vs the **3,145,728 B** huge_app partition → **90.7% used**, **292,736 B / 9.3% headroom**
  - Baseline (Phase-5 05-VALIDATION.md) was 2,852,288 B / 90.5% / 9.5%. The SD-log hook added only **704 B** to the bin (linker-reported app flash 2,846,693 B / 90.5%). Well within budget — no HW requirement dropped.
- **HW-04 mining-sacrosanct diff gate:** the entire-phase `git diff <phase-base>..HEAD -- src/mining.cpp` is exactly the single seam line. No change at the disconnect-alarm line (mining.cpp:263), no `shares++` change, no stratum/hashing/pool change.
- This task made **no source change** (build passed clean), so there is no Task 2 fork commit (per the plan: "Task 2 makes NO source change unless the build fails").

## HW-04 evidence — whole-phase mining.cpp diff (verbatim)

phase-base = `8b9814d444123eeecc1b607d865670278e6f76cc` (`/tmp/nmm-phase6-base-sha`, fork HEAD before 06-01)

```diff
diff --git a/src/mining.cpp b/src/mining.cpp
index 96d2897..5e9e694 100644
--- a/src/mining.cpp
+++ b/src/mining.cpp
@@ -450,7 +450,7 @@ void runStratumWorker(void *name) {
                                             best_diff = itt->second->diff;
                                           if (itt->second->is32bit) {
                                             shares++;
-                                            Speaker::playShareBeep();
+                                            Speaker::beepShareAccepted();
                                           }
                                           if (itt->second->isValid)
                                           {
```

That is the complete diff — one removed line, one added line, both the seam call. HW-04 satisfied.

## SD log row format chosen
- **Periodic stats row** (existing, `logCSVStats`): `<ts>,<hashes>,<shares>,<uptime>`
- **Accepted-share event row** (new, `logShareEvent`): `event=share,<ts>,<shares>,<uptime>`

Both append to `SD_CSV_PATH` (`/sd/mining_stats.csv`) via `appendCSVLog`, sharing the same 1 MB rotation. The `event=share` leading token makes event rows trivially filterable from periodic rows.

## Deviations from Plan
None. Plan executed as written. The pre-existing, out-of-scope working-tree changes in the fork (`src/monitor.cpp` TLS/WiFiClientSecure, `src/wManager.cpp` config-over-serial additions) were present before this plan and are **not** part of plan 06-02's file set — they were left untouched and unstaged (scope boundary). Untracked build artifacts (`*.d`, `branding/logo_test.png`) were likewise left alone.

## Flash budget note
firmware.bin grew 2,852,288 → 2,852,992 B (+704 B) across the whole phase (beep rename + nm_led module + SD-log hook). Headroom 9.3% (292,736 B). No overflow; no HW requirement at risk.

## Verification Results
- Task 1: `SDLOG_OK` — `logShareEvent` exists, isReady()-gated, uses `appendCSVLog`; `SD_FREQUENCY_MHZ 4` present.
- Task 2: `BUILD_DIFF_OK` — `pio run` SUCCESS + firmware.bin present + mining.cpp diff == single seam line.
- Full 06-VALIDATION.md gate suite re-run against the integrated state: HW-01 PASS, HW-01b PASS, HW-02 PASS, HW-03 PASS, HW-04 PASS.
- sdstore.cpp unchanged (no new SDStore method); mining.cpp untouched by this plan's edits.

## Commits (fork repo: firmware/custom/NerdMiner-CYD-Custom)
- `8ca17f6`: feat(06-02): add graceful at-event accepted-share SD log (HW-02)
- (no Task 2 commit — build passed clean, no source change)

## Self-Check: PASSED
