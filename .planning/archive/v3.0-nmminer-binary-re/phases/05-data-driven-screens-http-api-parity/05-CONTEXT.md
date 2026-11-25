# Phase 5: Data-Driven Screens + HTTP API Parity - Context

**Gathered:** 2026-06-18 (auto mode; grounded in fork scout + SPEC.md + Phase-2/3/4 outputs)
**Status:** Ready for planning (research-recommended per research SUMMARY)

<domain>
## Phase Boundary

Fill the page-gated data behaviors onto the working UIManager carousel: the **SPrice**
screen (Binance kline + pricewall), the **Weather** screen (open-meteo), the
**screensaver carousel** (C `.ss` decoder rendering LittleFS `/ss/` after idle + black
mode + upload/preflight), **Swarm** LAN scan + FindMe overlay, and **HTTP API parity**
with stock's endpoint set so the existing dashboard/controllers work. Branding/identity
strings preserved. Build SUCCESS (`pio run -e ESP32-2432S028R`) is the gate; on-device
data behavior is Phase 7.
</domain>

<decisions>
## Implementation Decisions

### Data sources — match stock (D-51)
- **D-51:** Use **Binance `data-api.binance.vision`** for market (kline/ticker — DATA-01) and **open-meteo.com** for weather (DATA-02) — exact stock hosts (FEATURES.md), do NOT substitute coingecko/mempool (anti-feature: data-source parity). Reuse the fork's existing `HTTPClient` + `WiFiClientSecure` pattern (monitor.cpp). Binance vision is HTTP:80 (no TLS needed); open-meteo is HTTPS.

### SPrice screen (D-52, fills Phase-3 stub)
- **D-52:** Replace the SPrice stub (`nm_screen_sprice`) with kline candlestick + pricewall modes per SPEC §SPrice. `PricePageMode` toggles kline↔pricewall; `MainCoin`/`KlineInterval`/`KlineRotate`/`WatchCoins`. Render kline on an LVGL canvas/chart surface (the +0x24 draw surface from SPEC). Page-gated: market fetch thread runs ONLY on Miner/Clock/Price; `/api/market/pairs` returns `[]` otherwise.

### Weather screen (D-53, fills Phase-3 stub)
- **D-53:** Replace the Weather stub (`nm_screen_weather`) with open-meteo forecast/AQI: current conditions, temp (°C/°F), 3-day, sunrise/sunset, wind. `WeatherTempUnit`/`WeatherSpeedUnit`/`WeatherAltMode`/City+lat/lon. Rate-limited refresh.

### Screensaver carousel (D-54)
- **D-54:** Port the `.ss` decoder to firmware C from `hardware/recovered-assets/SS-ALGORITHM-SPEC.md` (the Phase-1 `ss_render` algorithm; SN magic + RGB565→RLE→LZSS, with RGB565→RGB888 **bit-replication** per Phase-2). After idle timeout (integrate the Phase-4 idle timer; prefs `never/30s/1m/5m/15m/30m`), render `/ss/saver_320_240_*.ss` from LittleFS in a carousel; **black mode** (`ScreenSaverMode:"black"`) blanks instead; resume on touch. FindMe overlay OVERRIDES the screensaver. Upload (`/api/update/screensaver`, multipart ≤200KB, magic 0x4E53) + preflight (`/api/update/screensaver/preflight?size=`) endpoints.

### HTTP API parity (D-55)
- **D-55:** Extend the fork's `webapi.cpp` (`apiServer`) to the stock endpoint contract ADDITIVELY (keep the custom `/api/speaker/*` + `/api/sd/*`): `/probe`, `/alive`, `/api/system/info` (FULL stock field set — already an alias), `/api/setting/{network,mining,time,preference,market,weather}` (GET+POST), `/api/market/pairs`, `/api/weather/refresh` (429 rate-limit 60s), `/api/swarm/find`, `/api/system/restart`, `/api/update/screensaver` + `/preflight`. Match stock JSON paths/shapes so `apps/nmminer-control` + NMController work. Captive-portal routes (WiFiManager) already exist.

