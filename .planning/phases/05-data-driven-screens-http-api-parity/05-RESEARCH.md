# Phase 5: Data-Driven Screens + HTTP API Parity — Research

**Researched:** 2026-06-18
**Domain:** ESP32 Arduino / LVGL v8 / Binance REST / open-meteo REST / .ss firmware decoder / WebServer multipart
**Confidence:** HIGH for verified items; MEDIUM for API shapes (live-tested); ASSUMED for anything drawn from training only

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-51:** Use Binance `data-api.binance.vision` (HTTP:80 — no TLS) for market data; open-meteo.com for weather. Do NOT substitute coingecko/mempool.
- **D-52:** Replace `nm_screen_sprice` stub with kline candlestick + pricewall modes per SPEC §SPrice. LVGL canvas/chart (discretion). Page-gated: market thread runs ONLY on Miner/Clock/Price.
- **D-53:** Replace `nm_screen_weather` stub with open-meteo forecast/AQI; immediate refresh on settings save.
- **D-54:** Port `.ss` decoder to C from SS-ALGORITHM-SPEC.md. Idle → screensaver carousel in `/ss/` LittleFS. Black mode = blank. FindMe overrides screensaver. Upload + preflight endpoints.
- **D-55:** Extend `webapi.cpp` `apiServer` ADDITIVELY (keep `/api/speaker/*` + `/api/sd/*`). Match stock JSON paths/shapes.
- **D-56:** Background LAN ping scan gated to Miner page. `/alive` returns self + dedup IPs. `/api/swarm/find` → FindMe overlay overrides screensaver.
- **D-57:** `/probe` + `/api/system/info` report `model="NMMiner"`, `hwModel="NMMiner"`, "Make it better".
- **D-58 (RISK):** Flash at 87.6% of 3MB huge_app (~381 KB headroom). Monitor build gate. Surface decision if overflow; do NOT silently drop a DATA requirement.

### Claude's Discretion

- Kline rendering: LVGL chart widget vs manual canvas draw.
- ArduinoJson buffer sizing.
- Whether market/weather run as FreeRTOS tasks or loop-cadence.

### Deferred Ideas (OUT OF SCOPE)

- Speaker/SD/LED wiring at share-accept seam → Phase 6.
- On-device data/render/upload confirmation → Phase 7.
- Differentiator polish (new-fw reminder, Christmas egg, auto-TZ) → v2/POLISH.
</user_constraints>

---

## Summary

Phase 5 fills the three data-driven stubs (SPrice, Weather, screensaver carousel) and adds the remaining 13 stock API endpoints to the fork's existing `apiServer`. All six technical domains have been researched against live APIs and the existing codebase.

The two most important findings: (1) `data-api.binance.vision` serves HTTP on port 80 — confirmed live — so `HTTPClient` without `WiFiClientSecure` suffices for all market calls; open-meteo requires HTTPS for both forecast and AQI. (2) Flash headroom is 381 KB with an estimated ~26 KB Phase 5 code addition, leaving ~355 KB — LOW risk under normal coding discipline, though the `huge_app` partition is already at 87.6%. Every .ss image lives in LittleFS (not app flash), so image count is not a flash problem.

