# Feature Research

**Domain:** ESP32 CYD (ESP32-2432S028R) solo-BTC miner — recovering ORIGINAL NMMiner v2.0.02 on-device behavior
**Researched:** 2026-06-17
**Confidence:** HIGH for catalogued items (grounded in firmware.bin strings + saved API reference + vendor manual). MEDIUM for exact default values where the binary did not expose a literal (flagged inline).

## Scope

This catalogs the ORIGINAL closed NMMiner v2.0.02 features that the rebuilt firmware must reproduce so the device "feels identical to stock." Sources, in priority order:
1. Recovered stock binary `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin` (string-extracted; authoritative for screen class names, internal labels, data sources). [REF: firmware.bin]
2. `docs/research/nmminer-api-reference.md` (21-endpoint HTTP API, field semantics, defaults). [REF: docs/research/nmminer-api-reference.md]
3. `docs/research/nmminer-github-readme.md` (release log, button controls, setup flow). [REF: docs/research/nmminer-github-readme.md]
4. `docs/vendor/NM-CYD-ESP32-2432S028-manual.txt` (button mapping, config flow, pools). [REF: docs/vendor/...manual.txt]
5. `docs/customization-audit.md` (live-device readbacks, LittleFS contents). [REF: docs/customization-audit.md]

**Confirmed on-device screen classes (from binary):** `Loading320x240`, `Miner320x240`, `Clock320x240`, `SPrice320x240`, `Weather320x240`, `Config320x240`. A `UIManager` registers these as a page list with `LastPage` persistence and `goto_page(index)`. [REF: firmware.bin: "Loading320x240","Miner320x240","Clock320x240","SPrice320x240","Weather320x240","Config320x240","UIManager: %d page(s) registered","LastPage"]

---

## On-Device Screens (the page carousel)

The device rotates through a fixed set of LVGL pages. Touch (short) advances to the next page; the page list and last page are persisted. These are the visual identity the user wants to match exactly.

| Screen class | What it displays | Key behaviors | Data source |
|---|---|---|---|
| `Loading320x240` | Boot/loading splash, branding "NMMiner / Make it better", firmware version, connection progress, share-by-share boot log | Shown on boot; gradual brightness ramp to avoid ST7789 distortion (per readme v1.8.20); new-firmware-available reminder | local |
| `Miner320x240` | Live mining stats: hashrate, share R/A, net diff, pool diff, last diff, best diff (session/all-time), block hits, freeheap, temp(real-time clock, no sensor), RSSI, pool URL | Background LAN swarm scan runs ONLY on this page; `/alive` wakes it with 30s idle timer | local + swarm |
| `Clock320x240` | Real-time clock (NTP), date, BTC price w/ green-up/red-down coloring, WiFi signal | Clock style "remove second"; 12/24h + date format user-selectable; price color = direction | NTP + Binance |
| `SPrice320x240` | Price page: K-line candlestick chart OR price-wall (multi-coin watchlist) | `PricePageMode` toggles kline↔pricewall; K-line carousel rotates pairs; market thread runs on Miner/Clock/Price pages only | Binance |
| `Weather320x240` | Current conditions, temp, AQI, 3-day forecast, sunrise/sunset, wind, pressure/altitude | Temp unit °C/°F; immediate refresh on settings save; `/api/weather/refresh` rate-limited 60s | open-meteo.com |
| `Config320x240` | On-device config-mode indicator (AP `nmap-2.4g`, 192.168.4.1) | Entered via long-press boot (10s) or 15s WiFi-connect failure | local |

---

## Feature Landscape

### Table Stakes (Must Match Stock Exactly)

Reproducing these is required for the rebuild to "feel like stock." Each is observable on-device or via the documented HTTP API.

