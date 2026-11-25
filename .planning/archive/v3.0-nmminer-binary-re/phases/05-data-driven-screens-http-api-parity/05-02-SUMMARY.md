---
phase: 05-data-driven-screens-http-api-parity
plan: 02
subsystem: firmware-cyd-market
tags: [binance, http, klines, candlestick, pricewall, lv_chart, data-driven, stock-parity]
requires:
  - "nm_settings (05-01): NM_SET_MAIN_COIN/WATCH_COINS/KLINE_INTERVAL/PRICE_PAGE_MODE getters"
provides:
  - "nm_market C module — page-gated Binance HTTP:80 fetch (klines/ticker/pairs) + shared OHLC/ticker globals"
  - "nm_market_tick(now_ms) — loop-driven fetch cadence hook (05-06 wires into ino.cpp loop)"
  - "nm_market_set_page_allowed(bool) — UIManager page-gate (D-52: Miner/Clock/SPrice only)"
  - "nm_market_get_pairs(out,len) — USDT pairs CSV for the Wave-3 /api/market/pairs endpoint"
  - "nm_screen_sprice — real SPrice descriptor (lv_chart candlestick + pricewall, replaces stub)"
affects:
  - "05-06 (integration) — calls nm_market_tick from the loop; calls set_page_allowed from the UIManager page switch"
  - "05-05 (HTTP API) — feeds /api/market/pairs from nm_market_get_pairs"
tech-stack:
  added: []
  patterns:
    - "plain HTTPClient HTTP:80 (no WiFiClientSecure) to data-api.binance.vision — D-51"
    - "millis() cadence in tick() mirroring monitor.cpp updateGlobalData — no new FreeRTOS task"
    - "http.getStream() stream-parse into bounded DynamicJsonDocument (Pitfall 7, no String copy)"
    - "fetch thread writes globals; LVGL thread (screen update) reads them (Pitfall 3 — no lv_* in fetch)"
    - "lv_chart BAR + LV_EVENT_DRAW_PART_BEGIN custom OHLC candles (no lv_canvas — 66KB OOM)"
key-files:
  created:
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_market.h
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_market.cpp
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_sprice.cpp
key-decisions:
  - "Pairs list built from /api/v3/ticker/price stream-filtered to the USDT suffix; the 18MB full exchange-info endpoint is never called (Pitfall 1, T-05-OOM)."
  - "Candlesticks custom-drawn over a suppressed lv_chart BAR series (bar bg_opa=TRANSP) via the draw event; lv_canvas rejected (66KB > 32KB LVGL heap)."
  - "fetch_pairs streams object-by-object with a 128-byte StaticJsonDocument per entry (filter overload was ambiguous in ArduinoJson 6.21; direct per-entry parse is equivalent and bounded)."
  - "PricePageMode re-read every update() so a settings change flips kline<->pricewall live without a rebuild."
requirements-completed: [DATA-01]
metrics:
  duration: "~20 min"
  completed: 2026-06-18
---

# Phase 5 Plan 02: Binance Market Client + SPrice Candlestick/Pricewall Summary

Live Binance market data for the SPrice screen: a page-gated background fetch module
(`nm_market`) pulling klines + 24hr ticker + a USDT pairs list over plain HTTP:80, and a
rewritten SPrice screen rendering an `lv_chart` OHLC candlestick (custom draw event) plus a
WatchCoins pricewall, toggled live by `PricePageMode`.

## What Was Built

- **`src/nm_market.h`** — extern "C" contract. `nm_ohlc_t` (open/high/low/close) and
  `nm_ticker_t` (symbol/last_price/change_pct) structs; shared globals `g_klines[48]` +
  `g_kline_count`, `g_ticker[12]` + `g_ticker_count`, `g_*_updated_ms`, `g_market_ok`
  (volatile, fetch-thread-written / screen-read). API: `nm_market_tick(now_ms)`,
  `nm_market_set_page_allowed(bool)`, `nm_market_get_pairs(out,len)`.
