# Phase 5 Validation — Data-Driven Screens + HTTP API Parity

**Gate type:** PlatformIO build success + per-module/per-endpoint presence gates.
This is a firmware project — no host-side unit tests. On-device behavior is Phase 7
(RESEARCH §"Validation Architecture": Phase 5 gate is BUILD SUCCESS only).

**Build command (canonical):**
```
cd firmware/custom/NerdMiner-CYD-Custom && \
  /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R
```

---

## 1. Build Gate (phase blocker)

| Gate | Command | Pass condition |
|------|---------|----------------|
| Full build | `pio run -e ESP32-2432S028R 2>&1 \| tail -6` | `SUCCESS` present, no `error:` |
| Flash budget (D-58) | `ls -la .pio/build/ESP32-2432S028R/firmware.bin` | size recorded; computed headroom vs 3,145,728 bytes; overflow surfaced, never silently drops a DATA req |

Baseline for the budget delta: 2,754,736 bytes / 87.6% of 3MB (RESEARCH §"Flash Budget Analysis").

---

## 2. Requirement → Presence Gate Map

Run from `firmware/custom/NerdMiner-CYD-Custom/`. Each gate filters comments
(`grep -v '^[[:space:]]*[/*]'`) before counting so header prose does not self-satisfy.

| Req | Behavior | Gate command | Pass |
|-----|----------|--------------|------|
| DATA-01 | Binance market fetch + SPrice kline/pricewall | `grep -q data-api.binance.vision src/nm_market.cpp && ! grep -q exchangeInfo src/nm_market.cpp && grep -q lv_chart src/ui_nm/nm_screen_sprice.cpp && ! grep -q lv_canvas_create src/ui_nm/nm_screen_sprice.cpp` | exit 0 |
| DATA-02 | open-meteo weather fetch + Weather screen | `grep -q open-meteo.com src/nm_weather.cpp && grep -q air-quality-api src/nm_weather.cpp && grep -q setInsecure src/nm_weather.cpp && grep -q g_weather src/ui_nm/nm_screen_weather.cpp` | exit 0 |
| DATA-03 | Screensaver carousel decodes .ss from LittleFS | `grep -q LittleFS src/ui_nm/nm_screensaver.cpp && grep -Eq '0x53\|0x4E\|"SN"' src/ui_nm/nm_screensaver.cpp && grep -q s_ss_active src/ui_nm/nm_uimanager.cpp` | exit 0 |
| DATA-04 | Swarm scan + FindMe override + endpoints | `grep -Eq 'nm_swarm_add_ip\|nm_swarm_request_findme' src/nm_swarm.cpp && grep -q '/alive' src/webapi.cpp && grep -q '/api/swarm/find' src/webapi.cpp` | exit 0 |
| DATA-05 | Stock API parity, additive | `grep -q '"/probe"' src/webapi.cpp && grep -c 'api/setting/' src/webapi.cpp \| awk '$1>=6' && grep -q 'api/speaker' src/webapi.cpp && grep -q '/api/update/screensaver' src/webapi.cpp` | exit 0 |
| DATA-06 | Branding model=NMMiner | `grep -q '"/probe"' src/webapi.cpp && grep -q 'NMMiner' src/webapi.cpp` | exit 0 |

---

## 3. Page-Gating & State-Machine Presence Gates

| Check | Gate command | Pass |
|-------|--------------|------|
| Market page-gated | `grep -q nm_market_set_page_allowed src/ui_nm/nm_uimanager.cpp` | exit 0 |
| Swarm Miner-gated | `grep -q nm_swarm_set_enabled src/ui_nm/nm_uimanager.cpp` | exit 0 |
| Modules driven from loop | `grep -q nm_market_tick src/NerdMinerV2.ino.cpp && grep -q nm_weather_tick src/NerdMinerV2.ino.cpp && grep -q nm_swarm_tick src/NerdMinerV2.ino.cpp` | exit 0 |
| Settings init at boot | `grep -q nm_settings_init src/NerdMinerV2.ino.cpp` | exit 0 |

---

## 4. Security Presence Gates (ASVS L1 boundary checks)

| Threat | Gate command | Pass |
|--------|--------------|------|
| T-05-UPL (upload magic+size) | `grep -Eq '0x53\|0x4E' src/webapi.cpp && grep -q UPLOAD_FILE_START src/webapi.cpp` | exit 0 |
| T-05-SS (.ss header bounds) | `grep -Eq '800\|599' src/ui_nm/nm_screensaver.cpp` | exit 0 |
| T-05-WRATE (weather 429) | `grep -q 429 src/webapi.cpp` | exit 0 |
| T-05-PWD (no pw leak) | manual: GET /api/setting/network/mining responses contain no WiFiPWD/pool password fields | review |

---

## 5. Sampling Rate (from RESEARCH)

- **Per task commit:** `pio run -e ESP32-2432S028R 2>&1 | grep -E "error:|\.cpp|\.h"` (fast compile check).
- **Per wave merge:** full `pio run -e ESP32-2432S028R` (build + binary size).
- **Phase gate:** full build green + firmware.bin size + headroom recorded (05-06 Task 2).

## 6. Wave 0 Gaps

None. All source files either exist (stubs to replace: nm_screen_sprice.cpp, nm_screen_weather.cpp;
extend: nm_uimanager.{cpp,h}, webapi.cpp, NerdMinerV2.ino.cpp) or are new (nm_settings, nm_market,
nm_weather, nm_screensaver, nm_swarm). Build system already configured; no test scaffold to create.

## 7. Deferred to Phase 7 (NOT validated here)

On-device data fetch/render confirmation, live screensaver carousel display, real upload, swarm
discovery against real peers, and the visual parity checklist. Phase 5 proves the code compiles and
links; behavior is Phase 7 (CONTEXT §domain, deferred ideas).
