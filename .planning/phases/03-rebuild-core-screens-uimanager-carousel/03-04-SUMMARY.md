---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 04
subsystem: ui
tags: [lvgl, miner-screen, fontawesome, recovered-fonts, palette, cyd, esp32-2432s028r, ui-03, ui-06]
requires:
  - "Plan 03-02: nm_screen_t contract + NM_PAGE_MINER enum + UIManager page registry"
  - "Plan 02-02: recovered_fonts.h (fa_nmminer_subset + Montserrat) + nmminer_palette.h"
  - "SPEC_Miner.md: 23-label Miner320x240 geometry/font/color table"
provides:
  - "const nm_screen_t nm_screen_miner = { \"Miner320x240\", build, update, destroy }"
  - "Miner screen: 23 SPEC widgets at exact coords; update() drives live miner stats"
affects:
  - "Plan 03-06 (final link/build once all 6 descriptors exist)"
  - "Phase 5 data-driven screens (will wire network diff / pool diff placeholders to real sources)"
tech-stack:
  added: []
  patterns:
    - "mk_label() positioned-label factory (x/y/align/width/font/color/text) — one helper, 23 call sites"
    - "file-static miner_widgets_t holds only the dynamic handles update() refreshes; static icons/labels are fire-and-forget"
    - "getMiningData(0) as the per-tick live miner-stat source (no reset of averaging window)"
    - "env-guarded whole file (#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB))"
key-files:
  created:
    - "firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_miner.cpp"
  modified: []
decisions:
  - "Live miner-stat source is getMiningData() returning mining_data (the fork's per-tick struct), NOT a fabricated mMonitor/pData object — the plan's pattern hint named mMonitor/pData but the fork exposes getMiningData; used the real API"
  - "network diff (+0x2c) / pool diff (+0x34) honestly placeholdered '--' — they are global_data/pool_data fields fetched on a slower network cadence, not in the per-tick mining_data; wiring deferred to Phase 5 (data-driven), not faked"
  - "RSSI from WiFi.RSSI() and pool URL from getPoolAPIUrl() — both real fork APIs, used directly since they are not in mining_data"
  - "Font mapping per recovered_fonts.h descriptor table: big->montserrat_26, medium->montserrat_16, value->montserrat_22, status->montserrat_14, icons->fa_nmminer_subset (all enabled in src/ui_nm/lv_conf.h)"
  - "firmware version label uses the fork's CURRENT_VERSION macro (V1.8.3), not the stock literal 'v2.0.02' — this is the running firmware's own version, matching the SPEC slot's semantic (fw version) rather than copying the stock binary's string"
metrics:
  duration: "~5 min"
  tasks_completed: 2
  files_created: 1
  files_modified: 0
  completed: 2026-06-18
---

# Phase 3 Plan 04: Miner320x240 Screen Summary

The CYD's primary stats screen is now a native LVGL v8 rebuild of stock NMMiner v2.0.02's `Miner320x240`, generated from `SPEC_Miner.md`'s 23-label geometry table. `const nm_screen_t nm_screen_miner` fills the Wave-2 contract: `build()` lazily lays out all 23 widgets at exact SPEC coordinates (hashrate row, four FontAwesome-icon + value stat rows, firmware version, wide status/pool line, bottom temp/RSSI/uptime row, and the trailing `H/s` tag) using the recovered FontAwesome subset, built-in Montserrat, and the stock palette; `update()` pushes live miner stats from the fork's per-tick `getMiningData()` source; `destroy()` frees the tree.

## What Was Built

### `nm_screen_miner` descriptor + SPEC layout (`d056a91`)
- `src/ui_nm/nm_screen_miner.cpp` (196 lines): `const nm_screen_t nm_screen_miner = { "Miner320x240", miner_build, miner_update, miner_destroy };`.
- `miner_build()` creates `lv_obj_create(NULL)` (black bg, non-scrollable) and adds **23 labels** via a `mk_label(parent, x, y, align, width, font, color, text)` factory — one per SPEC table row:
  - **hashrate cluster** (y~3): big value `+0x04` (TOP_MID, montserrat_26, white, full 320w), medium helper/sub/extra cells `+0x08/+0x10/+0x18/+0x1c`, orange `"d"` unit `+0x0c` and orange wifi glyph `+0x14`.
  - **4 middle stat rows** (gray FA icon @ x153 + white value @ x185 TOP_MID, montserrat_22), y = 30/60/88/115: download→shares `+0x20/+0x24`, globe→netDiff `+0x28/+0x2c`, chart→poolDiff `+0x30/+0x34`, bestdiff glyph→bestDiff `+0x38/+0x3c`.
  - **left/mid orange accents** `+0x40` (17,63) and `+0x44` (165,157).
  - **firmware version** `+0x48` (11,123) light-gray, `CURRENT_VERSION`.
  - **wide status/pool line** `+0x4c` (18,175, 250w, montserrat_14, white).
  - **bottom row** y215: temp `+0x50` (3,215,130w) / RSSI `+0x58` (145,215,80w) / uptime `+0x54` (237,215,58w), all montserrat_22 white.
  - **static `"H/s"` tag** (290,218) — fire-and-forget, not stored.
- Dynamic handles stored in a file-static `miner_widgets_t` for `update()`. SPEC align enum honored (1=TOP_LEFT, 2=TOP_MID).