| Feature | Stock behavior (testable) | Complexity | Notes / source |
|---|---|---|---|
| Page carousel + class names | 6 screens above; short touch = next page; last page restored after reboot/crash | HIGH | Binary class names are the contract. [REF: firmware.bin] [readme v1.8.20 "Resumes on last page"] |
| Touch: short = next, long = config | Short touch advances page; long-press (10s) enters config mode; tap wakes a sleeping screen | MEDIUM | [manual §2.2/§2.3] [readme v2.0.01 "Tap to switch; long-press 10s config"] |
| Boot button controls | Double-click = next page; long-press = force config; single-click = wake screen | LOW | [manual §2.2] [readme Button Controls] |
| Miner stats screen content | All fields in `/api/system/info.miner` must render: hashRate, sAccepted/sRejected, net/pool/last/best diff (session + all-time), blkhits, freeHeap, RSSI, temp | HIGH | [api-ref /api/system/info] — share UI is "session best/all-time best" when save-status on, else "session last/session best" [readme v1.7.05] |
| Clock screen | NTP time, configurable 12/24h + date format, BTC price with green-up/red-down | MEDIUM | [api-ref /api/setting/time] [readme v1.7.06 price color] |
| Screensaver (image carousel) | After idle timeout, render `.ss` images from `/ss/saver_320_240_NNN.ss` in carousel; resume on touch | HIGH | `ss_render` RGB565+RLE/Huffman, magic `0x4E53`. [firmware.bin "ss_render","/ss/%s%03d.ss","saver_%u_%u_"] |
| Screensaver: black mode | `ScreenSaverMode: "black"` blanks screen instead of images | LOW | New in v2.0.02. [api-ref preference] |
| Screensaver timeout options | `ScreenSaver`: `never`/`30s`/`1m`/`5m`/`15m`/`30m`; applies immediately, no reboot | LOW | [api-ref preference] — tokens `never/30s/1m/...` present in binary |
| Brightness (1–100), immediate | `Brightness` applies live without reboot; gradual ramp on boot; min-brightness must still light screen | MEDIUM | [api-ref preference] [readme v2.0.01 "apply immediately"; v1.8.20 ramp] |
| Screen rotation, immediate | `RotateScreen` ∈ {0,90,180,270}; applies live | LOW | [api-ref preference] [firmware.bin "RotateScreen","ScreenOrient"] |
| LED status enable/disable | `LedEnable` 0/1; requires reboot; status indicator behavior | LOW | [api-ref preference] [readme v1.3.01 LED option] |
| Market: K-line chart | Candlestick chart for `MainCoin`; `KlineInterval` ∈ {1m,5m,15m,30m,1h,4h,1d,1w}; `KlineRotate` carousel ∈ {never,10s,20s,60s,5m,30m} | HIGH | Binance klines API. [api-ref market] [firmware.bin binance.vision /klines] |
| Market: price-wall | Multi-coin watchlist (`WatchCoins`, ≤20 symbols) ticker display | MEDIUM | [api-ref market] [firmware.bin /ticker/24hr?symbols=] |
| Market data source | Binance `data-api.binance.vision:80` — /ticker/price, /ticker/24hr, /klines; pairs list via /api/market/pairs | MEDIUM | [firmware.bin] — exact host matters for behavior parity |
| Market thread gating | Market thread sleeps unless on Miner/Clock/Price page; `/api/market/pairs` returns `[]` otherwise | MEDIUM | [api-ref /api/market/pairs note] |
| Weather page + data source | open-meteo.com forecast (temp, humidity, AQI, 3-day, sunrise/sunset, wind, pressure); °C/°F, speed unit, pressure/altitude mode | HIGH | [firmware.bin open-meteo URL] [api-ref weather] |
| Swarm: LAN discovery | Background ping scan (Miner page only) aggregates reachable miners; `/alive` returns self + dedup IPs | HIGH | [api-ref /alive] [firmware.bin "(swarm) new scan"] |
| Swarm "find me" | `/api/swarm/find` triggers screen-flash/LED overlay to identify the unit; FindMe overrides screensaver | MEDIUM | [firmware.bin "(findme) overlay shown/fade-out/removed","FindMe overrides screensaver"] |
| Config / AP mode | On WiFi fail (15s) or long-press, opens AP `nmap-2.4g` (no pw), captive portal at 192.168.4.1; save → 10–15s auto-restart | MEDIUM | [manual §2] [firmware.bin "nmap-2.4g","192.168.4.1"] |
| 21-endpoint HTTP API | All endpoints in api-reference must exist with identical paths/shapes (see API table below) | HIGH | [api-ref] — this is the controller/web contract |
| Branding strings | "NMMiner", "Make it better", `hwModel:"NMMiner"`, `model:"NMMiner"` in /probe and /api/system/info | LOW | [firmware.bin] — controllers key off `model="NMMiner"` |
| Pool / mining core | Stratum solo BTC; primary+fallback pool; switch-back to primary within 10s when it recovers | HIGH | [readme v1.7.05] [api-ref /api/setting/mining] |
| NVS persistence | Best diff, block-hit counter, uptime-ever, last page, all settings survive reboot; reduced NVS write frequency | MEDIUM | [readme v2.0.01] [api-ref uptimeEver] |

### Differentiators (Nice-to-Have / Polish — match if cheap)

These exist in stock but are lower-priority for "feels identical." Reproduce after table stakes.

