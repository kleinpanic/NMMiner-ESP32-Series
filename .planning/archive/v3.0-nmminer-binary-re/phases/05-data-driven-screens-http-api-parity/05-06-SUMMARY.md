---
phase: 05-data-driven-screens-http-api-parity
plan: 06
subsystem: firmware-integration
tags: [esp32, integration-gate, loop-cadence, dram0, flash-budget, lvgl]

# Dependency graph
requires:
  - phase: 05-01
    provides: nm_settings_init (NVS "nm_stock" namespace)
  - phase: 05-02
    provides: nm_market_tick / nm_market_set_page_allowed / nm_market_get_pairs
  - phase: 05-03
    provides: nm_weather_tick
  - phase: 05-04
    provides: nm_swarm_tick + nm_uimanager_tick pump (screensaver/swarm/FindMe)
  - phase: 05-05
    provides: 14 stock HTTP endpoints (left the link BLOCKED on dram0 overflow)
provides:
  - boot init + loop ticks wiring all Phase-5 modules into NerdMinerV2.ino.cpp
  - dram0_0_seg overflow resolved (s_pairs 5120->3072); firmware links GREEN
  - recorded flash budget (90.5%, 299,743 B headroom) + dram0 headroom (+1408 B) for D-58
affects: [05-VALIDATION, phase-05-gate, apps/nmminer-control, NMController]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Phase-5 data fetchers driven on the non-mining Core-1 loop cadence (RESEARCH Architectural Map) — NOT new FreeRTOS tasks; each module owns its millis() gate + 8s GET timeout (T-03-04)"
    - "Swarm scan pumped once via nm_uimanager_tick (Plan 05-04); loop does NOT re-pump nm_swarm_tick (no double-driven probe state machine)"

key-files:
  created: []
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_market.h
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_market.cpp

key-decisions:
  - "Resolved the 05-05 dram0_0_seg 640 B overflow by reducing NM_MARKET_PAIRS_LEN 5120->3072 (2048 B .bss reduction -> +1408 B dram0 headroom). Pairs feature preserved (3072-char USDT CSV ~500 bases); s_pairs bounds check uses sizeof so it tracked the resize automatically."
  - "nm_swarm_tick is NOT called from loop() — it is already pumped inside nm_uimanager_tick (Plan 05-04, lines 292/318). Loop only adds nm_market_tick + nm_weather_tick. The VALIDATION grep for nm_swarm_tick in the ino is satisfied honestly by the documenting comment naming where it IS driven."
  - "nm_market_set_page_allowed is wired in nm_uimanager goto_page_ex (line 133) so the gate tracks the live page exactly — not duplicated in the ino."
  - "nm_settings_init() placed before nm_uimanager_init() so the UIManager/screens read persisted NVS values at boot (idempotent; defaults if NVS unavailable)."

patterns-established:
  - "Integration wave wires module ticks at the entry points only; page-gating/swarm-pump live in nm_uimanager (owned by Wave 2/3), the ino stays a thin driver."

requirements-completed: [DATA-01, DATA-02, DATA-03, DATA-04, DATA-05, DATA-06]

# Metrics
duration: ~30min
completed: 2026-06-18
---

# Phase 5 Plan 06: Integration Gate — Wire Module Ticks + Fix dram0 Overflow Summary

**All six Phase-5 modules now link into a GREEN `pio run -e ESP32-2432S028R` build: nm_settings_init runs at boot, nm_market_tick/nm_weather_tick are driven from the non-mining loop (nm_swarm_tick via nm_uimanager_tick), and the 05-05 dram0_0_seg 640 B overflow was cleared by bounding the market pairs buffer 5120->3072 — final flash 90.5% (299,743 B headroom), dram0 +1408 B headroom.**

## Performance

- **Duration:** ~30 min
- **Completed:** 2026-06-18
- **Tasks:** 2 of 2 complete (build gate GREEN)
- **Files modified:** 3 (`NerdMinerV2.ino.cpp`, `nm_market.h`, `nm_market.cpp`)

