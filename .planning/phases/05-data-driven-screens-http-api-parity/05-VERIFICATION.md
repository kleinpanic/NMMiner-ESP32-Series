---
phase: 05-data-driven-screens-http-api-parity
verified: 2026-06-18T00:00:00Z
status: code_build_passed_hardware_unverified
hardware_verified: false
hardware_status: unverified_on_device_until_phase_7
superseded_by: ".planning/phases/07-on-device-validation/07-VERIFICATION.md"
score: "6/6 DATA code/build requirements verified; live device validation pending"
overrides_applied: 0
re_verification:
build:
  env: ESP32-2432S028R
  result: SUCCESS
  flash_pct: 90.5
  flash_bytes: 2846141
  flash_total: 3145728
  ram_pct: 37.6
  ram_bytes: 123168
---

# Phase 5: Data-Driven Screens + HTTP API Parity — Verification Report

**Phase Goal:** The page-gated market/weather/swarm/screensaver behaviors and the full HTTP API match stock cadence and shapes so the existing dashboard/controller contract holds.
**Verified:** 2026-06-18
**Status:** passed for code/build/API-structure scope only; on-device data, render, upload, and API behavior remain Phase 7 gaps
**Re-verification:** No — initial verification

> Current evidence boundary (2026-06-19): this report verifies the BUILD gate and code/API structure for each DATA requirement. It does not prove live CYD data fetch, rendering, upload, or API behavior. Use `07-CURRENT-FAILURE-STATE.md` and `07-VERIFICATION.md` for live hardware truth.

## Build Gate

```
cd firmware/custom/NerdMiner-CYD-Custom && .venv/bin/pio run -e ESP32-2432S028R
```
Result: `[SUCCESS] Took 71.72 seconds` — all Phase-5 modules linked.
- **Flash: 90.5%** (2,846,141 / 3,145,728 bytes) — D-58 budget respected, no overflow.
- **RAM: 37.6%** (123,168 / 327,680 bytes).

## Goal Achievement — Code/API Truths (per DATA req)

These truths were verified from source and build output, not from live Wi-Fi,
HTTP, or screen behavior on the physical miner.

| # | Requirement | Status | Evidence |
| --- | --- | --- | --- |
| DATA-01 | SPrice/Binance kline + pricewall, page-gated | ✓ VERIFIED | `nm_market.cpp` HTTP:80 `data-api.binance.vision`, `/api/v3/klines`+`/ticker/24hr`+`/ticker/price` (zero `exchangeInfo` repo-wide), `s_page_allowed` gate. `nm_screen_sprice.cpp` lv_chart BAR + `sprice_chart_draw_cb` candle draw + pricewall rows, mode toggled by `PricePageMode`, reads `g_klines`/`g_ticker`. WR-01: ticker mapped by `symbol` field (nm_market.cpp:143-161), not position. |
| DATA-02 | Weather/open-meteo forecast+AQI, 60s rate-limit | ✓ VERIFIED | `nm_weather.cpp` HTTPS `api.open-meteo.com` forecast + `air-quality-api.open-meteo.com` AQI, `WiFiClientSecure::setInsecure`, `NM_WEATHER_REFRESH_MS 60000`, `nm_weather_request_refresh` returns false (→429) inside window. `nm_screen_weather.cpp` renders temp(C/F)/cond/humidity/wind+unit/pressure/AQI/sunrise/sunset/3-day from `g_weather`. |
| DATA-03 | Screensaver .ss carousel, black mode, prefs, upload/preflight, resume | ✓ VERIFIED | `nm_screensaver.cpp` SN magic `0x53 0x4E` validate, LZSS (MSB bitreader, 1024 ring) → RLE → RGB565 stream blit, `/ss/ss_saver_320_240_*.ss` enumeration, `s_black_mode`, `nm_ss_tick` carousel. `nm_uimanager.cpp` `s_ss_active` 2-stage idle SM, touch resume via `nm_input_wake_if_dimmed`→`nm_ss_stop`. `webapi.cpp` `/api/update/screensaver` (magic+200KB) + `/preflight`. |
| DATA-04 | Swarm Miner-gated scan + /api/swarm/find FindMe override | ✓ VERIFIED | `nm_swarm.cpp` `s_enabled` Miner gate, /24 probe, dedup `nm_swarm_add_ip`, 30s `NM_SWARM_SLEEP_MS` sleep after no `/alive`. `nm_uimanager.cpp` polls `nm_swarm_findme_pending`→`nm_ss_findme_start`, flash branch `return`s before screensaver stage (overrides carousel). `webapi.cpp` `/api/swarm/find`→`nm_swarm_request_findme`. |
| DATA-05 | HTTP API parity (21 endpoints), additive | ✓ VERIFIED | `webapi.cpp` registers `/probe`, `/alive`, `/api/system/info`, `/api/setting/{network,mining,time,preference,market,weather}` GET+POST, `/api/market/pairs`, `/api/weather/refresh`, `/api/swarm/find`, `/api/system/restart`, `/api/update/screensaver{,/preflight}` — `/api/speaker/*` + `/api/sd/*` routes preserved. POST market/weather trigger refresh; pairs `[]` when empty; refresh 429 within 60s. |
| DATA-06 | Branding model="NMMiner" / "Make it better" | ✓ VERIFIED | `handleProbe` model+hwModel="NMMiner"; `handleSystemInfo` identity.model/hwModel="NMMiner", tagline="Make it better". |