| Feature | Value | Complexity | Notes |
|---|---|---|---|
| New-firmware-available reminder | Shown on loading + miner page | LOW | [readme v1.6.03] — cosmetic; can stub |
| Christmas surprise (Dec 25) | Easter-egg, exit via touch | LOW | [readme v1.8.23] — pure novelty |
| Auto timezone | Derives TZ automatically | MEDIUM | [readme v1.7.02] — manual TZ is the table-stakes path |
| Dark/clock-page styling variants | Dark clock style; "remove second" | LOW | [readme v1.7.02/v1.8.23] |
| Pool URL shown on clock page | Displays active pool on clock screen | LOW | [readme v1.6.03] |
| Browser cache strategy | Reduces web-server crash under frequent polling | MEDIUM | [readme v2.0.02] — stability polish |
| Web-frontend capability gating | Frontend hides display settings the board lacks | MEDIUM | [readme v2.0.02] — only matters if reusing stock index.html |

### Anti-Features (Do NOT Reproduce)

| Feature | Why it seems wanted | Why problematic | Alternative |
|---|---|---|---|
| License/activation gate | Stock requires per-device license | Closed, server-dependent, blocks our own builds | Omit entirely; our build is unlicensed |
| SD/TF-card functionality from stock | Manual mentions TF slot | Stock explicitly reserves TF/external I/O for "next version" — NO SD path exists in v2.0.02 | Our SD module is a NEW feature (milestone re-add), not a stock recovery target [REF: customization-audit "no SD-card customization path"] |
| Patch stock firmware.bin directly | Fastest path to "exact" | Binary-only, no source; high brick risk; not maintainable | Reimplement behavior from this catalog [REF: customization-audit high-risk] |
| Non-Binance / non-open-meteo data sources | "Better" APIs | Breaks parity; stock users expect identical numbers/cadence | Match Binance + open-meteo exactly |
| Multi-board abstraction | Stock supports 30+ boards | Out of scope; we target ONE board (esp32-2432s028r-st7789) | Hardcode for this board |

---

## Existing Stock HTTP API (21 endpoints — must preserve)

| # | Method | Endpoint | Purpose |
|---|---|---|---|
| 1 | GET | `/probe` | Identity probe (model, ver, hr required) for controller discovery |
| 2 | GET | `/alive` | Self IP + swarm dedup IP list; wakes scan thread (30s idle) |
| 3 | GET | `/api/system/info` | Full snapshot: identity, miner stats, stratum, temps, storage |
| 4 | G/P | `/api/setting/network` | Hostname + WiFi creds (pw never returned; change needs reboot) |
| 5 | G/P | `/api/setting/mining` | Primary/secondary pool URL/address/password (reboot to apply) |
| 6 | G/P | `/api/setting/time` | Timezone, TimeFormat 12/24, DateFormat (immediate) |
| 7 | G/P | `/api/setting/preference` | Brightness, RotateScreen, LedEnable, ScreenSaver, ScreenSaverMode |
| 8 | G/P | `/api/setting/market` | MainCoin, WatchCoins(≤20), KlineRotate, KlineInterval, PricePageMode |
| 9 | G/P | `/api/setting/weather` | City, lat/lon, temp/speed unit, alt mode |
| 10 | GET | `/api/market/pairs` | Cached USDT base symbols (empty unless on Miner/Clock/Price) |
| 11 | POST | `/api/weather/refresh` | Force weather+AQI fetch; 429 rate-limited 60s |
| 12 | POST | `/api/swarm/find` | "Find me" screen-flash/LED; returns `{status:ok}` |
| 13 | POST | `/api/system/restart` | Reboot (~600ms after 200 response) |
| 14 | GET | `/api/update/screensaver/preflight` | Space check (?size=); action new/overwrite/reject |
| 15 | POST | `/api/update/screensaver` | Upload `.ss` (multipart, ≤200KB, magic 0x4E53) |

Plus captive-portal/WiFiManager routes served in config mode (not REST): `/`, `/wifi`, `/wifisave`, `/param`, `/info`, `/u`(OTA), `/close`, `/exit`, `/restart`, `/erase`. [REF: firmware.bin "Available pages" table] — the 21-endpoint count in the milestone context includes these portal routes alongside the 15 REST endpoints.

---

## Default Values (stock, for parity)

Confirmed/observed defaults to reproduce. Values without a literal in the binary are flagged MEDIUM.

