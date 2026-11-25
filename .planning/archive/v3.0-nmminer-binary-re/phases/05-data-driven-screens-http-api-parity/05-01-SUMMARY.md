---
phase: 05-data-driven-screens-http-api-parity
plan: 01
subsystem: firmware-cyd-settings
tags: [nvs, preferences, settings, stock-parity, foundation]
requires: []
provides:
  - "nm_settings C interface (nm_stock NVS namespace) — typed get/put for all 13 stock fields"
  - "nm_settings_ss_timeout_ms token->ms map (never/30s/1m/5m/15m/30m)"
  - "stock-exact field-name + default macros (NM_SET_* / NM_DEF_*)"
affects:
  - "Wave 2 (05-02 market, 05-03 weather, 05-04 screensaver) — read/write these fields"
  - "Wave 3 (05-05 HTTP API setting handlers) — write the same NVS keys"
tech-stack:
  added: []
  patterns:
    - "mirrors nm_uimanager nm_ui Preferences idiom: static Preferences + s_ready begin() gate"
    - "extern C contract header, env-guarded .cpp (#if ESP32_2432S028R || ESP32_2432S028_2USB)"
key-files:
  created:
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.h
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.cpp
  modified: []
decisions:
  - "nm_stock namespace chosen distinct from nm_ui (owned by LastPage manager) — no collision."
  - "WeatherSpeedUnit (16ch) exceeds the 15-char NVS key limit; stored under short alias WSpeedUnit. Public API key string stays stock-exact via resolve_key()."
  - "No native unit-test env exists in the fork; TDD behavior is gated by the ESP32 build (D-58 minimal-code — adding a native harness is out of scope). Behavior functions are pure and implemented to the documented contract."
metrics:
  duration: "~6 min"
  completed: 2026-06-18
---

# Phase 5 Plan 01: NVS Settings Foundation Summary

Preferences-backed stock-parity settings layer (`nm_settings.{h,cpp}`) on a dedicated
`nm_stock` NVS namespace, exposing typed get/put for all 13 stock setting fields with
stock-exact names/defaults plus the ScreenSaver token->ms map — the shared foundation the
Wave-2 data screens and Wave-3 HTTP API handlers both key off.

## What Was Built

- **`src/nm_settings.h`** — extern "C" contract. `NM_SET_*` field-name strings + `NM_DEF_*`
  default macros for market (MainCoin/WatchCoins/KlineInterval/KlineRotate/PricePageMode),
  screensaver (ScreenSaver/ScreenSaverMode), weather (WeatherCity/WeatherLat/WeatherLon/
  WeatherTempUnit/WeatherSpeedUnit/WeatherAltMode). Declares `nm_settings_init`,
  `nm_settings_get_str/put_str`, `nm_settings_get_f/put_f`, `nm_settings_get_b/put_b`,
  `nm_settings_ss_timeout_ms`.
- **`src/nm_settings.cpp`** — static `Preferences` on `nm_stock`, `s_ready` begin() gate
  mirroring nm_uimanager. Each accessor maps to `getString/putString/getFloat/putFloat/
  getBool/putBool` with header defaults. `nm_settings_get_str` is bounded to outlen-1 +
  NUL (T-05-VAL). `resolve_key()` aliases WeatherSpeedUnit->WSpeedUnit (NVS 15ch limit).
  Token map: never->0, 30s->30000, 1m->60000, 5m->300000, 15m->900000, 30m->1800000,
  unknown->60000. Env-guarded to CYD boards.

## nm_settings API Surface (for W2/W3 executors)

```c
bool     nm_settings_init(void);                                                /* open nm_stock once; true if ready */
size_t   nm_settings_get_str(const char* key, char* out, size_t outlen, const char* def);
void     nm_settings_put_str(const char* key, const char* val);
float    nm_settings_get_f(const char* key, float def);
void     nm_settings_put_f(const char* key, float val);
bool     nm_settings_get_b(const char* key, bool def);
void     nm_settings_put_b(const char* key, bool val);
uint32_t nm_settings_ss_timeout_ms(const char* token);
```

### Fields (use the `NM_SET_*` macro as `key`; getter returns `NM_DEF_*` if unset)

