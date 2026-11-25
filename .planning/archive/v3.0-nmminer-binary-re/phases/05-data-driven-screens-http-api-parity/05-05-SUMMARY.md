---
phase: 05-data-driven-screens-http-api-parity
plan: 05
subsystem: api
tags: [esp32, webserver, http-api, nmminer-parity, littlefs, multipart-upload, branding]

# Dependency graph
requires:
  - phase: 05-01
    provides: nm_settings bounded get/put accessors (NVS "nm_stock" namespace)
  - phase: 05-02
    provides: nm_market_get_pairs / nm_market_tick (USDT pairs CSV)
  - phase: 05-03
    provides: nm_weather_request_refresh (60s-rate-limited forced fetch)
  - phase: 05-04
    provides: nm_swarm_note_alive / get_list / request_findme
provides:
  - 14 stock-parity HTTP endpoints added additively to the fork apiServer
  - /probe + full /api/system/info with model/hwModel="NMMiner" branding (D-57)
  - /api/setting/{network,mining,time,preference,market,weather} GET+POST
  - /api/market/pairs, /api/weather/refresh (429), /api/swarm/find, /api/system/restart
  - /api/update/screensaver{,/preflight} with magic+size+path validation
affects: [05-06, apps/nmminer-control, NMController]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "WebServer 4-arg on(path, method, doneFn, uploadFn) multipart upload (RESEARCH Pattern 9 [ASSUMED] VERIFIED against WebServer.h:90)"
    - "Partial-JSON settings POST with isNull() skip + boundary range checks"
    - "Firmware-computed upload path (no client filename) to defeat traversal"

key-files:
  created: []
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp

key-decisions:
  - "Routed stock-named fields (market/weather/ScreenSaver/TimeFormat/DateFormat/RotateScreen/LedEnable) through nm_settings_put_* bounded setters; routed fork-native fields (WifiSSID/WifiPW/pool/wallet/Timezone/Brightness) through the TSettings struct + nvMemory::saveConfig (config.json)."
  - "Upload filename is firmware-computed /ss/ss_saver_320_240_NNN.ss (NOT the stock saver_320_240_NNN.ss) because the fork carousel (nm_screensaver.cpp) only enumerates ss_saver_320_240_* — uploads must match that prefix to be displayed."
  - "Magic check uses buf[0]==0x53 && buf[1]==0x4E ('SN'), matching nm_screensaver.cpp's decoder (hdr[0]!=0x53||hdr[1]!=0x4E), i.e. little-endian 0x4E53."
  - "Upload state heap-allocated (new/delete) rather than file-scope static to keep File+String off the saturated dram0_0_seg (D-58)."

patterns-established:
  - "Stock-parity additive API: keep every existing route, add the stock set, replace only the /api/system/info alias with the full stock payload while preserving the fork's sd/speaker fields inside it."

requirements-completed: [DATA-04, DATA-05, DATA-06]

# Metrics
duration: ~75min
completed: 2026-06-18
---

# Phase 5 Plan 05: Stock HTTP API Parity (webapi.cpp) Summary

**14 stock NMMiner endpoints (probe/info branding, six settings GET+POST, market pairs, weather refresh, swarm alive/find, restart, screensaver upload+preflight) added additively to the fork apiServer with magic/size/path-validated .ss upload — code complete and compiling, but firmware link is BLOCKED by a cross-module dram0 overflow (D-58).**

## Performance

- **Duration:** ~75 min
- **Started:** 2026-06-18T06:46:50Z
- **Completed:** 2026-06-18T07:00:00Z (approx)
- **Tasks:** 1 of 2 fully complete; Task 2 code complete, build BLOCKED at link
- **Files modified:** 1 (`src/webapi.cpp`, +529/-1)

## Accomplishments