| Setting | Stock default | Confidence | Source |
|---|---|---|---|
| `ScreenSaver` (timeout) | `"never"` (in GET example); legacy "auto off 60s" | HIGH for example value | [api-ref preference GET] |
| `ScreenSaverMode` | `"image"` | HIGH | [api-ref] |
| `Brightness` | `80` | MEDIUM (GET example) | [api-ref] |
| `RotateScreen` | `270` | MEDIUM (GET example) | [api-ref] |
| `LedEnable` | `1` | MEDIUM | [api-ref] |
| `TimeFormat` | `24` | MEDIUM | [api-ref] |
| `DateFormat` | `"YYYY-MM-DD"` | MEDIUM | [api-ref] |
| `MainCoin` | `"BTC"` | HIGH | [api-ref] |
| `WatchCoins` | `"ETH,BNB,SOL"` (example) | MEDIUM | [api-ref] |
| `KlineRotate` | `"20s"` | MEDIUM | [api-ref] |
| `KlineInterval` | `"1h"` | MEDIUM | [api-ref] |
| `PricePageMode` | `"kline"` | MEDIUM | [api-ref] |
| Default pools (flasher) | primary `solobtc.nmminer.com:3333`, fallback `au.solobtc.nmminer.com:3333` | HIGH | [firmware.bin + customization-audit] |
| AP SSID | `nmap-2.4g` (open) | HIGH | [manual + firmware.bin] |
| WiFi-fail → config | 15s | HIGH | [manual §2.1] |
| Long-press → config | 10s | HIGH | [readme v2.0.01] |
| Screensaver max upload | 200 KB | HIGH | [api-ref] |

> The user's live device (customization-audit) was reconfigured to `pool.nerdminers.org:3333` and TZ `-4` — those are the USER'S values, not stock defaults. Stock defaults are the solobtc pair above.

---

## Feature Dependencies (for roadmap phase ordering)

```
[Display / LVGL UIManager + page list]
    └──requires──> [Screen driver: ST7789, SPI MODE3, 320x240]
[Page carousel]
    └──requires──> [Touch input (short=next, long=config) + boot button]
    └──requires──> [LastPage NVS persistence]
[Miner screen]
    └──requires──> [Stratum mining core] ──provides──> [stats: diff/shares/hits]
    └──enhances───> [Swarm scan (Miner-page-gated)] ──exposes──> [/alive, /api/swarm/find]
[Clock + Price + Weather screens]
    └──requires──> [WiFi + NTP]
    └──requires──> [HTTP client] ──> Binance (price/kline) , open-meteo (weather)
[Screensaver]
    └──requires──> [.ss decoder (ss_render: RGB565+RLE/Huffman, magic 0x4E53)]
    └──requires──> [LittleFS /ss/ partition]
    └──requires──> [idle timer + ScreenSaver/ScreenSaverMode prefs]
    └──conflicts──> [FindMe overlay]  (FindMe overrides screensaver)
[HTTP API (21 endpoints)]
    └──requires──> [Web server + settings store (NVS)]
    └──enables───> [NMController web/client, our dashboard]
[Config/AP mode]
    └──requires──> [WiFiManager captive portal]
```

### Dependency notes
- **UIManager page list before any screen:** the binary's `UIManager` registers pages and renders by `UIPageId`; the page-switch/persistence machinery must exist before individual screens are meaningful.
- **Screensaver requires the `.ss` decoder AND LittleFS:** stock ships `/ss/saver_320_240_001.ss` + `_002.ss` in LittleFS; the decoder (`ss_render`) validates magic `0x4E53` and RGB565+RLE/Huffman. Both must be reproduced for upload + carousel to work. [REF: customization-audit LittleFS; firmware.bin ss_render]
- **Swarm scan is Miner-page-gated:** scan thread only runs on Miner page and sleeps after 30s of no `/alive`. Reproducing the gating is part of behavioral parity, not just having the endpoint.
- **Market thread is page-gated:** runs only on Miner/Clock/Price; `/api/market/pairs` returns `[]` otherwise. The web settings dropdowns depend on this.
- **FindMe vs screensaver conflict:** FindMe overlay must visibly override an active screensaver.

---

## MVP Definition (recovery order)

### Launch With (v1 — "boots and looks like stock")
- [ ] ST7789 320x240 driver, SPI MODE3, brightness ramp on boot — without this nothing renders correctly [REF: readme v2.0.02 SPI fix]
- [ ] UIManager page list + 6 screen classes (even if some are stubs) — the structural identity
- [ ] Miner screen with full stat set — the primary stock screen
- [ ] Touch + boot button: short/double = next page, long = config; tap wakes screen
- [ ] Stratum mining core (primary + fallback, switch-back ≤10s)
- [ ] Config/AP mode (`nmap-2.4g`, 192.168.4.1, captive portal)
- [ ] HTTP API: `/probe`, `/alive`, `/api/system/info`, all `/api/setting/*` — controller + dashboard contract
- [ ] LastPage + stats NVS persistence
- [ ] Branding strings (`model="NMMiner"`, "Make it better")