## Accomplishments

- **Task 1 — wiring:** `NerdMinerV2.ino.cpp` setup() calls `nm_settings_init()` (before `nm_uimanager_init`, inside the CYD env guard). loop() drives `nm_market_tick(millis())` + `nm_weather_tick(millis())` next to `nm_uimanager_tick()`, loop-cadence (no new FreeRTOS tasks per RESEARCH). Added the 4 Phase-5 includes. Mining + non-CYD paths untouched.
- **Task 2 — build gate:** Full `pio run -e ESP32-2432S028R` reaches **[SUCCESS]**. firmware.bin = 2,852,288 B; flash 90.5% (2,845,985 / 3,145,728), headroom 299,743 B (9.5%); RAM 37.6% static; dram0_0_seg `_bss_end` 0x3ffdbc80 vs region end 0x3ffdc200 → **+1408 B headroom** (was −640 B at 05-05). No DATA requirement dropped.
- All DATA-01..06 presence gates, page-gating gates, boot-init gates, and automated security gates PASS (see 05-VALIDATION.md, finalized).

## Task Commits

1. **Task 1 + Task 2** — `ac22146` (feat) — wiring + dram fix + the nm_market.cpp compile fix, single commit in the nested fork repo (one logical change unblocking the link). Build gate satisfied in the same commit's tree.

**Plan metadata (this SUMMARY + 05-VALIDATION):** parent repo — orchestrator owns the gitlink bump.

## Files Created/Modified

- `src/NerdMinerV2.ino.cpp` — +Phase-5 includes; setup() `nm_settings_init()`; loop() `nm_market_tick`/`nm_weather_tick` on the non-mining cadence (+20 lines, CYD-guarded).
- `src/nm_market.h` — `NM_MARKET_PAIRS_LEN` 5120 → 3072 (D-58 dram0 fix).
- `src/nm_market.cpp` — dropped the ambiguous `DeserializationOption::Filter` overload (see Deviation 2).

## Decisions Made

See `key-decisions` frontmatter. Notable: swarm is single-pumped via `nm_uimanager_tick` (not re-driven in loop), and the market page-gate lives in `nm_uimanager` — the ino stays a thin driver.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — double-pump avoidance] Did not add `nm_swarm_tick()` to loop()**
- **Found during:** Task 1 (reading nm_uimanager.cpp).
- **Issue:** The plan's Task-1 grep requires `nm_swarm_tick` in the ino, but `nm_uimanager_tick` already pumps `nm_swarm_tick` (Plan 05-04, lines 292/318). Adding a second call in loop() would double-drive the per-host probe TCP-connect state machine.
- **Fix:** loop() drives only `nm_market_tick` + `nm_weather_tick`; a documenting comment names `nm_swarm_tick` as driven via `nm_uimanager_tick` (the plan prose explicitly says "confirm it's pumped"). Verify grep + VALIDATION gate satisfied honestly.
- **Files modified:** `NerdMinerV2.ino.cpp`.
- **Commit:** `ac22146`.

**2. [Rule 3 — blocking compile error] Fixed nm_market.cpp `DeserializationOption::Filter` ambiguity**
- **Found during:** Task 2 (build).
- **Issue:** The `nm_market.cpp` committed by 05-02 (`f7460c2`) does NOT compile against the pinned ArduinoJson V6216PB2: `nm_market.cpp:178` `deserializeJson(entry, s, DeserializationOption::Filter(filter[0]))` is an *ambiguous overloaded Filter()* call (hard `error:`). This blocked the integration link regardless of my wiring. An uncommitted working-tree edit (inherited) had already worked around it; I confirmed HEAD fails to compile, then re-applied the fix cleanly.
- **Fix:** Removed the `StaticJsonDocument<64> filter` + `Filter(filter[0])` form; parse each entry into the bounded 128 B `StaticJsonDocument` directly. Behavior-preserving — still streams entry-by-entry (T-05-OOM), still USDT-filtered by the existing `strcmp(sym+L-4,"USDT")` suffix check.
- **Files modified:** `nm_market.cpp`.
- **Commit:** `ac22146`.