- **`src/nm_market.cpp`** — plain `HTTPClient` HTTP:80 to `data-api.binance.vision`:
  - **klines**: `/api/v3/klines?symbol=<MainCoin>USDT&interval=<KlineInterval>&limit=48`,
    `getStream()` -> `DynamicJsonDocument(10240)`, fills g_klines from row indices [1..4].
  - **ticker**: `/api/v3/ticker/24hr?symbols=%5B"<WatchCoins>USDT"...%5D`,
    `DynamicJsonDocument(4096)` -> g_ticker (base symbol mapped back from request order).
  - **pairs**: `/api/v3/ticker/price` stream-parsed object-by-object (128B StaticJsonDocument
    per entry), filtered to the "USDT" suffix, suffix stripped -> CSV `s_pairs[5120]`. The
    18MB full exchange-info endpoint is never called (Pitfall 1).
  - `nm_market_tick` runs only when page-allowed + Wi-Fi up; ~30s fetch cadence, weekly pairs
    refresh; `http.setTimeout(8000)`. No FreeRTOS task, no `lv_*` (Pitfall 3). Env-guarded.
- **`src/ui_nm/nm_screen_sprice.cpp`** — real `nm_screen_sprice` descriptor (replaces the
  Phase-3 stub). Header per SPEC §SPrice320x240 (coin/title 24pt grey, price 26pt, interval
  12pt white, % change 22pt orange). **Kline mode**: `lv_chart` `LV_CHART_TYPE_BAR`,
  `point_count=48`, `lv_chart_set_range` from kline min/max; a `LV_EVENT_DRAW_PART_BEGIN`
  callback suppresses the default bar (`bg_opa=TRANSP`) and draws each candle — wick
  (low->high `lv_draw_line`) + body (open->close `lv_draw_rect`), green `#84DB31` when
  close>=open else red `#FF0000`. **Pricewall mode**: vertical WatchCoins rows
  (symbol + price + green/red %change pill). `update()` reads g_klines/g_ticker only (never
  fetches) and re-reads `PricePageMode` to switch modes live. No `lv_canvas`.

## nm_market API (for 05-06 integration)

```c
void   nm_market_tick(uint32_t now_ms);           /* call from loop(); ~30s fetch + weekly pairs */
void   nm_market_set_page_allowed(bool allowed);  /* UIManager: true on Miner/Clock/SPrice, false elsewhere (D-52) */
size_t nm_market_get_pairs(char* out, size_t outlen); /* USDT base CSV for /api/market/pairs (Wave 3) */
```

05-06 must: (1) call `nm_market_tick(millis())` each loop iteration; (2) call
`nm_market_set_page_allowed()` from the UIManager page-switch (allowed on NM_PAGE_MINER /
NM_PAGE_CLOCK / NM_PAGE_SPRICE, false on the rest). The SPrice screen's own `update()` is
driven by the existing UIManager update pump — no extra wiring needed for the render side.

## Per-Task Status

| Task | Name                                            | Status | Commit (nested) | Files                              |
|------|-------------------------------------------------|--------|-----------------|------------------------------------|
| 1    | Market fetch module + page-gating               | done   | f7460c2         | src/nm_market.h, src/nm_market.cpp |
| 2    | SPrice kline candlestick + pricewall (de-stub)  | done   | 5e153bb         | src/ui_nm/nm_screen_sprice.cpp     |

Commits are in the nested fork repo (`firmware/custom/NerdMiner-CYD-Custom`). The gitlink bump
is the orchestrator's responsibility.

## Confirmation of Critical Constraints

- **kline = lv_chart** (not lv_canvas): `grep lv_chart` PRESENT, `grep lv_canvas_create` ABSENT.
- **pairs = /api/v3/ticker/price** (USDT-filtered): PRESENT; **exchangeInfo: 0 occurrences** in
  nm_market.cpp (and the .h) — the 18MB endpoint is never referenced.