### Add After Core (v1.x)
- [ ] Clock screen (NTP, 12/24h, date format, BTC price coloring)
- [ ] Screensaver: `.ss` decoder + carousel + black mode + idle-timeout prefs + upload/preflight endpoints
- [ ] Market: Binance price/24hr/klines; SPrice screen with kline + pricewall modes
- [ ] Weather: open-meteo screen + `/api/weather/refresh`
- [ ] Swarm: LAN scan (Miner-gated) + `/api/swarm/find` FindMe overlay

### Future / Re-add Layer (our NEW features, post-parity)
- [ ] Speaker module (NOT stock — our addition)
- [ ] SD-card module (NOT stock — stock reserves TF slot)
- [ ] Custom LED behaviors / branding beyond stock
- [ ] Differentiator polish: new-fw reminder, Christmas egg, auto-TZ, dark clock style

---

## Feature Prioritization Matrix

| Feature | User Value (match-stock) | Implementation Cost | Priority |
|---|---|---|---|
| UIManager + 6 screens | HIGH | HIGH | P1 |
| Miner stat screen | HIGH | HIGH | P1 |
| Touch/button page nav + config entry | HIGH | MEDIUM | P1 |
| Stratum core + pools | HIGH | HIGH | P1 |
| HTTP API (status + settings) | HIGH | HIGH | P1 |
| Config/AP captive portal | HIGH | MEDIUM | P1 |
| Screensaver (.ss carousel + black) | HIGH | HIGH | P2 |
| Clock screen | HIGH | MEDIUM | P2 |
| Market (kline + pricewall, Binance) | MEDIUM | HIGH | P2 |
| Weather (open-meteo) | MEDIUM | HIGH | P2 |
| Swarm scan + FindMe | MEDIUM | MEDIUM | P2 |
| New-fw reminder / Xmas / auto-TZ | LOW | LOW | P3 |
| Speaker / SD / custom LED (NEW) | HIGH (user goal) | MEDIUM | P3 (post-parity) |

---

## Competitor / Reference Feature Analysis

| Feature | Stock NMMiner v2.0.02 | Open NerdMiner_v2 fork (our fallback) | Our rebuild approach |
|---|---|---|---|
| Screen identity | 6 named LVGL pages, carousel, screensaver | Different look/feel (user dislikes) | Reproduce stock page set + names |
| Screensaver `.ss` | RGB565+RLE/Huffman, upload API | absent | Reimplement `ss_render` + endpoints |
| Market data | Binance vision API, kline + pricewall | limited/absent | Match Binance endpoints |
| Weather | open-meteo full forecast | absent | Match open-meteo |
| Swarm/FindMe | LAN scan + overlay | partial | Reproduce gating + overlay |
| HTTP API | 21 endpoints | different | Match stock paths/shapes |
| Speaker/SD | absent (TF reserved) | (our custom work exists) | Re-add as NEW layer on stock-parity base |

---

## Open Questions / Gaps for Phase Research

- Exact idle-timeout-to-screensaver state machine (does timeout also dim before blanking?) — not literal in binary; verify on live device.
- Precise stat-field formatting (units, abbreviations like "92.67T", "8.00K") for Miner/Clock screens — capture from live `/api/system/info` and on-device photos.
- `.ss` header layout beyond magic `0x4E53` (hs_params, dimensions, compression params) — needs binary RE of `ss_render` for a byte-accurate decoder.
- K-line rendering specifics (candle count, colors, axis) — observe live or RE the SPrice page.
- LED status semantics (what each state means) per `LedEnable` — observe on device.
- Default `ScreenSaver` timeout on a factory device (`never` vs legacy `60s`) — confirm against a freshly flashed unit, not the user's reconfigured device.

## Sources

- `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin` (string extraction — authoritative for screen classes, internal labels, data-source URLs, ss_render, findme/swarm) [HIGH]
- `docs/research/nmminer-api-reference.md` (21-endpoint API, field semantics, default examples) [HIGH]
- `docs/research/nmminer-github-readme.md` (release log, button controls, setup) [HIGH]
- `docs/vendor/NM-CYD-ESP32-2432S028-manual.txt` (buttons, config flow, pools, TF-slot-reserved note) [HIGH]
- `docs/customization-audit.md` (live readbacks, LittleFS contents, flasher defaults) [HIGH]

---
*Feature research for: NMMiner v2.0.02 original-behavior recovery (ESP32-2432S028R)*
*Researched: 2026-06-17*