### Swarm (D-56)
- **D-56:** Background LAN ping scan gated to the Miner page (sleeps after 30s no `/alive`); `/alive` returns self + dedup IP list; `/api/swarm/find` triggers a screen-flash/LED FindMe overlay that overrides the screensaver.

### Branding (D-57)
- **D-57:** `/probe` + `/api/system/info` report `model="NMMiner"`, `hwModel="NMMiner"`, "Make it better" so controllers keying off `model` work (DATA-06). (Note: the running device identifies as the fork today; this restores stock identity in the API.)

### Flash budget watch (D-58)
- **D-58 (RISK):** Flash is at **87.4%** of the 3MB `huge_app` partition (~400KB headroom). Market+weather+screensaver+API+`.ss`-decoder add code. The `.ss` IMAGES live in LittleFS (not app flash), so the main cost is logic + JSON parsing. Monitor the build gate; if it overflows: trim unused LVGL features in lv_conf.h, gate optional bits, or revisit partition. Do NOT silently drop a DATA requirement to fit — surface a budget decision.

### Claude's Discretion
- Kline rendering: LVGL chart widget vs manual canvas draw.
- JSON lib (ArduinoJson — already in fork) buffer sizing.
- Whether market/weather run as FreeRTOS tasks or loop-cadence (match stock thread-gating either way).
</decisions>

<canonical_refs>
## Canonical References
- hardware/recovered-assets/SPEC.md §SPrice/§Weather/§ss_render + §Screen Inventory (geometry + market field set).
- hardware/recovered-assets/SS-ALGORITHM-SPEC.md (the C `.ss` decoder algorithm), scripts/nmminer_ss_decode.py (host reference), hardware/recovered-assets/littlefs/ (the 17 `.ss`).
- docs/research/nmminer-api-reference.md (the 21-endpoint stock API — paths/fields/defaults), .planning/research/FEATURES.md (data sources, page-gating, screensaver/swarm behavior).
- .planning/research/PITFALLS.md / SUMMARY.md (Phase 5 research flags: .ss header byte-layout, kline specifics, idle→screensaver state machine).
- firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp (existing API to extend), src/monitor.cpp (HTTPClient/WiFiClientSecure pattern + existing price feed), src/ui_nm/nm_screen_{sprice,weather}.cpp (the stubs to fill), nm_uimanager.{cpp,h} (page-gating hook + idle timer).
- .planning/REQUIREMENTS.md — DATA-01..06.
- apps/nmminer-control (the local dashboard that consumes the API — keep its contract working).
</canonical_refs>

<code_context>
## Existing Code Insights
- monitor.cpp already does HTTPClient + WiFiClientSecure (skip-cert) for public APIs — reuse for Binance/open-meteo.
- webapi.cpp `apiServer` already serves /api/system/info + speaker/sd — extend, don't replace; add stock endpoints alongside.
- nm_uimanager has the page index (for page-gating market/swarm) + the Phase-4 idle timer (for screensaver trigger).
- ArduinoJson is already a fork dep.
</code_context>

<specifics>
## Specific Ideas
- Match stock numbers/cadence exactly (Binance + open-meteo) — the dashboard + muscle-memory expect identical values.
- The screensaver reuses the user's already-working `.ss` files in LittleFS (17 present) + the Phase-2-validated decode algorithm with the saturation fix.
</specifics>

<deferred>
## Deferred Ideas
- Speaker/SD/LED wiring at the share-accept seam → Phase 6.
- On-device data/render/upload confirmation → Phase 7.
- Differentiator polish (new-fw reminder, Christmas egg, auto-TZ) → v2/POLISH.
</deferred>

---
*Phase: 5-data-driven-screens-http-api-parity*
*Context gathered: 2026-06-18*