The `.ss` decoder algorithm is fully specified in SS-ALGORITHM-SPEC.md and complete enough for a direct C port. The idle→screensaver state machine requires extending the Phase-4 `s_dimmed` / `s_last_input_ms` mechanism in `nm_uimanager.cpp` rather than building a new one. LVGL v8 `LV_USE_CHART` and `LV_USE_CANVAS` are both enabled in `lv_conf.h`; `lv_chart` with `LV_CHART_DRAW_PART_BAR` + custom draw event is the recommended OHLC path (no manual pixel loop, respects LVGL's 32 KB heap constraint).

**Primary recommendation:** Implement the market/weather fetches as periodic loop-cadence functions (matching stock's existing HTTPClient/millis()-gated pattern in `monitor.cpp`) rather than new FreeRTOS tasks — saves ~4 KB stack RAM per task on a no-PSRAM device.

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Market data fetch (klines/ticker) | Background loop task (Core 1) | — | HTTPClient is blocking; must run off the LVGL pump. Mirror stock's monitor.cpp pattern |
| Weather data fetch (forecast + AQI) | Background loop task (Core 1) | — | Two HTTPS calls; same reason |
| LVGL screen rendering (SPrice/Weather) | LVGL pump (nm_uimanager_tick) | — | All LVGL API calls must be on the LVGL thread |
| Screensaver state machine | nm_uimanager_tick (extends Phase-4 idle) | — | Already owns `s_dimmed`/`s_last_input_ms` |
| .ss decoder (C port) | Called from screensaver render path | LVGL display driver | Streams rows to display via driver vtable |
| HTTP API endpoints | apiServer (WebServer, port 80) | WiFiManager captive portal | Must coexist: different WebServer instances or same one with different routes |
| NVS settings persistence | Preferences (Arduino) | — | All stock settings stored/read here |
| Swarm LAN scan | FreeRTOS task (gated) | — | Blocking TCP probes; must be off-LVGL |
| FindMe overlay | LVGL pump | — | Screen overlay drawn via LVGL |
| .ss upload (multipart) | apiServer (WebServer) | LittleFS | WebServer::onFileUpload callback |

---

## Standard Stack

### Core (already in fork — no new installs)

| Library | Version | Purpose | Status |
|---------|---------|---------|--------|
| `lvgl` | ^8.4.0 | LVGL v8 display widgets (`lv_chart`, `lv_canvas`) | Already linked |
| `bblanchon/ArduinoJson` | ^6.21.5 | JSON parse for API responses | Already linked |
| `HTTPClient` | Arduino ESP32 built-in | HTTP GET for Binance (port 80) | Already used in monitor.cpp |
| `WiFiClientSecure` | Arduino ESP32 built-in | HTTPS for open-meteo | Already used in monitor.cpp (`secureClient.setInsecure()`) |
| `WebServer` | Arduino ESP32 built-in | `apiServer` on port 80 | Already used in webapi.cpp |
| `LittleFS` | ESP32 Arduino framework | `/ss/` screensaver image storage | Board `board_build.filesystem = LittleFS` already set |
| `Preferences` | Arduino ESP32 built-in | NVS storage for stock settings | Already used in nm_uimanager.cpp |

**Phase 5 adds zero new library dependencies.** All required libraries are already present.

---

## Package Legitimacy Audit

> Phase 5 installs NO new external packages. All libraries are existing fork dependencies.

**Packages removed due to SLOP verdict:** none
**Packages flagged as suspicious SUS:** none

---

## Architecture Patterns

### System Architecture Diagram

```
Touch / Button
     │
     ▼
nm_uimanager_tick()  ◄── loop() Core 1
     │
     ├── LVGL pump (nm_display_tick)
     │       └── active page update() ──► lv_chart / lv_label_set_text_fmt
     │
     ├── Idle timer (s_last_input_ms)
     │       ├── ACTIVE: normal rendering
     │       ├── DIM: nm_display_dim() [Phase 4 — already live]
     │       └── SCREENSAVER: [Phase 5 — new]
     │               ├── mode="image": ss_render() → LittleFS → display
     │               └── mode="black": nm_display_dim(0) / blank
     │
     └── Page index (s_current)
             ├── Miner/Clock/SPrice → market_task allowed to run
             └── Loading/Weather/Config → market_task sleeps


Market loop (monitor-style, Core 1, periodic millis() gate):
     Binance HTTP:80
          ├── /api/v3/klines?symbol=BTCUSDt&interval=1h&limit=48
          │       └── parse → g_kline_data[48] (O/H/L/C float array)
          ├── /api/v3/ticker/24hr?symbols=[...]
          │       └── parse → g_ticker[20] (symbol/price/change)
          └── /api/v3/ticker/price (all) → g_pairs_list[] (USDT bases)

Weather loop (monitor-style, Core 1, periodic):
     open-meteo HTTPS (WiFiClientSecure, setInsecure())
          ├── api.open-meteo.com/v1/forecast
          │       └── parse → g_weather (temp/humidity/wind/pressure/weather_code/daily/sunrise/sunset)
          └── air-quality-api.open-meteo.com/v1/air-quality
                  └── parse → g_weather.aqi (european_aqi current hour)

apiServer (WebServer port 80):
     GET  /probe  /alive  /api/system/info
     G/P  /api/setting/{network,mining,time,preference,market,weather}
     GET  /api/market/pairs
     POST /api/weather/refresh  (429 rate-limit 60s)
     POST /api/swarm/find
     POST /api/system/restart
     GET  /api/update/screensaver/preflight?size=
     POST /api/update/screensaver  (multipart, ≤200KB)
     GET/POST /api/speaker/*  /api/sd/*  (existing, keep)
```

### Recommended Project Structure

New files for Phase 5 (all in `src/ui_nm/` or `src/`):

```
src/
├── ui_nm/
│   ├── nm_screen_sprice.cpp     [REPLACE stub — kline + pricewall]
│   ├── nm_screen_weather.cpp    [REPLACE stub — open-meteo display]
│   ├── nm_screensaver.cpp       [NEW — .ss decode + carousel state machine]
│   ├── nm_screensaver.h         [NEW]
│   └── nm_uimanager.cpp         [EXTEND — idle→screensaver transition]
├── nm_market.cpp                [NEW — Binance fetch + page-gating]
├── nm_market.h                  [NEW]
├── nm_weather.cpp               [NEW — open-meteo fetch]
├── nm_weather.h                 [NEW]
├── nm_swarm.cpp                 [NEW — LAN scan + FindMe]
├── nm_swarm.h                   [NEW]
├── nm_settings.cpp              [NEW — NVS for stock settings fields]
├── nm_settings.h                [NEW]
└── webapi.cpp                   [EXTEND — add 13 stock endpoints]
```

### Pattern 1: Binance klines fetch (HTTP:80, no TLS)

```cpp
// Source: verified live — http://data-api.binance.vision/api/v3/klines
// [VERIFIED: live curl test, HTTP/1.1 200 from port 80]
HTTPClient http;
http.setTimeout(10000);
http.begin("http://data-api.binance.vision/api/v3/klines"
           "?symbol=BTCUSDT&interval=1h&limit=48");
int code = http.GET();
if (code == 200) {
    // Buffer: 48 klines * ~190 bytes each = ~9KB; use DynamicJsonDocument(10240)
    DynamicJsonDocument doc(10240);
    deserializeJson(doc, http.getStream());  // stream parse — avoids large String copy
    // kline[i][0]=open_time_ms, [1]=open_str, [2]=high_str, [3]=low_str, [4]=close_str
    for (int i = 0; i < doc.size() && i < 48; i++) {
        g_klines[i].open  = doc[i][1].as<float>();
        g_klines[i].high  = doc[i][2].as<float>();
        g_klines[i].low   = doc[i][3].as<float>();
        g_klines[i].close = doc[i][4].as<float>();
    }
}
http.end();
```

**Critical:** Use `http.getStream()` not `http.getString()` — avoids a 9KB heap allocation for the raw string.

### Pattern 2: Binance 24hr ticker (multi-symbol pricewall)

```cpp
// Source: verified live — http://data-api.binance.vision/api/v3/ticker/24hr
// [VERIFIED: live curl test]
// symbols param: URL-encode the JSON array
// ETH,BNB,SOL → symbols=%5B%22ETHUSDT%22%2C%22BNBUSDT%22%2C%22SOLUSDT%22%5D
String url = "http://data-api.binance.vision/api/v3/ticker/24hr?symbols=";
url += "%5B%22" + coin1 + "USDT%22%5D";  // build URL-encoded array
// Response fields per entry: symbol, lastPrice, priceChange, priceChangePercent
// 20-coin response ~2.5KB; DynamicJsonDocument(4096) sufficient
```

### Pattern 3: Pairs list from ticker/price (avoid 18MB exchangeInfo)

```cpp
// Source: verified live — /api/v3/ticker/price returns 152KB for all symbols
// [VERIFIED: live curl test — 436 USDT pairs, 152KB response]
// DO NOT use /api/v3/exchangeInfo — it returns 18MB, ESP32 cannot parse it.
// Instead: fetch /api/v3/ticker/price, filter entries where symbol ends in "USDT",
// strip "USDT" suffix to get base asset list.
// Parse with stream + filter: DynamicJsonDocument(4096) per entry, build list iteratively
// OR: use a pre-filtered static list from first boot and refresh weekly.
// The firmware string firmware.bin confirms this path: binary contains "/ticker/price"
```

### Pattern 4: open-meteo forecast (HTTPS — needs WiFiClientSecure)

```cpp
// Source: verified live response structure
// [VERIFIED: live curl test confirmed JSON shape]
// Forecast: HTTPS only (unlike Binance which also supports HTTP)
WiFiClientSecure sc;
sc.setInsecure();   // same pattern as monitor.cpp secureClient
HTTPClient http;
http.begin(sc, "https://api.open-meteo.com/v1/forecast"
               "?latitude=39.9075&longitude=116.3972"
               "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,surface_pressure,weather_code"
               "&daily=temperature_2m_max,temperature_2m_min,weather_code,sunrise,sunset"
               "&temperature_unit=celsius&wind_speed_unit=kmh&forecast_days=3");
// Response: ~1.1KB. StaticJsonDocument<2048> is sufficient.
// current.temperature_2m (float), current.wind_speed_10m (float), current.weather_code (int)
// daily.time[0..2], daily.temperature_2m_max[0..2], daily.sunrise[0..2] ("2026-06-17T20:45")
```

### Pattern 5: open-meteo AQI (separate endpoint, HTTPS)

```cpp
// Source: verified live
// [VERIFIED: live curl test confirmed JSON shape]
// Air quality is a SEPARATE host: air-quality-api.open-meteo.com
http.begin(sc, "https://air-quality-api.open-meteo.com/v1/air-quality"
               "?latitude=39.9075&longitude=116.3972"
               "&hourly=european_aqi,pm2_5&forecast_days=1");
// Returns hourly arrays (24 entries). Extract index matching current UTC hour.
// european_aqi: integer EAQI index. StaticJsonDocument<4096> for 24 hours.
```

### Pattern 6: LVGL v8 candlestick via lv_chart BAR + draw event

```cpp
// Source: LVGL v8 docs — LV_USE_CHART=1 confirmed in lv_conf.h
// [VERIFIED: lv_conf.h grep — LV_USE_CHART 1]
lv_obj_t* chart = lv_chart_create(parent);
lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
lv_chart_set_point_count(chart, 48);
// Use lv_chart_set_range to set Y min/max from kline data
lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, (lv_coord_t)min_price, (lv_coord_t)max_price);
// One series per OHLC field, OR use single series for close + draw event for wicks
lv_chart_series_t* ser = lv_chart_add_series(chart, lv_color_white(), LV_CHART_AXIS_PRIMARY_Y);
// Intercept per-bar drawing to add wick lines and body color:
lv_obj_add_event_cb(chart, chart_draw_event_cb, LV_EVENT_DRAW_PART_BEGIN, ohlc_data);
// In the callback: dsc->part == LV_PART_ITEMS, dsc->id = bar index
// Use lv_draw_line() for high/low wicks, color the bar green/red based on open vs close
```

Memory: a 48-point chart with 1 series = 48 * sizeof(lv_coord_t) = 48 * 2 = 96 bytes of series data in LVGL heap. Negligible.

### Pattern 7: .ss decoder C port entry point

```cpp
// Source: SS-ALGORITHM-SPEC.md (transcribed from decompiled ss_render.c)
// [CITED: hardware/recovered-assets/SS-ALGORITHM-SPEC.md]
// 8-byte header: magic[2]="SN" (0x53,0x4E), width[2 LE], height[2 LE], flag[1]=0x01, hs_params[1]=0xA4
// flag=1 → LZSS path. hs_params=0xA4 → W=10 bits (window), L=4 bits (length)
// LZSS → RLE of RGB565 pixels → push to display line-by-line
// On CYD: display is RGB565 native; skip RGB565→RGB888 conversion,
//         push raw RGB565 line buffers directly to the LVGL/TFT driver.
// Line buffer: width * 2 bytes = 320 * 2 = 640 bytes (stack-safe, allocate once)
// LZSS window: 2^W = 1024 bytes (10-bit window)
// Total decode RAM: 640 (line) + 1024 (window) = 1664 bytes
bool ss_decode_frame(File& f, lv_obj_t* canvas_or_null);
// If canvas_or_null is NULL: decode to display directly (screensaver mode)
// If non-NULL: decode to canvas buffer (test mode)
```

**Gap flagged:** The signature `void ss_render(char* buf, uint w, uint h)` is inferred from call-shape (SS-ALGORITHM-SPEC.md §5 "Confidence note"). Phase 5 must not attempt to call the stock firmware symbol directly — it must port the algorithm to a new C function following the spec.

### Pattern 8: Idle → screensaver state machine (extends Phase 4)

Phase 4's `nm_uimanager.cpp` already implements:
- `s_last_input_ms` — reset on any touch/button
- `s_dimmed` — set when `(millis() - s_last_input_ms) > NM_UI_IDLE_MS`
- `nm_display_dim()` / `nm_display_wake()` — already wired

Phase 5 adds a **second threshold** after dim:

```
States:
  ACTIVE  → (idle > ScreenSaver timeout) → DIM_PENDING or SCREENSAVER_ACTIVE
  SCREENSAVER_ACTIVE → (touch) → ACTIVE (reset timer, nm_display_wake())
  SCREENSAVER_ACTIVE → (FindMe trigger) → FINDME_OVERLAY (FindMe draws on top)
  FINDME_OVERLAY → (FindMe fade-out) → SCREENSAVER_ACTIVE or ACTIVE
```

Implementation: extend `nm_uimanager_tick()` with a second millis() threshold check.
- If `ScreenSaver == "never"` → skip screensaver entirely (only dim applies).
- If `ScreenSaverMode == "black"` → call `nm_display_off()` (backlight 0) instead of carousel.
- If `ScreenSaverMode == "image"` → iterate `/ss/saver_320_240_NNN.ss` files from LittleFS.

The `NM_UI_IDLE_MS` macro currently at 60s governs dim. The screensaver timeout is a runtime setting (`ScreenSaver` pref). These are independent thresholds: dim fires first (60s), screensaver fires at user-configured timeout (which may equal or exceed dim time).

**Key wiring point:** `nm_uimanager_on_touch()` already calls `nm_display_wake()` and returns true when dimmed. Phase 5 must also wake from screensaver state — the same function needs to check the screensaver state flag and call `ss_stop_carousel()`.

### Pattern 9: WebServer multipart upload (.ss upload)

```cpp
// Source: Arduino ESP32 WebServer class (built-in)
// [ASSUMED] — WebServer::onFileUpload pattern from Arduino ESP32 framework training knowledge
// The WebServer upload() method provides chunked access during multipart POST.
apiServer.on("/api/update/screensaver", HTTP_POST,
    []() { /* completion handler — runs after all chunks received */
        apiServer.send(200, "application/json", "{\"status\":\"ok\",\"path\":\"/ss/saver_320_240_003.ss\"}");
    },
    []() { /* upload handler — called per chunk */
        HTTPUpload& upload = apiServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            // Validate magic 0x4E53 in first chunk
            if (upload.buf[0] != 0x53 || upload.buf[1] != 0x4E) { /* reject */ return; }
            // Open LittleFS file for writing
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            // Append chunk to open file
        } else if (upload.status == UPLOAD_FILE_END) {
            // Close file; delete oldest if space was needed
        }
    }
);
```

**Port conflict note:** The WiFiManager captive portal also uses a `WebServer` instance. In the existing fork, `webApiSetup()` starts the custom `apiServer` on port 80 after WiFi connects. WiFiManager's AP-mode server runs on a DIFFERENT IP (192.168.4.1) in AP mode, so there is no port conflict in normal operation. During config mode, the custom `apiServer` is not active (WiFiManager owns the interface). No changes needed.

### Pattern 10: Stock API settings (NVS + Preferences)

The fork's `TSettings` struct in `storage.h` does NOT include stock fields (MainCoin, WatchCoins, ScreenSaver, etc.). Phase 5 must add a `NMSettings` struct (or extend `TSettings`) stored in a dedicated Preferences namespace (e.g., `"nm_stock"`):

```cpp
// New NVS namespace alongside nm_uimanager's "nm_ui"
// Keys matching stock API field names exactly:
// "MainCoin", "WatchCoins", "KlineInterval", "KlineRotate", "PricePageMode"
// "ScreenSaver", "ScreenSaverMode", "Brightness", "RotateScreen", "LedEnable"
// "Timezone", "TimeFormat", "DateFormat"
// "Hostname", "PrimaryPool", "PrimaryAddress", etc.
// GET handler: Preferences.getString("MainCoin", "BTC")
// POST handler: Preferences.putString("MainCoin", val)
```

### Anti-Patterns to Avoid

- **Parsing exchangeInfo for pairs list:** `/api/v3/exchangeInfo` returns 18MB — fatal on ESP32. Use `/api/v3/ticker/price` (152KB) and filter by "USDT" suffix.
- **DynamicJsonDocument for small payloads:** Use `StaticJsonDocument<N>` where N is known small (weather ~2KB, ticker row ~256B). Reserve `DynamicJsonDocument` for klines (size varies with `limit`).
- **`http.getString()` for large responses:** For klines (9KB+) and ticker/price (152KB), use `http.getStream()` with stream-based `deserializeJson()` to avoid double-buffering.
- **Calling LVGL API from the market/weather background task:** All `lv_*` calls must happen on the LVGL thread. Use a double-buffer: background task writes to global struct, nm_uimanager_tick() reads and calls lv_label_set_text_fmt.
- **New FreeRTOS tasks for market/weather:** Each task costs 4-13 KB stack RAM on a no-PSRAM device. Use loop-cadence millis()-gated calls in the existing Monitor task (Core 1) instead — same pattern as `getBTCprice()` in monitor.cpp.
- **lv_canvas for the kline chart:** `lv_canvas_create()` requires a pre-allocated pixel buffer the size of the canvas (e.g., 320x212 = 67840 bytes for RGB565). That is ~66KB from the 32KB LVGL heap — impossible. Use `lv_chart` + draw events instead.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| HTTP GET JSON parse | Custom HTTP + JSON parser | `HTTPClient` + `ArduinoJson` stream | Already in fork; ArduinoJson handles escape/UTF-8 edge cases |
| OHLC chart rendering | Manual pixel-by-pixel draw | `lv_chart` + `LV_EVENT_DRAW_PART_BEGIN` | LVGL manages clipping, dirty-rect, refresh |
| Multipart file upload | Custom MIME parser | `WebServer::onFileUpload` callback | Handles chunking, boundary parsing, content-length |
| LittleFS file iteration | Manual FAT traversal | `LittleFS.openDir("/ss/")` + `dir.next()` | Correct; do not reimagine it |
| NVS key-value storage | EEPROM or custom flash writes | `Preferences` namespace | Wear-leveled, already used in `nm_uimanager.cpp` |
| LZSS bitstream decode | Bit manipulation from scratch | Direct port of SS-ALGORITHM-SPEC.md | Spec is already validated against 17 live files |

**Key insight:** The `lv_canvas` pixel buffer would require ~66KB for a 320x212 surface — more than twice the LVGL heap (32KB). `lv_chart` with draw events uses only 96 bytes of series data.

---

## Common Pitfalls

### Pitfall 1: exchangeInfo is 18MB — instant OOM crash

**What goes wrong:** Code calls `/api/v3/exchangeInfo` to get the pairs list. The 18MB response overflows ArduinoJson and/or heap entirely; device crashes.
**Why it happens:** Stock firmware likely builds its pairs list from `/api/v3/ticker/price` (152KB, manageable) or caches it differently.
**How to avoid:** Use `/api/v3/ticker/price`, stream-parse it, filter for "USDT" suffix, store only the base asset strings. The 670 USDT base symbols as a comma-separated string fit in ~5KB.
**Warning signs:** OOM exception or WDT reset shortly after the pairs fetch.

### Pitfall 2: open-meteo AQI index off by current hour

**What goes wrong:** Code reads `hourly.european_aqi[0]` expecting the current AQI. But the hourly array starts at midnight UTC; index 0 is the 00:00 reading.
**Why it happens:** The API returns 24 hourly values for the day. Current AQI = array index matching the current UTC hour.
**How to avoid:** Parse `hourly.time[i]` until it matches the current hour, or compute `index = (utc_epoch % 86400) / 3600`.

### Pitfall 3: LVGL lv_label_set_text from background task crashes

**What goes wrong:** Market task calls `lv_label_set_text(s_price_label, "$65000")` from a FreeRTOS task. LVGL is not thread-safe; heap corruption or invalid draw follows.
**Why it happens:** LVGL v8 has no internal mutex. All widget mutations must happen on the same thread that calls `lv_timer_handler()`.
**How to avoid:** Background tasks write to shared globals (protected by a flag or atomic). `nm_uimanager_tick()` / page `update()` reads the globals and calls LVGL.

### Pitfall 4: ss_render signature mismatch — don't call the stock function

**What goes wrong:** Code attempts to call `FUN_400f6ff4` from the stock binary as `ss_render(buf, w, h)`. The stock binary is not linked; this is a different firmware.
**Why it happens:** Confusion between RE-documented stock behavior and what the fork actually compiles.
**How to avoid:** Phase 5 PORTS the algorithm to a new C function. The signature `void ss_render(char* buf, uint w, uint h)` is the MODEL, not a linkable symbol.

### Pitfall 5: Two WebServer instances on port 80

**What goes wrong:** WiFiManager creates its own `WebServer(80)` in AP mode; the fork's `apiServer` also binds port 80. Conflict.
**Why it happens:** Both use the same port for different operational modes.
**How to avoid:** The existing design already handles this — `webApiSetup()` only runs after WiFi connects (station mode). During AP/config mode, the custom apiServer is not started. Verify `webApiSetup()` is called AFTER `init_WifiManager()` returns with station-mode WiFi — which it already is in `NerdMinerV2.ino.cpp`.

### Pitfall 6: Screensaver timer fires during page switch

**What goes wrong:** Screensaver activates mid-way through `nm_uimanager_goto_page()`, calling `lv_scr_load()` on the screensaver while another screen is being built.
**Why it happens:** `nm_uimanager_tick()` checks the idle threshold; if a page switch is slow, the threshold may fire.
**How to avoid:** Set a `s_nav_in_progress` flag in `nm_uimanager_goto_page_ex()`, check it before activating screensaver. The screensaver is not a page in the carousel — it overlays the current page or replaces it in a separate LVGL screen.

### Pitfall 7: klines stream-parse with non-zero start offset

**What goes wrong:** `deserializeJson(doc, http.getStream())` fails or gets empty array because the stream offset is wrong after checking `http.GET()`.
**Why it happens:** Some WiFiClientSecure paths consume bytes during handshake, leaving the stream at a non-zero position.
**How to avoid:** Call `http.getStream()` immediately after `http.GET()` returns 200 — no intermediate `http.getString()` calls. Use `HTTPClient` plain (not Secure) for Binance since it's HTTP:80; the secure client preamble issue does not apply.

---

## Binance API — Verified Details

[VERIFIED: live curl test — `curl -sI http://data-api.binance.vision/api/v3/ticker/price` returned `HTTP/1.1 200 OK`]

| Endpoint | Method | Path | Key Params | Response Shape |
|----------|--------|------|-----------|---------------|
| Klines | GET | `/api/v3/klines` | `symbol`, `interval`, `limit` (max 1000, default 500) | Array of 12-element arrays. [0]=open_ms, [1]=open_str, [2]=high_str, [3]=low_str, [4]=close_str |
| 24hr ticker multi | GET | `/api/v3/ticker/24hr` | `symbols=["BTCUSDT","ETHUSDT"]` (URL-encoded JSON array) | Array of objects: `symbol`, `lastPrice`, `priceChange`, `priceChangePercent`, `volume` |
| All prices | GET | `/api/v3/ticker/price` | none (or `symbol=X` for single) | Array of `{symbol, price}`. Use to build pairs list by filtering "USDT" suffix. 152KB total. |
| Exchange info | GET | `/api/v3/exchangeInfo` | `symbols=[]` | 18MB full response — DO NOT use on ESP32 |

**Rate limits:** Weight 2 per `/api/v3/klines` call. No auth required. Server returns 429 when weight budget exceeded; the `X-MBX-USED-WEIGHT-(intervalNum)(intervalLetter)` header tracks usage. At weight 2 per klines call, even polling every 30 seconds is well within limits.

**TLS:** HTTP:80 works (confirmed live). HTTPS:443 also works. D-51 says use HTTP:80 — correct, no `WiFiClientSecure` needed for Binance calls. This frees the secureClient for open-meteo use.

**KlineInterval token mapping** (NVS string → API param): direct pass-through — the stock setting strings (`"1m"`, `"5m"`, `"15m"`, `"30m"`, `"1h"`, `"4h"`, `"1d"`, `"1w"`) are valid Binance interval enum values.

---

## open-meteo API — Verified Details

[VERIFIED: live curl test confirmed JSON structure]

| Endpoint | Host | Auth | Key Params |
|----------|------|------|-----------|
| Forecast | `https://api.open-meteo.com/v1/forecast` | None | `latitude`, `longitude`, `current=temperature_2m,relative_humidity_2m,wind_speed_10m,surface_pressure,weather_code`, `daily=temperature_2m_max,temperature_2m_min,weather_code,sunrise,sunset`, `temperature_unit`, `wind_speed_unit`, `forecast_days=3` |
| Air quality | `https://air-quality-api.open-meteo.com/v1/air-quality` | None | `latitude`, `longitude`, `hourly=european_aqi,pm2_5`, `forecast_days=1` |

**Temperature unit values:** `celsius` (default) or `fahrenheit`. Maps directly to stock `WeatherTempUnit` setting values.

**Wind speed unit values:** `kmh` (default), `ms`, `mph`, `kn`. Maps directly to stock `WeatherSpeedUnit` values.

**Sunrise/sunset format:** ISO 8601 string `"2026-06-17T20:45"` — UTC. Must offset by timezone for display.

**AQI extraction:** `hourly.european_aqi` is a 24-element array. Current value = `hourly.european_aqi[utc_hour]`.

**Response sizes:** Forecast ~1.1KB, AQI hourly 24h ~1.2KB. Both fit in `StaticJsonDocument<3072>`.

**HTTPS required:** Both endpoints. Use `WiFiClientSecure` with `setInsecure()`, same as existing `secureClient` in `monitor.cpp`.

---

## .ss Decoder — Completeness Assessment

[CITED: hardware/recovered-assets/SS-ALGORITHM-SPEC.md]

The spec is **complete enough for a direct C port.** No live-device capture needed.

| Component | Status | Notes |
|-----------|--------|-------|
| Header layout (8 bytes) | Complete | magic[2], width[2 LE], height[2 LE], flag[1], hs_params[1] — exact offsets and values for all 17 files |
| LZSS bit-stream decode | Complete | W=10 bits, L=4 bits from `hs_params=0xA4`; flag-bit → back-ref (offset+length) or literal; overlapping copy rule documented |
| RLE layer decode | Complete | b&0x80 = literal run, else repeat run; RGB565 LE two-byte read |
| RGB565→RGB888 | Complete (with note) | Bit-replication formula verified. On CYD, skip — push RGB565 directly to TFT driver |
| Dimension bounds | Complete | `height-1 <= 599`, `width <= 800`; `width != 0` |
| Centering when smaller | Documented | `(w-width)/2`, `(h-height)/2` offsets |
| LZSS window size | Known | 2^10 = 1024 bytes |
| Argument signature | Inferred (ASSUMED) | `void ss_render(char* buf, uint w, uint h)` — call-shape inference from decompile, not confirmed linkage |

**One open item:** The signature is the working hypothesis. The Phase 5 port defines its own function (`nm_ss_decode()`) following the algorithm, so the inferred signature is informational only.

**17 .ss files confirmed present** in `hardware/recovered-assets/littlefs/` (saver_320_240_002 through _018, excluding some numbers). All 17 available for LittleFS upload and carousel testing.

---

## Flash Budget Analysis

[VERIFIED: `ls -la firmware.bin` + `size firmware.elf` run on current build]

| Item | Value |
|------|-------|
| Current firmware.bin | 2,754,736 bytes |
| huge_app partition | 3,145,728 bytes (3MB) |
| Current usage | 87.6% |
| Current headroom | 390,992 bytes (381 KB) |

**Phase 5 estimated code additions:**

| Component | Est. code size |
|-----------|---------------|
| nm_market.cpp (fetch + parse + page-gating) | ~5 KB |
| nm_weather.cpp (2 endpoints + parse) | ~4 KB |
| nm_screen_sprice.cpp replacement (kline + pricewall) | ~5 KB |
| nm_screen_weather.cpp replacement (display layout) | ~4 KB |
| nm_screensaver.cpp (.ss decoder + carousel state) | ~4 KB |
| nm_swarm.cpp (LAN scan + FindMe overlay) | ~3 KB |
| nm_settings.cpp (NVS get/put for stock fields) | ~3 KB |
| webapi.cpp additions (13 new endpoints) | ~4 KB |
| **Total estimate** | **~32 KB** |

**Post-Phase-5 headroom estimate: ~349 KB (11.1% of 3MB)** — LOW flash risk under normal coding discipline.

**If flash overflows (mitigations in priority order):**
1. Disable unused LVGL widgets in `lv_conf.h` (e.g., `LV_USE_CHART` if pricewall-only chosen, `LV_USE_SPINNER`, extra font sizes).
2. Gate AQI fetch behind a compile flag (save ~1-2 KB).
3. Reduce klines `limit` default (affects only RAM buffer size, not flash).
4. Revisit `LV_MEM_SIZE` — currently 32KB (already minimal for no-PSRAM).
5. If still overflowing: surface to user per D-58. Do NOT silently drop DATA-01/02.

**Note:** `.ss` image files live in **LittleFS**, not app flash. 17 files totaling potentially several MB do NOT impact the 3MB app partition.

---

## Idle → Screensaver State Machine (complete specification)

Phase 4 already implements `ACTIVE → DIM` in `nm_uimanager.cpp`. Phase 5 adds `DIM → SCREENSAVER` and the FindMe override.

```
nm_uimanager_tick() extended state:

                  ┌──────────────────────────────────────┐
                  │              ACTIVE                   │
                  │  (s_dimmed=false, s_ss_active=false) │
                  └──────────┬───────────────────────────┘
                             │ millis() - s_last_input_ms > NM_UI_IDLE_MS (60s default)
                             ▼
                  ┌──────────────────────────────────────┐
                  │               DIM                    │
                  │  (s_dimmed=true, backlight low)       │◄──┐
                  └──────────┬───────────────────────────┘   │
                             │ ScreenSaver != "never" AND      │
                             │ millis() - s_last_input_ms      │
                             │   > ss_timeout_ms               │
                             ▼                                 │
                  ┌──────────────────────────────────────┐    │
                  │          SCREENSAVER ACTIVE           │    │
                  │  (s_ss_active=true)                   │    │
                  │  mode="image": ss_render() carousel   │    │
                  │  mode="black": backlight=0            │    │
                  └──────────┬───────────────────────────┘    │
                             │                                  │
            touch/button     │     /api/swarm/find              │
                 ┌───────────┘     ┌────────────────────────┐  │
                 ▼                 ▼                        │  │
          s_ss_active=false   FINDME OVERLAY               │  │
          nm_display_wake()   (flash screen + LED)         │  │
          return to ACTIVE    fade-out → return to SS      │  │
                                                            │  │
          Any touch/button while in SS → ACTIVE ───────────┘  │
          Any touch/button while in DIM → ACTIVE ─────────────┘
```

**ScreenSaver timeout token → milliseconds mapping:**
- `"never"` → skip screensaver (dim only)
- `"30s"` → 30,000
- `"1m"` → 60,000
- `"5m"` → 300,000
- `"15m"` → 900,000
- `"30m"` → 1,800,000

**Implementation note:** `nm_uimanager_on_touch()` / `nm_uimanager_on_button_wake()` already call `nm_display_wake()` and clear `s_dimmed`. Phase 5 must also clear `s_ss_active` in those same paths before they return.

---

## HTTP API Parity — Endpoint Mapping

The existing `webapi.cpp` has: `/api/status`, `/api/system/info` (alias), `/api/speaker/*`, `/api/sd/*`.

Phase 5 adds these endpoints ADDITIVELY to the same `apiServer`:

| Endpoint | Method | Handler action |
|----------|--------|---------------|
| `/probe` | GET | Return identity JSON with `model="NMMiner"`, `ver`, `hr`, `sbd`, `ebd`, `ut`, `sw=320`, `sh=240` |
| `/alive` | GET | Return `{self: WiFi.localIP(), ips: [...swarm_list]}`, wake swarm scan thread |
| `/api/system/info` | GET | **Replace** current stub with FULL stock field set: identity, miner, stratum, temps, storage |
| `/api/setting/network` | GET+POST | NVS get/put `Hostname`, `WiFiSSID`, `WiFiPWD` (never return PW in GET) |
| `/api/setting/mining` | GET+POST | NVS get/put pool URLs/addresses/passwords |
| `/api/setting/time` | GET+POST | NVS get/put `Timezone`, `TimeFormat`, `DateFormat`; apply immediately |
| `/api/setting/preference` | GET+POST | NVS get/put `Brightness`, `RotateScreen`, `LedEnable`, `ScreenSaver`, `ScreenSaverMode`; apply immediately |
| `/api/setting/market` | GET+POST | NVS get/put `MainCoin`, `WatchCoins`, `KlineInterval`, `KlineRotate`, `PricePageMode`; trigger market refresh on POST |
| `/api/setting/weather` | GET+POST | NVS get/put `WeatherCity`, `WeatherLat`, `WeatherLon`, `WeatherTempUnit`, `WeatherSpeedUnit`, `WeatherAltMode`; trigger weather refresh on POST |
| `/api/market/pairs` | GET | Return `g_pairs_list` if on Miner/Clock/Price page, else `[]` |
| `/api/weather/refresh` | POST | Trigger weather re-fetch; 429 if < 60s since last refresh |
| `/api/swarm/find` | POST | Set FindMe flag, return `{status:"ok"}` |
| `/api/system/restart` | POST | `delay(600); ESP.restart()` |
| `/api/update/screensaver/preflight` | GET | Check LittleFS free space vs `?size=`; return action/overwriteCount |
| `/api/update/screensaver` | POST | Multipart upload; validate magic 0x4E53; write to `/ss/saver_320_240_NNN.ss` |

**POST response convention (stock):** Text `"OK"` (text/plain) for settings endpoints. JSON `{status:"ok"}` for action endpoints. The existing `sendOk()` returns `{ok:true}` — this must be adjusted to match stock shapes exactly for the action endpoints; settings endpoints return text "OK".

**Partial update convention:** All settings POST handlers accept partial JSON (only include changed keys). `ArduinoJson`'s `doc[key].isNull()` check for optional keys.

---

## Runtime State Inventory

> Phase 5 is additive (no rename/refactor). Included per checklist — all categories verified.

| Category | Items Found | Action Required |
|----------|-------------|-----------------|
| Stored data | NVS namespace `"nm_ui"` (lastpage) already exists. No stock settings keys exist yet. | Phase 5 creates new `"nm_stock"` Preferences namespace |
| Live service config | None — device not yet broadcasting stock API fields | n/a |
| OS-registered state | None | n/a |
| Secrets/env vars | None | n/a |
| Build artifacts | `.pio/build/ESP32-2432S028R/firmware.bin` — stale after edits. Rebuild required. | `pio run -e ESP32-2432S028R` gate |

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `data-api.binance.vision:80` | Market data (D-51) | ✓ | HTTP/1.1 — verified live | — |
| `api.open-meteo.com:443` | Weather forecast (D-53) | ✓ | HTTPS — verified live | — |
| `air-quality-api.open-meteo.com:443` | AQI (D-53) | ✓ | HTTPS — verified live | — |
| LittleFS filesystem | Screensaver files (D-54) | ✓ | `board_build.filesystem = LittleFS` in platformio.ini | — |
| LVGL `lv_chart` | Kline chart (D-52) | ✓ | `LV_USE_CHART 1` in lv_conf.h | — |
| LVGL `lv_canvas` | Optional (not needed — see pitfall) | ✓ | `LV_USE_CANVAS 1` in lv_conf.h | Moot — use lv_chart instead |
| ArduinoJson ^6.21 | JSON parse | ✓ | Already in lib_deps | — |
| PlatformIO `huge_app` partition | Flash headroom | ✓ | 381 KB available | Trim LVGL features if overflow |

**Missing dependencies with no fallback:** None.

---

## Validation Architecture

> `workflow.nyquist_validation` absent from config.json → treated as enabled.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | PlatformIO build gate (`pio run -e ESP32-2432S028R`) |
| Config file | `platformio.ini` — env `ESP32-2432S028R` |
| Quick run command | `cd firmware/custom/NerdMiner-CYD-Custom && pio run -e ESP32-2432S028R 2>&1 | tail -5` |
| Full suite command | same (only build target for this project) |

**Note:** This is a firmware project. There are no host-side unit test files. Validation = build success + on-device flash + Phase 7 behavioral verification. Phase 5 gate is BUILD SUCCESS only (per CONTEXT.md domain block).

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DATA-01 | Binance market fetch + SPrice screen renders kline/pricewall | build | `pio run -e ESP32-2432S028R` | ✅ |
| DATA-02 | open-meteo weather fetch + Weather screen renders | build | `pio run -e ESP32-2432S028R` | ✅ |
| DATA-03 | Screensaver carousel reads .ss from LittleFS after idle | build | `pio run -e ESP32-2432S028R` | ✅ |
| DATA-04 | All 15 REST endpoints respond correctly (API parity) | build (compile) | `pio run -e ESP32-2432S028R` | ✅ |
| DATA-05 | Swarm LAN scan + FindMe overlay override screensaver | build | `pio run -e ESP32-2432S028R` | ✅ |
| DATA-06 | `/probe` + `/api/system/info` return `model="NMMiner"` | build | `pio run -e ESP32-2432S028R` | ✅ |

### Sampling Rate

- **Per task commit:** `pio run -e ESP32-2432S028R 2>&1 | grep -E "ERROR|error:|\.cpp|\.h"` (fast compile check)
- **Per wave merge:** `pio run -e ESP32-2432S028R` (full build; check final binary size vs 3MB limit)
- **Phase gate:** Full build green + binary size reported before `/gsd-verify-work`

### Wave 0 Gaps

None — all required source files either exist (stubs to replace) or are new files with no existing test infrastructure to set up. Build system already configured.

---

## Security Domain

> `security_enforcement` not set in config.json → treated as enabled.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | No auth on local LAN API (matches stock) |
| V3 Session Management | No | Stateless REST |
| V4 Access Control | Partial | LAN-trust model already documented in webapi.cpp comment |
| V5 Input Validation | Yes | `size` param in preflight, path in upload, all settings POST keys |
| V6 Cryptography | No | No new crypto; TLS via setInsecure() (matches stock; self-signed/untrusted is accepted risk for embedded) |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| `.ss` upload with crafted header (fake magic, oversized dims) | Tampering | Validate magic 0x4E53 in first chunk; validate `width <= 800` and `height <= 600` before allocating line buffer |
| LittleFS path traversal via filename | Tampering | Filename is computed by firmware (`saver_320_240_NNN.ss`), never user-supplied |
| JSON injection in settings POST | Tampering | ArduinoJson's key lookup is type-safe; no string interpolation |
| 413 bypass (upload > 200KB) | Tampering | Check `upload.totalSize` in UPLOAD_FILE_START handler; abort and return 413 early |
| `?size=` overflow in preflight | Tampering | Parse as `size_t` with range check before LittleFS space comparison |

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `WebServer::onFileUpload` callback API works for multipart .ss upload | Pattern 9 | If wrong: need alternative multipart parsing; medium rework |
| A2 | `ss_render` signature `(char* buf, uint w, uint h)` — Phase 5 ports the algorithm, does not call this symbol | .ss Decoder | Low risk — Phase 5 writes its own function anyway |
| A3 | Stock firmware uses `/api/v3/ticker/price` (not exchangeInfo) for pairs list | Pattern 3 | If stock uses a different endpoint, pairs list response may differ; LOW functional risk since we control the implementation |
| A4 | Market/weather as loop-cadence functions (not new FreeRTOS tasks) will not miss the 3s WDT | Architecture Patterns | If HTTP calls block > 3s: WDT reset; mitigation = `http.setTimeout(8000)` and verify on device |
| A5 | `DynamicJsonDocument(10240)` heap allocation succeeds at runtime for klines | Code Examples | Heap fragmentation may fail this; mitigation = allocate at task start, reuse |

---

## Open Questions

1. **Weather page static geometry** — SPEC.md marks the Weather320x240 static frame as NOT AVAILABLE (lazy-create code not decompiled). Phase 5 must design the weather layout from scratch using the field set (temp, humidity, AQI, 3-day, sunrise/sunset, wind, pressure). This is the largest open design area.
   - What we know: cb3 time-widget sub-structure (4 slots), 3 update callbacks, field names.
   - What's unclear: exact x/y positions, font sizes, row layout.
   - Recommendation: design a functionally-equivalent layout matching the field set; no exact RE needed since geometry is unrecoverable.

2. **SPrice draw callback for OHLC geometry** — `FUN_400f5ba0` (the per-bar draw callback) is not decompiled.
   - What we know: it's called by `lv_chart` infrastructure; colors should be green (up) / red (down).
   - What's unclear: exact candle body vs wick pixel geometry.
   - Recommendation: standard OHLC candle geometry (body from open to close, wick from low to high).

3. **FindMe overlay implementation** — SPEC identifies `FindMe overrides screensaver` but the overlay mechanism (full-screen flash vs partial overlay) is not decompiled.
   - Recommendation: full-screen white flash + LED blink, fading to previous state, is the conservative match for "screen-flash/LED FindMe overlay."

---

## Sources

### Primary (verified live)

- `http://data-api.binance.vision/api/v3/klines` — live HTTP:80 curl test confirmed shape [VERIFIED]
- `http://data-api.binance.vision/api/v3/ticker/24hr` — live curl test [VERIFIED]
- `https://api.open-meteo.com/v1/forecast` — live curl test confirmed JSON structure [VERIFIED]
- `https://air-quality-api.open-meteo.com/v1/air-quality` — live curl test [VERIFIED]
- `hardware/recovered-assets/SS-ALGORITHM-SPEC.md` — transcribed from decompiled ss_render.c [CITED]
- `hardware/recovered-assets/SPEC.md` — SPrice/Weather/ss_render geometry [CITED]
- `docs/research/nmminer-api-reference.md` — 21-endpoint stock API contract [CITED]
- `.planning/research/FEATURES.md` — data sources, page-gating, screensaver behavior [CITED]

### Secondary (official docs, MEDIUM confidence)

- [Binance Spot API docs — klines endpoint](https://developers.binance.com/docs/binance-spot-api-docs/rest-api/market-data-endpoints) [CITED]
- [open-meteo docs](https://open-meteo.com/en/docs) [CITED]
- [open-meteo air quality docs](https://open-meteo.com/en/docs/air-quality-api) [CITED]
- [LVGL v8 chart docs](https://lvgl.io/docs/open/8/widgets/extra/chart) [CITED]

### Codebase (direct reads)

- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp` — Phase 4 idle timer implementation [VERIFIED]
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/lv_conf.h` — `LV_USE_CHART=1`, `LV_USE_CANVAS=1`, `LV_MEM_SIZE=32KB` [VERIFIED]
- `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp` — existing apiServer routes [VERIFIED]
- `firmware/custom/NerdMiner-CYD-Custom/src/monitor.cpp` — HTTPClient/WiFiClientSecure pattern [VERIFIED]
- `.pio/build/ESP32-2432S028R/firmware.bin` — 2,754,736 bytes / 87.6% of 3MB [VERIFIED]

---

## Metadata

**Confidence breakdown:**
- Binance API shape: HIGH (live-tested endpoints)
- open-meteo API shape: HIGH (live-tested)
- .ss decoder completeness: HIGH (spec from decompile, 17 live test files available)
- LVGL chart approach: MEDIUM (docs verified, no build-test yet)
- Flash budget: MEDIUM (based on estimation; actual measured at build time)
- WebServer multipart upload pattern: LOW/ASSUMED (from training knowledge — verify at implementation)

**Research date:** 2026-06-18
**Valid until:** 2026-07-18 (APIs are stable; LVGL v8 is frozen spec)
