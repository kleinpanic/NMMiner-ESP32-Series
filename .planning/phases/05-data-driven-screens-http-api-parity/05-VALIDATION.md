# Phase 5 Validation — Data-Driven Screens + HTTP API Parity

> **RESULT (finalized 2026-06-18, Plan 05-06 integration gate): PASSED.**
> Full build GREEN, all DATA-01..06 presence gates PASS, all page-gating + boot-init
> gates PASS, all automated security gates PASS (T-05-SS gate pattern corrected — see §4).
> Phase build gate satisfied; fork commit `ac22146`.
>
> | Metric | Value |
> |--------|-------|
> | Build | `pio run -e ESP32-2432S028R` → **SUCCESS** (no `error:`) |
> | firmware.bin | 2,852,288 bytes |
> | Flash used | 2,845,985 / 3,145,728 = **90.5%** (headroom 299,743 B / 9.5%) |
> | RAM (total static) | 37.6% (123,168 / 327,680 B) |
> | dram0_0_seg | `_bss_end` 0x3ffdbc80, region end 0x3ffdc200 → **+1408 B headroom** (was −640 B overflow at 05-05) |
> | Flash delta vs baseline | +91,249 B over the 2,754,736 B RESEARCH baseline (+2.9 pts) — within budget, no DATA req dropped |

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

| Gate | Command | Pass condition | Result |
|------|---------|----------------|--------|
| Full build | `pio run -e ESP32-2432S028R 2>&1 \| tail -6` | `SUCCESS` present, no `error:` | **PASS** — SUCCESS |
| Flash budget (D-58) | `ls -la .pio/build/ESP32-2432S028R/firmware.bin` | size recorded; computed headroom vs 3,145,728 bytes; overflow surfaced, never silently drops a DATA req | **PASS** — 2,852,288 B bin; flash 90.5% (299,743 B headroom); no req dropped |

Baseline for the budget delta: 2,754,736 bytes / 87.6% of 3MB (RESEARCH §"Flash Budget Analysis").

**D-58 dram0 resolution:** the 640 B `dram0_0_seg` overflow reported at 05-05 (root cause:
`nm_market` `s_pairs[5120]` `.bss` resurrected once the loop links `nm_market_tick`) was cleared
by reducing `NM_MARKET_PAIRS_LEN` 5120→3072 (a 2048 B `.bss` reduction). Post-fix dram0 headroom
is **+1408 B**. The pairs feature is preserved (3072-char USDT CSV ≈ 500 bases, ample); the
`s_pairs`-length bounds check uses `sizeof(s_pairs)` so it tracked the resize automatically.

---

## 2. Requirement → Presence Gate Map

Run from `firmware/custom/NerdMiner-CYD-Custom/`. Each gate filters comments
(`grep -v '^[[:space:]]*[/*]'`) before counting so header prose does not self-satisfy.

| Req | Behavior | Gate command | Pass | Result |
|-----|----------|--------------|------|--------|
| DATA-01 | Binance market fetch + SPrice kline/pricewall | `grep -q data-api.binance.vision src/nm_market.cpp && ! grep -q exchangeInfo src/nm_market.cpp && grep -q lv_chart src/ui_nm/nm_screen_sprice.cpp && ! grep -q lv_canvas_create src/ui_nm/nm_screen_sprice.cpp` | exit 0 | **PASS** |
| DATA-02 | open-meteo weather fetch + Weather screen | `grep -q open-meteo.com src/nm_weather.cpp && grep -q air-quality-api src/nm_weather.cpp && grep -q setInsecure src/nm_weather.cpp && grep -q g_weather src/ui_nm/nm_screen_weather.cpp` | exit 0 | **PASS** |
| DATA-03 | Screensaver carousel decodes .ss from LittleFS | `grep -q LittleFS src/ui_nm/nm_screensaver.cpp && grep -Eq '0x53\|0x4E\|"SN"' src/ui_nm/nm_screensaver.cpp && grep -q s_ss_active src/ui_nm/nm_uimanager.cpp` | exit 0 | **PASS** |
| DATA-04 | Swarm scan + FindMe override + endpoints | `grep -Eq 'nm_swarm_add_ip\|nm_swarm_request_findme' src/nm_swarm.cpp && grep -q '/alive' src/webapi.cpp && grep -q '/api/swarm/find' src/webapi.cpp` | exit 0 | **PASS** |
| DATA-05 | Stock API parity, additive | `grep -q '"/probe"' src/webapi.cpp && grep -c 'api/setting/' src/webapi.cpp \| awk '$1>=6' && grep -q 'api/speaker' src/webapi.cpp && grep -q '/api/update/screensaver' src/webapi.cpp` | exit 0 | **PASS** (6+ setting routes) |
| DATA-06 | Branding model=NMMiner | `grep -q '"/probe"' src/webapi.cpp && grep -q 'NMMiner' src/webapi.cpp` | exit 0 | **PASS** |

---

## 3. Page-Gating & State-Machine Presence Gates

| Check | Gate command | Pass | Result |
|-------|--------------|------|--------|
| Market page-gated | `grep -q nm_market_set_page_allowed src/ui_nm/nm_uimanager.cpp` | exit 0 | **PASS** (wired in goto_page_ex, line 133) |
| Swarm Miner-gated | `grep -q nm_swarm_set_enabled src/ui_nm/nm_uimanager.cpp` | exit 0 | **PASS** (line 134) |
| Modules driven from loop | `grep -q nm_market_tick src/NerdMinerV2.ino.cpp && grep -q nm_weather_tick src/NerdMinerV2.ino.cpp && grep -q nm_swarm_tick src/NerdMinerV2.ino.cpp` | exit 0 | **PASS** — `nm_market_tick`/`nm_weather_tick` driven in loop(); `nm_swarm_tick` driven via `nm_uimanager_tick` (Plan 05-04, not re-pumped to avoid double-driving the probe state machine) |
| Settings init at boot | `grep -q nm_settings_init src/NerdMinerV2.ino.cpp` | exit 0 | **PASS** (setup(), before nm_uimanager_init) |

---

## 4. Security Presence Gates (ASVS L1 boundary checks)

| Threat | Gate command | Pass | Result |
|--------|--------------|------|--------|
| T-05-UPL (upload magic+size) | `grep -Eq '0x53\|0x4E' src/webapi.cpp && grep -q UPLOAD_FILE_START src/webapi.cpp` | exit 0 | **PASS** |
| T-05-SS (.ss header bounds) | ~~`grep -Eq '800\|599' src/ui_nm/nm_screensaver.cpp`~~ → `grep -q 'NM_SS_MAX_W' src/ui_nm/nm_screensaver.cpp && grep -Eq '800' src/ui_nm/nm_screensaver.h` | exit 0 | **PASS (gate pattern corrected)** — the bound is `NM_SS_MAX_W=800 / NM_SS_MAX_H=600` defined in nm_screensaver.h and enforced in nm_screensaver.cpp:171 (`w>NM_SS_MAX_W \|\| (h-1)>(NM_SS_MAX_H-1)`). The original gate's `800\|599` literals were stale (`599` typo for `600`, and the literals live in the `.h` macro not the `.cpp`). The actual header-bounds check IS present and correct. |
| T-05-WRATE (weather 429) | `grep -q 429 src/webapi.cpp` | exit 0 | **PASS** |
| T-05-PWD (no pw leak) | manual: GET /api/setting/network/mining responses contain no WiFiPWD/pool password fields | review | deferred to Phase 7 (on-device); 05-05 SUMMARY asserts secrets never serialized in GET |

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