### Live `update()` + `destroy()` (same commit)
- `miner_update()` reads `getMiningData(0)` (`mining_data`) and sets:
  - `hr_big` ← `currentHashRate`, `hr_sub` ← `totalMHashes`
  - `v_shares` ← `completedShares` (+0x24), `v_bestdiff` ← `bestDiff` (+0x3c)
  - `v_temp` ← `"%sC"` of `temp` (+0x50), `v_uptime` ← `timeMining` (+0x54)
  - `v_rssi` ← `WiFi.RSSI()` (`"%lddBm"`, or `--` if not connected) (+0x58)
  - `v_status` ← `getPoolAPIUrl()` (+0x4c)
  - `v_netdiff` / `v_pooldiff` ← `"--"` placeholder (see Known Stubs)
- All `lv_label_set_text(_fmt)` on existing labels: O(slots), no allocation, no network (T-03-10 accepted).
- `miner_destroy()` `lv_obj_del(scr)` and zeroes the static struct.

## Stat Field Wiring (live vs placeholder)

| SPEC slot | stat | source | status |
|-----------|------|--------|--------|
| +0x04/+0x10 | hashrate value + sub | `getMiningData().currentHashRate` / `.totalMHashes` | **live** |
| +0x24 | shares accepted | `.completedShares` | **live** |
| +0x3c | best diff (session) | `.bestDiff` (suffix_string) | **live** |
| +0x50 | temperature | `.temp` + `"C"` | **live** |
| +0x54 | uptime | `.timeMining` | **live** |
| +0x58 | RSSI | `WiFi.RSSI()` | **live** |
| +0x4c | pool URL / status | `getPoolAPIUrl()` | **live** |
| +0x2c | network diff | (global_data, slow cadence) | **placeholder `--`** |
| +0x34 | pool diff | (pool_data, slow cadence) | **placeholder `--`** |

7 of 9 dynamic stat slots are wired to live data; 2 (network diff, pool diff) are honest placeholders.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Live stat source is getMiningData(), not mMonitor/pData**
- **Found during:** Task 2
- **Issue:** The plan's `key_links` pattern hinted `update()` reads `mMonitor`/`pData`, but `src/monitor.h` exposes no such objects — the fork's per-tick miner stats come from `mining_data getMiningData(unsigned long mElapsed)`.
- **Fix:** Used the real fork API `getMiningData(0)` (mElapsed=0 reads live counters without resetting the hashrate averaging window). Did not invent a monitor object.
- **Files modified:** `src/ui_nm/nm_screen_miner.cpp`
- **Commit:** `d056a91`

**2. [Rule 2 - Honest placeholdering] network diff / pool diff not faked**
- **Found during:** Task 2
- **Issue:** SPEC slots +0x2c (netDiff) and +0x34 (poolDiff) have no field in the per-tick `mining_data` struct — they live in `global_data` (`getCoinData`/`getGlobalHash`) and `pool_data` (`getPoolData`), fetched on a slower (1-2 min) network cadence unsuitable for a per-tick UI `update()`.
- **Fix:** Seeded `"--"` rather than fabricating a per-tick source or doing blocking network I/O in `update()` (would violate T-03-10). Wiring to the slow-cadence sources is Phase 5 (DATA-* data-driven screens).
- **Files modified:** `src/ui_nm/nm_screen_miner.cpp`
- **Commit:** `d056a91`

## Threat Surface
Plan `<threat_model>` mitigations applied:
- **T-03-08 (Info disclosure, recovered geometry):** only numeric coords/font/color facts in source; no closed bytes. SPEC stays gitignored.
- **T-03-09 (Tampering, format strings):** all `lv_label_set_text_fmt` use fixed literal format strings (`"%sC"`, `"%lddBm"`) with matching arg types; no user-controlled format string.
- **T-03-10 (DoS, per-tick update):** `update()` is `lv_label_set_text(_fmt)` on existing labels only — no allocation, no network; accepted-and-bounded.

No new threat surface beyond the plan's register.

## Known Stubs

| Stub | File | Reason |
|------|------|--------|
| `v_netdiff` = `"--"` (+0x2c network diff) | `src/ui_nm/nm_screen_miner.cpp` miner_update() | Not in per-tick `mining_data`; lives in `global_data` (slow network cadence). Resolved in Phase 5 (DATA-* data-driven wiring). |
| `v_pooldiff` = `"--"` (+0x34 pool diff) | `src/ui_nm/nm_screen_miner.cpp` miner_update() | Not in per-tick `mining_data`; lives in `pool_data` (slow network cadence). Resolved in Phase 5. |

These do not block UI-03's goal (the screen renders the full SPEC layout and drives 7/9 stat slots live); they are intentional and scoped to the data-driven phase.

## Notes for Next Plans
- `nm_screen_miner` is defined and matches the contract; Plan 03-06 final link can reference it once clock/sprice/weather/loading/config descriptors also exist.
- Phase 5: wire `v_netdiff`/`v_pooldiff` from `getCoinData()`/`getPoolData()` cached values (do not block `update()` on network).
- Structural compile only this plan (per plan); full `pio run` SUCCESS is the 03-06 gate.
- Parent gitlink bump for `firmware/custom/NerdMiner-CYD-Custom` is the orchestrator's job; the task commit `d056a91` lives in the nested fork repo.

## Self-Check: PASSED
- `src/ui_nm/nm_screen_miner.cpp` present on disk in fork; SUMMARY.md created in parent `.planning`.
- Commit `d056a91` confirmed in fork git log.
- Grep gates (both tasks) pass; 23 mk_label widgets = full SPEC table; brace/paren balanced; all referenced symbols (getMiningData, getPoolAPIUrl, mining_data fields, CURRENT_VERSION, palette/font macros) exist.