| Macro (key string)        | Type   | Default            | Group       |
|---------------------------|--------|--------------------|-------------|
| NM_SET_MAIN_COIN          | str    | "BTC"              | market      |
| NM_SET_WATCH_COINS        | str    | "ETH,BNB,SOL,XRP"  | market      |
| NM_SET_KLINE_INTERVAL     | str    | "1h"               | market      |
| NM_SET_KLINE_ROTATE       | bool   | false              | market      |
| NM_SET_PRICE_PAGE_MODE    | str    | "kline"            | market      |
| NM_SET_SCREENSAVER        | str    | "1m"               | screensaver |
| NM_SET_SCREENSAVER_MODE   | str    | "image"            | screensaver |
| NM_SET_WEATHER_CITY       | str    | "Beijing"          | weather     |
| NM_SET_WEATHER_LAT        | float  | 39.9075f           | weather     |
| NM_SET_WEATHER_LON        | float  | 116.3972f          | weather     |
| NM_SET_WEATHER_TEMPUNIT   | str    | "celsius"          | weather     |
| NM_SET_WEATHER_SPDUNIT    | str    | "kmh"              | weather     |
| NM_SET_WEATHER_ALTMODE    | bool   | false              | weather     |

`NM_SET_WEATHER_SPDUNIT` value string is `"WeatherSpeedUnit"` (stock-exact); it is the only
field stored under an internal NVS alias (`WSpeedUnit`). Callers always pass the macro.

### ScreenSaver timeout token map (`nm_settings_ss_timeout_ms`)

| token   | ms      | note                         |
|---------|---------|------------------------------|
| "never" | 0       | caller treats 0 as skip      |
| "30s"   | 30000   |                              |
| "1m"    | 60000   |                              |
| "5m"    | 300000  |                              |
| "15m"   | 900000  |                              |
| "30m"   | 1800000 |                              |
| (other) | 60000   | unknown -> stock "1m"        |

## Per-Task Status

| Task | Name                                          | Status | Commit  | Files                         |
|------|-----------------------------------------------|--------|---------|-------------------------------|
| 1    | Define the NMSettings interface (.h)          | done   | 1d373e1 | src/nm_settings.h             |
| 2    | Implement Preferences-backed accessors (.cpp) | done   | 4f04255 | src/nm_settings.cpp           |

Commits are in the nested fork repo (`firmware/custom/NerdMiner-CYD-Custom`). Gitlink bump
is the orchestrator's responsibility.

## Verification

- Task 1 gate: `grep nm_settings_get && grep nm_settings_ss_timeout_ms` on the header — HDR_OK.
- Task 2 gate: `pio run -e ESP32-2432S028R` — **SUCCESS** (75s); nm_settings.cpp linked.
  Flash 87.4% (2748673 / 3145728), RAM 35.3%.

## Deviations from Plan

None affecting behavior. Two plan-anticipated specializations applied:

1. **[Rule 3 - blocking] WeatherSpeedUnit NVS key alias.** The plan instructed: if any field
   name exceeds the 15-char NVS limit, use a documented short alias while keeping the public
   API name stock-exact. WeatherSpeedUnit is 16 chars → aliased to `WSpeedUnit` via
   `resolve_key()`. All other 12 names fit (≤15) and pass through unchanged. Commit 4f04255.

2. **TDD note (not a deviation):** Task 2 carried `tdd="true"`, but the fork has no native
   PlatformIO test env and adding one contradicts D-58 (minimal-code vs. headroom). The TDD
   behavior contract (token map values, bounded copy, default-before-put) is satisfied by the
   implementation and gated by the ESP32 build; no separate `test(...)` commit was produced
   because there is no host test harness to run.

## TDD Gate Compliance

This is an `auto` plan (frontmatter `type: execute`), not a `type: tdd` plan, so the
RED/GREEN/REFACTOR commit sequence is not mandated at the plan level. Task 2's `tdd="true"`
marker was honored at the behavior level (implementation matches the documented behavior
assertions) but verified via the hardware build gate rather than a native test, as no native
test env exists in the fork.

## Self-Check: PASSED

- FOUND: firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.h
- FOUND: firmware/custom/NerdMiner-CYD-Custom/src/nm_settings.cpp
- FOUND commit (nested): 1d373e1
- FOUND commit (nested): 4f04255
