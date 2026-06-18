---
phase: 05-data-driven-screens-http-api-parity
audited: 2026-06-18
asvs_level: 1
block_on: high
threats_total: 21
threats_closed: 21
threats_open: 0
unregistered_flags: 0
status: secured
---

# Phase 5 — Security Audit (threat verification)

Verification of every declared mitigation in the Phase-5 threat register against the
implemented code. Each `mitigate` threat was confirmed by locating the actual control in
the cited file; each `accept` threat was confirmed against the LAN-trust model documented
in code. Implementation files were not modified.

**Verdict: SECURED — 21/21 closed, 0 open, 0 unregistered flags. Phase may ship.**

Largest attack surface (outbound HTTP fetches, 14 new HTTP endpoints, a multipart file
upload, an on-device `.ss` binary decoder) — all load-bearing untrusted-input paths are
bounded and validated.

## Threat Verification

| Threat ID | Category | Disposition | Verdict | Evidence |
|-----------|----------|-------------|---------|----------|
| T-05-VAL | Tampering | mitigate | CLOSED | `nm_settings.cpp:48-68` — `getString(...,outlen)` + `out[outlen-1]=0`; default path `strncpy(out,d,outlen-1)`+NUL. No unbounded copy. |
| T-05-NVS | Tampering | accept | CLOSED | LAN-trust model; NVS device-local. Accepted-risk entry below. |
| T-05-JSON | Tampering/DoS | mitigate | CLOSED | `nm_market.cpp:82` `DynamicJsonDocument doc(10240)` klines / `:135` `doc(4096)` ticker; stream-parsed via `http.getStream()`; klines loop `:87` `if (n >= NM_MARKET_MAX_KLINES) break`; `setTimeout(8000)` `:77,131`. |
| T-05-OOM | DoS | mitigate | CLOSED | `nm_market.cpp:170-206` pairs built from `/api/v3/ticker/price` stream object-by-object (`StaticJsonDocument<128>` per entry, `:189`); never `/exchangeInfo`. |
| T-05-TLS | Info disclosure | accept | CLOSED | `nm_market.cpp:35,73` HTTP:80 to `data-api.binance.vision` (public market data, no secrets in transit, D-51). Accepted-risk entry below. |
| T-05-WJSON | Tampering/DoS | mitigate | CLOSED | `nm_weather.cpp:40-41` `StaticJsonDocument<3072>`/`<4096>`; daily loop `:129` `for i<NM_WEATHER_DAYS` with per-field `i<size()` clamp `:130-134`; AQI idx clamped `:164`; `setTimeout(8000)` `:75`. |
| T-05-WRATE | DoS | mitigate | CLOSED | `nm_weather.cpp:193-201` 60s window → `return false`; `webapi.cpp:543-549` `handleWeatherRefresh` returns HTTP 429 on reject. |
| T-05-WTLS | Info disclosure | accept | CLOSED | `nm_weather.cpp:76` `sc.setInsecure()` HTTPS to open-meteo (public weather, no secrets in transit). Accepted-risk entry below. |
| T-05-SS | Tampering | mitigate | CLOSED | `nm_screensaver.cpp:166-191` magic `0x53 0x4E` + `w!=0 && w<=MAX_W && h>0 && (h-1)<=(MAX_H-1)` + param-nibble range guard — all validated BEFORE row/window alloc; LZSS `start<0` rejected `:223`. |
| T-05-SSRAM | DoS | mitigate | CLOSED | `nm_screensaver.cpp:199` fixed `row[<=320]`, `:200` `static uint8_t window[1024]`; output capped `:120` `if (emitted >= w*h) return`; col clamped `:204`. WR-02 window-width guard `:188` `if ((1<<w_bits)>NM_SS_WINDOW) reject`. |
| T-05-SWARM | DoS | mitigate | CLOSED | `nm_swarm.cpp:31` fixed `s_peers[MAX_PEERS]`; `:45` cap; `:43` dedup; `:28` 1 host/tick; `:26` 60ms connect timeout; `:104` /24-mask-only guard; `:93` 30s sleep. WDT-safe. |
| T-05-NAV | Tampering/crash | mitigate | CLOSED | `nm_uimanager.cpp:80` `s_nav_in_progress`; set `:97`/cleared `:136` around `lv_scr_load`; screensaver activation gated `:300` `if (!s_nav_in_progress)`. |
| T-05-PWD | Info disclosure | mitigate | CLOSED | `webapi.cpp:347-353` network GET emits only SSID (comment "NEVER WiFiPWD"); `:355-363` mining GET emits pool/port/wallet only (comment "NEVER PrimaryPassword"); `/probe` `:259-273`, `/info` `:275-325`, `/alive` `:327-343` serialize no secret. Grep of all GET serializers found no `WifiPW`/`PoolPassword`/privkey emission. |
| T-05-SET | Tampering | mitigate | CLOSED | `webapi.cpp:240-253` partial-JSON via `isNull()` skip + bounded `strlcpy`; range checks: port `:449` `1..65535`, timezone `:460` `-12..14`, brightness `:474` `1..100`, lat `:512` `±90`, lon `:516` `±180`. |
| T-05-UPL | Tampering/DoS | mitigate | CLOSED | `webapi.cpp:678-679` magic `0x53 0x4E` on first 2 bytes → `bad`; `:682` cumulative `> 200KB` → `bad`; done handler `:652` returns 413; partial file removed via `ssUploadFree(true)` `:645`. |
| T-05-PATH | Tampering | mitigate | CLOSED | `webapi.cpp:671` `s_ss->path = apiSsNextPath()` (firmware-computed `/ss/ss_saver_320_240_NNN.ss`, `:591-598`); client filename never read. No traversal. |
| T-05-PRE | Tampering | mitigate | CLOSED | `webapi.cpp:602-603` `arg("size").toInt()` then `if (sz<=0 || sz>200KB) reject` before LittleFS free-space compare `:605-625`. |
| T-05-AUTH | Spoofing | accept | CLOSED | No auth on local-LAN API = stock parity, documented `webapi.cpp:45-50`. No new privilege escalation; no destructive route (verified — see below). Accepted-risk entry below. |
| T-05-SC | Tampering | mitigate | CLOSED | `platformio.ini` lib_deps unchanged in Phase-5 commits; ArduinoJson/HTTPClient/WiFiClientSecure/LittleFS all pre-existing. Zero new packages. |
| T-05-WDT | DoS | mitigate | CLOSED | `nm_market.cpp:77,131,173` + `nm_weather.cpp:75` `setTimeout(8000)`; ticks millis()-gated (`nm_market.cpp:220,226`, `nm_weather.cpp:188`, `nm_swarm.cpp:98`); off the mining loop. |
| T-05-FLASH | Availability | mitigate | CLOSED | `05-06-SUMMARY.md` build SUCCESS 2,845,985/3,145,728 = 90.5% (299,743 B headroom); dram0 +1408 B after D-58 ladder (`NM_MARKET_PAIRS_LEN` 5120→3072). No DATA requirement silently dropped. |