- **HTTP:80 plain**: `http://data-api.binance.vision`, no `WiFiClientSecure`.
- **No lv_* in nm_market**: fetch module touches no LVGL; SPrice reads g_klines/g_ticker.
- **Files owned**: only nm_market.{h,cpp} + nm_screen_sprice.cpp. No mining.cpp / STATE /
  ROADMAP / sibling-W2 files (nm_weather*, nm_screensaver*, nm_swarm*, nm_uimanager*) touched.

## Verification

- Task 1 gate: `grep data-api.binance.vision && ! grep exchangeInfo` — **MARKET_OK**.
- Task 2 structural gates: `grep lv_chart` PRESENT, `grep lv_canvas_create` ABSENT,
  `grep g_klines` PRESENT, `grep /api/v3/ticker/price` PRESENT.
- Compile: both 05-02 files compile to clean `.o` objects under the full ESP32 toolchain
  (`.pio/build/ESP32-2432S028R/src/nm_market.cpp.o`,
  `.pio/build/ESP32-2432S028R/src/ui_nm/nm_screen_sprice.cpp.o`) — **0 errors in 05-02 files**.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] ArduinoJson `Filter(filter[0])` overload ambiguous (fetch_pairs)**
- **Found during:** Task 1 build.
- **Issue:** `deserializeJson(entry, s, DeserializationOption::Filter(filter[0]))` — passing a
  `JsonDocument` ElementProxy to `Filter()` is ambiguous in ArduinoJson 6.21.5; compile error.
- **Fix:** Dropped the filter; each `/ticker/price` entry is only `{symbol,price}` (~50B), so a
  direct `deserializeJson(entry, s)` into the 128-byte `StaticJsonDocument` per entry is
  equivalent and still bounded (T-05-OOM holds — one tiny doc per stream step).
- **Files modified:** src/nm_market.cpp.
- **Verification:** nm_market.cpp compiles to a clean `.o`.
- **Commit:** f7460c2 (fixed before the Task-1 commit).

**2. [Rule 3 - Blocking] `exchangeInfo` literal in comments tripped the no-exchangeInfo gate**
- **Found during:** Task 1 verify.
- **Issue:** The `! grep -q exchangeInfo` gate failed because explanatory comments contained the
  literal string "exchangeInfo".
- **Fix:** Reworded comments in nm_market.h/.cpp to "the 18MB full exchange-info endpoint" — the
  invariant is documented without the literal token; gate now passes.
- **Files modified:** src/nm_market.h, src/nm_market.cpp.
- **Commit:** f7460c2.

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking). **Impact:** none on behavior — both are
implementation-mechanics fixes; the documented fetch/render contract is fully met.

## Issues Encountered

**Full `pio run` SUCCESS gate not achievable in isolation (out of scope).** The plan's Task-2
gate requires a green full build, but the shared fork working tree currently fails to link due to
sibling-W2 file `src/ui_nm/nm_screensaver.cpp` (plan 05-04, mid-flight): `File`/`f`/`dir`/`e`
not declared (missing FS includes / scope). That file is owned by 05-04 and is explicitly
off-limits to 05-02 (scope boundary). Logged to
`.planning/phases/05-data-driven-screens-http-api-parity/deferred-items.md`. 05-02's own two
files compile to clean `.o` objects; the full-build SUCCESS gate is deferred to 05-06
integration (or whenever 05-04 lands).

## Self-Check: PASSED

- FOUND: firmware/custom/NerdMiner-CYD-Custom/src/nm_market.h
- FOUND: firmware/custom/NerdMiner-CYD-Custom/src/nm_market.cpp
- FOUND: firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_sprice.cpp
- FOUND commit (nested): f7460c2 (feat 05-02 market module)
- FOUND commit (nested): 5e153bb (feat 05-02 SPrice candlestick/pricewall)
- Object files built: nm_market.cpp.o, nm_screen_sprice.cpp.o (0 errors in 05-02 files)