- All 14 stock-parity endpoints implemented and registered additively; the 9 pre-existing routes (/api/status, /api/speaker/*, /api/sd/*) are preserved unchanged, plus the LAN-trust CORS comment.
- Branding (D-57/DATA-06): `/probe` and `/api/system/info` report `model`/`hwModel` = `"NMMiner"` and the `"Make it better"` tagline; `ver` = fork `CURRENT_VERSION`.
- Settings (D-55): six `/api/setting/*` endpoints with GET serializers + partial-JSON POST writers; secrets (WiFiPWD / pool passwords) are never serialized in GET; ranges validated at the boundary; market POST triggers `nm_market_tick`, weather POST triggers `nm_weather_request_refresh`.
- Swarm (D-56/DATA-04): `/alive` returns `{self, ips[]}` (dedup, wakes scan via `nm_swarm_note_alive`); `/api/swarm/find` raises FindMe via `nm_swarm_request_findme`.
- Screensaver upload: WebServer 4-arg `onFileUpload`; validates magic `0x53 0x4E` and size ≤200KB (413) before any LittleFS write; firmware-computed `/ss/ss_saver_320_240_NNN.ss` path; preflight bounds `?size=` and reports LittleFS free space.
- `webapi.cpp.o` compiles clean — the only build failure is the link-stage dram0 overflow described below.

## Task Commits

1. **Task 1 + Task 2 (code): stock endpoint contract** - `63f3957` (feat) — both tasks' code landed in one commit since they edit the same file and Task 2's build gate could not be satisfied (see Blocked).

**Plan metadata:** pending (this SUMMARY + STATE) — see final commit.

## Files Created/Modified

- `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp` — +14 stock endpoints, branding, settings GET/POST, market/weather/swarm/restart actions, screensaver upload+preflight. Additive to existing speaker/sd/status routes.

## Decisions Made

See `key-decisions` frontmatter. Notable: the upload filename prefix is `ss_saver_320_240_` (fork carousel convention), not the stock `saver_320_240_`, so uploaded screensavers are actually enumerated and displayed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - API mismatch] Plan named `nm_uimanager_trigger_findme`; actual API is `nm_swarm_request_findme`**
- **Found during:** Task 2
- **Issue:** The plan prose referenced `nm_uimanager_trigger_findme` for /api/swarm/find. The 05-04 module exposes the FindMe trigger as `nm_swarm_request_findme()` (nm_swarm.h), consumed by the UIManager via `nm_swarm_findme_pending()`.
- **Fix:** /api/swarm/find calls `nm_swarm_request_findme()`.
- **Verification:** Symbol resolves in webapi.cpp.o undefined refs; compiles clean.
- **Committed in:** 63f3957

**2. [Rule 1 - filename convention] Upload path uses the fork carousel prefix, not the stock prefix**
- **Found during:** Task 2
- **Issue:** Stock API doc specifies `saver_320_240_NNN.ss`, but the fork carousel (nm_screensaver.cpp) only enumerates `ss_saver_320_240_*.ss`. Writing the stock name would store a file the device never displays.
- **Fix:** Firmware-computed path `/ss/ss_saver_320_240_NNN.ss`.
- **Verification:** Matches `strncmp(base, "ss_saver_320_240_", 17)` in nm_screensaver.cpp:280.
- **Committed in:** 63f3957

**3. [Rule 3 - DRAM headroom] Heap-allocated the multipart upload context**
- **Found during:** Task 2 (DRAM diagnosis)
- **Issue:** File-scope static `File`+`String` upload state consumed scarce dram0_0_seg space.
- **Fix:** Moved upload state to a heap-allocated `SsUpload` struct (new on START, delete in done handler).
- **Verification:** Reclaimed 48 bytes of dram0 (overflow 688→640); good lifecycle hygiene regardless.
- **Committed in:** 63f3957

---

**Total deviations:** 3 auto-fixed (2 Rule 1 API/convention, 1 Rule 3 DRAM hygiene). No scope creep — all endpoints implemented as specified.

## Issues Encountered — BLOCKED: dram0_0_seg overflow (D-58)

**The firmware does NOT link.** `webapi.cpp` compiles clean, but the final link fails:

```
ld: region `dram0_0_seg' overflowed by 640 bytes
```

### Root cause (confirmed by symbol/section analysis)

- Baseline (HEAD webapi) links: dram0_0_seg ORIGIN `0x3ffbdb5c`, LENGTH `0x1e6a4` (end `0x3ffdc200`); baseline `_bss_end` = `0x3ffdb008` → **4600 bytes dram0 headroom**.
- This plan is the FIRST and ONLY translation unit to reference `nm_market_tick` / `nm_market_get_pairs` (verified: no other object references them — nm_market was built by 05-02 but not yet wired into the loop, which is 05-06's job). Referencing them pulls the entire `nm_market.cpp.o` into the link.
- `nm_market.cpp.o` contributes **6158 bytes of `.bss`** to dram0 — dominated by the `s_pairs[NM_MARKET_PAIRS_LEN]` buffer where `NM_MARKET_PAIRS_LEN = 5120` (plus `g_klines`/`g_ticker`). Previously dead-stripped; now resurrected.
- Net dram0 growth from this plan (nm_market 6158 + nm_weather 192 + small misc, minus dead-strip offsets) exceeds the 4600B headroom by 640 bytes.

### Why not auto-fixed here

The fix must reduce dram0 static usage, which is owned by files **outside this plan's scope** (the plan's `files_modified` is `webapi.cpp` only; "You OWN webapi.cpp"):

- **05-02 (preferred):** reduce `NM_MARKET_PAIRS_LEN` from 5120 to ~3072 (Binance lists ~400–500 USDT pairs ≈ 2.5KB CSV; 5120 is defensive over-allocation), OR move `s_pairs` to PSRAM/heap. Either frees ≥1.5KB and is behavior-preserving.
- **platformio.ini (alternative):** lower `HTTP_UPLOAD_BUFLEN`, or enable PSRAM bss placement.

Per the plan constraint ("if it overflows, FLAG don't drop endpoints"), no endpoints were removed and no out-of-scope module was edited. webapi.cpp is committed (63f3957) in a complete, compiling state so the fix can be applied independently and the firmware re-linked.

### What WAS reclaimed in-scope

- Heap-allocated upload context: +48 bytes (688→640). Insufficient alone; the remaining 640 is structural (the s_pairs buffer).

## Build Metrics

- **Compile:** `src/webapi.cpp` → `webapi.cpp.o` SUCCESS (clean).
- **Link:** FAILED — dram0_0_seg overflow by **640 bytes**.
- **Flash:** baseline 89.5% (2,816,373 / 3,145,728); not re-measurable for this plan (no firmware.bin produced). Flash is NOT the constraint — DRAM is.
- **RAM (last successful baseline):** 36.6% total static (this metric does not reflect the tight dram0_0_seg sub-region that overflowed).

## Next Phase Readiness

- **BLOCKER for 05-06 / phase build:** the dram0 overflow must be resolved (05-02 `s_pairs` resize/relocate or platformio.ini) before any firmware that links nm_market can be produced. This will surface regardless of webapi once 05-06 wires `nm_market_tick` into the loop — so fixing it now unblocks both.
- API contract is complete and matches stock JSON shapes/paths; once linked, apps/nmminer-control and NMController should work against the fork unchanged.

---
*Phase: 05-data-driven-screens-http-api-parity*
*Completed (code): 2026-06-18 — build BLOCKED on cross-module DRAM (D-58)*

## Self-Check: PASSED

- `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp` — FOUND (committed clean)
- Fork commit `63f3957` — FOUND in nested repo
- SUMMARY.md — FOUND