### D-11 (milestone redistribution boundary — verified for Phase-5 source)
No verbatim closed-firmware literals/bodies in committed Phase-5 source. Grep for
`share accepted` / `%ldms` / `FUN_4` body / `DAT_3f` / raw `0x400` addresses across
`webapi.cpp`, `nm_market.cpp`, `nm_weather.cpp`, `nm_swarm.cpp`, `nm_settings.cpp`,
`nm_screensaver.cpp` returned only one match: `nm_screensaver.cpp:16`, a comment stating
the code does NOT call the stock `FUN_400f6ff4` symbol (identifier reference in a fresh
port, not a redistributed firmware body). Clean.

### No destructive / auth-bypass route (verified)
Grep of `webapi.cpp` route table for `reset|factory|wipe|erase|clear|format` HTTP
registrations: none. No `deleteConfig`/`Preferences.clear()`/`nvs_flash_erase` reachable
from any handler. `/api/system/restart` (`:556-560`) is reboot-only (`ESP.restart()`).
Phase-4 credential-reset stays gated behind the physical control, not exposed as a route.

## Accepted Risks Log

| ID | Risk | Rationale | Accepted by |
|----|------|-----------|-------------|
| T-05-NVS | NVS (`nm_stock` namespace) is device-local, unauthenticated to a LAN actor | Matches stock NMMiner trust model; NVS is on-device, no remote write path beyond the bounded settings POSTs | PLAN 05-01 |
| T-05-TLS | Binance market fetch over plain HTTP:80 (no TLS) | Public market data, no secrets in transit; matches stock host `data-api.binance.vision` (D-51) | PLAN 05-02 |
| T-05-WTLS | open-meteo HTTPS with `setInsecure()` (cert not validated) | Public weather data, no secrets in transit; matches the fork's embedded `monitor.cpp` skip-cert idiom | PLAN 05-03 |
| T-05-AUTH | All new HTTP endpoints have no authentication on the LAN | Stock-parity LAN-trust model documented in `webapi.cpp:45-50`; dashboard talks via server-side proxy, no wildcard CORS; no new privilege escalation or destructive route introduced | PLAN 05-05 |