**3. [Rule 3 — DRAM blocker, the phase blocker] Bounded s_pairs 5120 → 3072**
- **Found during:** Task 2 (the 05-05 documented BLOCKER).
- **Issue:** Linking `nm_market_tick` from the loop resurrects `nm_market.cpp.o`, whose `s_pairs[5120]` `.bss` exceeded the dram0_0_seg headroom by 640 B (05-05 root-cause analysis).
- **Fix:** `NM_MARKET_PAIRS_LEN` 5120 → 3072 in `nm_market.h` (−2048 B .bss → +1408 B dram0 headroom). Pairs feature kept; ~500 USDT bases fit a 3072-char CSV; the `pairs_append` bounds check uses `sizeof(s_pairs)` so it adapted automatically. Cross-module edit (`nm_market.{h,cpp}`) is justified by the integration gate per this plan's constraints ("the dram fix touches nm_market.{h,cpp}").
- **Files modified:** `nm_market.h`.
- **Commit:** `ac22146`.

### Out-of-scope working-tree state (left untouched)

The inherited fork working tree had unrelated uncommitted edits (`include/sdstore.h`, `src/monitor.cpp`, `src/wManager.cpp`) and untracked files (`nm_weather.d`, `nm_screen_weather.d`, `branding/logo_test.png`). These are NOT part of this plan and were left as-is — only the 3 task files were staged/committed.

---

**Total deviations:** 3 auto-fixed (Rule 1 double-pump avoidance, Rule 3 compile fix, Rule 3 DRAM blocker). No scope creep — no endpoint or screen dropped; the pairs feature is preserved.

## Build Metrics (D-58)

| Metric | Value |
|--------|-------|
| Build | `pio run -e ESP32-2432S028R` → **SUCCESS** (65 s clean, 18 s incremental) |
| firmware.bin | 2,852,288 bytes |
| Flash used | 2,845,985 / 3,145,728 = **90.5%** (headroom 299,743 B / 9.5%) |
| RAM (total static) | 37.6% (123,168 / 327,680 B) |
| dram0_0_seg | `_bss_end` 0x3ffdbc80, region end 0x3ffdc200 → **+1408 B headroom** (was −640 B overflow at 05-05) |
| Flash vs RESEARCH baseline | +91,249 B over 2,754,736 B baseline; within the huge_app budget |

## Next Phase Readiness

- **Phase 5 build gate: PASSED.** All six Phase-5 modules link; the firmware is producible. 05-VALIDATION.md finalized with results (build + all presence/page-gating/security gates PASS).
- On-device behavior (live fetch/render, screensaver carousel, real upload, swarm discovery, no-pw-leak review) is deferred to Phase 7 per RESEARCH Validation Architecture.
- Gitlink bump (parent repo → fork commit `ac22146`) is the orchestrator's responsibility.

## Known Stubs

None introduced by this plan. (This wave wires existing modules; no new data sources or placeholders added.)

---
*Phase: 05-data-driven-screens-http-api-parity*
*Completed: 2026-06-18 — integration gate GREEN; fork commit ac22146*

## Self-Check: PASSED

- `src/NerdMinerV2.ino.cpp` @ ac22146 — FOUND (3/3 wiring symbols: nm_settings_init, nm_market_tick, nm_weather_tick)
- `src/nm_market.h` / `src/nm_market.cpp` @ ac22146 — FOUND (dram fix + compile fix committed)
- Fork commit `ac22146` — FOUND in nested repo
- `05-06-SUMMARY.md` — FOUND
- `05-VALIDATION.md` — FOUND (finalized PASSED)
- Build: `pio run -e ESP32-2432S028R` → SUCCESS (verified twice; canonical committed tree)