**Score:** 6/6 DATA code/build requirements verified; physical data/API behavior is pending Phase 7 validation.

## Required Artifacts

| Artifact | Status | Details |
| --- | --- | --- |
| `src/nm_settings.cpp` (124L) + `.h` | ✓ VERIFIED | `nm_settings_ss_timeout_ms` present; nm_stock namespace get/put. |
| `src/nm_market.cpp` (242L) | ✓ VERIFIED | klines/ticker/pairs HTTP:80, page-gated, WR-01 symbol map. |
| `src/ui_nm/nm_screen_sprice.cpp` (292L) | ✓ VERIFIED | lv_chart candle draw + pricewall, reads g_klines/g_ticker. |
| `src/nm_weather.cpp` (203L) | ✓ VERIFIED | open-meteo HTTPS forecast+AQI, 60s rate-limit. |
| `src/ui_nm/nm_screen_weather.cpp` (217L) | ✓ VERIFIED | full weather layout from g_weather. |
| `src/ui_nm/nm_screensaver.cpp` (370L) | ✓ VERIFIED | .ss decoder + carousel + black + FindMe. |
| `src/nm_swarm.cpp` (139L) | ✓ VERIFIED | Miner-gated scan + dedup + FindMe flag. |
| `src/ui_nm/nm_uimanager.cpp` (329L) | ✓ VERIFIED | s_ss_active SM + FindMe override + page gating. |
| `src/webapi.cpp` (750L) | ✓ VERIFIED | 21 endpoints additive + branding + upload validation. |
| `src/NerdMinerV2.ino.cpp` | ✓ VERIFIED | nm_settings_init (boot), nm_market_tick/nm_weather_tick + webApiSetup/Loop wired; swarm_tick driven inside nm_uimanager_tick. |

## Key Link Verification

| From | To | Status | Detail |
| --- | --- | --- | --- |
| nm_market.cpp | data-api.binance.vision:80 | ✓ WIRED | 2 host refs; klines/ticker/pairs URLs. |
| nm_screen_sprice.cpp | g_klines/g_ticker | ✓ WIRED | read in update(); draw cb indexes g_klines. |
| nm_weather.cpp | open-meteo:443 (forecast+AQI) | ✓ WIRED | 2 host refs, setInsecure + GET. |
| nm_screen_weather.cpp | g_weather | ✓ WIRED | guarded by g_weather.valid. |
| nm_uimanager.cpp | screensaver+swarm+market gate | ✓ WIRED | s_ss_active, nm_swarm_set_enabled, nm_market_set_page_allowed. |
| nm_screensaver.cpp | LittleFS /ss/ | ✓ WIRED | LittleFS.open + nm_ss_decode. |
| webapi.cpp | nm_settings/market/weather/swarm | ✓ WIRED | handlers call into modules. |
| webapi.cpp | LittleFS /ss upload | ✓ WIRED | UPLOAD_FILE_* + magic 0x53/0x4E. |
| NerdMinerV2.ino.cpp setup() | nm_settings_init | ✓ WIRED | line 146. |
| NerdMinerV2.ino.cpp loop() | nm_market_tick/nm_weather_tick | ✓ WIRED | lines 414-415; swarm via uimanager_tick. |

## Data-Flow Trace (Level 4)

On-device live data is Phase 7. Structural data path verified: render screens read shared globals (`g_klines`/`g_ticker`/`g_weather`) that the background ticks populate from real HTTP/JSON parses (not hardcoded). No static-empty render paths — empty states show "--"/"loading" until first fetch, then real data flows.

## Anti-Patterns Found

None. Zero `TBD/FIXME/XXX/PLACEHOLDER` markers across all Phase-5 files. No stub renders (empty states are intentional pre-fetch placeholders overwritten by tick-populated globals).

## Requirements Coverage

| Requirement | Status | Evidence |
| --- | --- | --- |
| DATA-01..06 | code/build satisfied, hardware pending | See per-req table above and Phase 7 hardware gaps. |

`mining.cpp`: untouched — `git diff` clean, zero Phase-5 symbol references; goal "additive on the fork" honored.

## Deferred Items

`deferred-items.md` entries (nm_market Filter ambiguity, cross-plan build ordering) were mid-execution build-sequencing notes, now RESOLVED: full `pio run` SUCCEEDS; the Filter ambiguity was worked around with per-entry streaming parse (nm_market.cpp:178-204). No outstanding deferrals.

## Human Verification Required

Required in Phase 7 for milestone closure. No hardware test was part of this
original code/build verifier, and live data/render/upload/API behavior remains
unproven on the CYD.

## Gaps Summary

No gaps for the original Phase 5 code/build/API-structure scope. Phase 7
hardware gaps remain: Wi-Fi/pool/mining, live market/weather fetch, custom API
exposure, screensaver upload, and on-device render behavior must be judged from
`07-VERIFICATION.md`, not this report. All 6 DATA requirements have substantive,
wired, build-passing implementations. Build SUCCESS at Flash 90.5% / RAM 37.6%,
within the D-58 budget. The fork's speaker/sd routes are preserved (additive);
mining.cpp is untouched.

---

_Verified: 2026-06-18_
_Verifier: Claude (gsd-verifier)_