## Unregistered Flags

None. SUMMARY.md files contain no `## Threat Flags` section. The 4 warnings in 05-REVIEW.md
(WR-01 ticker symbol mapping, WR-02 LZSS window width, WR-03 begin() stale state, WR-04
brightness round-trip) are correctness/robustness defects, not new attack surface — WR-01
and WR-02 were folded into the audited code (symbol-match loop at `nm_market.cpp:149-161`;
window guard at `nm_screensaver.cpp:188`).

## Verification Commands

```sh
# T-05-PWD — no secret in any GET serializer
grep -n "WifiPW\|PoolPassword\|privkey\|WiFiPWD" src/webapi.cpp   # only in POST setters, never GET

# T-05-UPL / T-05-PATH — magic + size + firmware path
grep -n "0x53\|0x4E\|API_SS_MAX_BYTES\|apiSsNextPath" src/webapi.cpp

# T-05-SS / T-05-SSRAM / WR-02 — header validate pre-alloc + window guard
grep -n "f.read(hdr\|NM_SS_MAX_W\|1 << w_bits\|NM_SS_WINDOW\|start < 0" src/ui_nm/nm_screensaver.cpp

# T-05-JSON / T-05-OOM / T-05-WDT — bounded docs + timeouts
grep -n "DynamicJsonDocument\|StaticJsonDocument\|setTimeout\|http.end" src/nm_market.cpp src/nm_weather.cpp

# T-05-SWARM — bounded array + WDT-safe probe
grep -n "MAX_PEERS\|CONNECT_MS\|HOSTS_PER_TICK\|mask\[" src/nm_swarm.cpp

# no destructive route
grep -nE "reset|factory|wipe|erase|format" src/webapi.cpp | grep "apiServer.on"   # empty

# T-05-SC — no new lib deps in phase 5
git log -p --since="2026-06-17" -- platformio.ini | grep -E "^\+.*[a-z]+/[A-Za-z].*@"  # empty

# D-11 — no closed-firmware literals in committed source
grep -rn "share accepted\|%ldms\|DAT_3f" src/webapi.cpp src/nm_market.cpp src/nm_weather.cpp \
  src/nm_swarm.cpp src/nm_settings.cpp src/ui_nm/nm_screensaver.cpp   # empty
```

---
_Audited: 2026-06-18 — gsd-security-auditor (FORCE stance)_
_All paths relative to firmware/custom/NerdMiner-CYD-Custom/_
