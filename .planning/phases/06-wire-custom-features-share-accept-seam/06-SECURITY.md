---
phase: 06-wire-custom-features-share-accept-seam
audited: 2026-06-18
auditor: Claude (gsd-security-auditor)
asvs_level: 1
block_on: open
fork_commit_range: 8b9814d..9a476e7
threats_total: 8
threats_mitigated: 6
threats_accepted: 2
threats_open: 0
status: secured
---

# Phase 6: Security Audit — Wire Custom Features at Share-Accept Seam

**Scope:** Local hardware seam (LEDC speaker GPIO26, RGB LED GPIO 4/16/17, VSPI SD) plus
one carry-forward network surface (`monitor.cpp` TLS for public price/pool/block feeds).

**Method:** Every declared mitigation verified against the implemented code in the fork tree
(`firmware/custom/NerdMiner-CYD-Custom`). Documentation and intent were NOT accepted as
evidence — each verdict cites `file:line`.

---

## Threat Verdicts

| Threat ID | Category | Disposition | Verdict | Evidence (file:line) |
|-----------|----------|-------------|---------|----------------------|
| T-06-WDT | DoS | mitigate | MITIGATED | `speaker.cpp:129-135` beepShareAccepted = `playTone(2000,80)` arms tone + returns, no vTaskDelay; `nm_led.cpp:53-87` tick is millis-gated, no delay |
| T-06-MINE / T-06-MINE2 | Tampering (HW-04) | mitigate | MITIGATED | `git diff 8b9814d..9a476e7 -- src/mining.cpp` = exactly one line: `playShareBeep()`→`beepShareAccepted()`; disconnect alarm at `mining.cpp:263` unchanged; 0 non-Speaker changed lines |
| T-06-SDLOG | DoS | mitigate | MITIGATED | `NerdMinerV2.ino.cpp:305` isReady()-gated; `:311` share-delta guard; only I/O is `appendCSVLog` (`sdstore.cpp:215-243`, rotation-bounded at SD_CSV_MAX_SIZE `:179`); no delay |
| T-06-SDABSENT | Availability | accept | ACCEPTED-RISK | `sdstore.cpp:39-44` CARD_NONE→SD.end()→returns false (no crash); `NerdMinerV2.ino.cpp:305` logShareEvent returns on !isReady() — mining unaffected (D-63). Logged below. |
| T-06-SDPATH | Tampering | mitigate | MITIGATED | `sdstore.cpp:394-439` sanitizePath: null-byte reject, leading-slash strip, `..` removal, `/sd/` subtree constraint, final `..`/`//` rejection (Phase-5 audited, unchanged) |
| T-06-TLS | Spoofing/Tampering (MITM) | accept | ACCEPTED-RISK | `monitor.cpp:40,71,97,139,179,461,463` setInsecure() on a shared static WiFiClientSecure. Public read-only APIs, no secrets transmitted. Logged below with rationale. WR-05 mitigation verified (see T-06-TLSREUSE). |
| T-06-TLSREUSE | DoS (stale TLS session) | mitigate | MITIGATED | WR-05: `secureClient.stop()` after `http.end()` on EVERY path — success paths `monitor.cpp:93,119,151,198,514`, catch paths `:123,155,202,522`, pool-error path `:510`. http.end() present on all return paths (no socket leak, IN-02). |
| T-06-CFG | Tampering (serial-config bounds) | mitigate | MITIGATED | `wManager.cpp:183` Brightness `constrain(...,0,255)` (IN-03); `:182` InvertColors `.as<bool>()`; `:179-180` strncpy with `sizeof()-1`; SSID-length guard `:184` |
| D-11 | Supply-chain (closed-firmware bytes) | mitigate | MITIGATED | No prebuilt archive/blob link in platformio.ini (only `build_src_filter`); no vendored `.a`/`.o` in src/include; "NMMiner" hits are clean-room LVGL/`.ss` reimplementations (strings/names only) |
| T-06-SC | Tampering (pkg installs) | accept | ACCEPTED-RISK | No npm/pip/cargo installs this phase (existing PlatformIO C++ toolchain). Trivial-accept, not separately logged. |

**Count:** 6 MITIGATED · 3 ACCEPTED-RISK (T-06-SDABSENT, T-06-TLS, T-06-SC) · 0 GAP

> Note: T-06-MINE and T-06-MINE2 are the same HW-04 invariant declared in both sub-plans;
> counted once. T-06-SC is a trivial standing accept (no installs); the two substantive
> accepted-risk entries logged below are T-06-SDABSENT and T-06-TLS.

---

## Accepted Risks Log

### AR-01 — TLS certificate validation disabled for stock public feeds (T-06-TLS / 06-REVIEW IN-01)

**Component:** `src/monitor.cpp` — mempool.space (global hash / fees / block height),
coingecko (BTC price), and the mining pool stats API.
**Disposition:** ACCEPTED-RISK.
**Mechanism:** A file-static `WiFiClientSecure secureClient` (`monitor.cpp:40`) has
`setInsecure()` called before each `http.begin()` (`:71,97,139,179,461,463`), disabling
CA/hostname validation for all stock feed GETs.

**Rationale (why accepted, not a gap):**
- These are **public, read-only, unauthenticated** APIs. No credentials, wallet keys, or
  secrets are transmitted on these requests (the BTC wallet sent to the pool API is a
  public address, already broadcast on-chain).
- A successful MITM yields only **already-public data** (block height, network hashrate,
  BTC price, pool worker stats) shown on the device display. No integrity-sensitive control
  path depends on these values; mining/stratum is a separate connection.
- **CA-pinning is impractical here:** four rotating third-party hosts with independently
  rotating certificates, on a 3 MB-flash ESP32 already at 90.5% flash (9.5% headroom). Bundling
  + maintaining multiple CA roots is not viable for this device class.
- **Matches upstream + project pattern:** identical to upstream NerdMiner's feed handling and
  the project's Phase-5 D-51 setInsecure pattern. Carry-forward, not new behavior introduced
  by this phase's intent.

**Required mitigation present (WR-05, T-06-TLSREUSE):** `secureClient.stop()` is called after
`http.end()` on every path — all five success paths, all three catch paths, and the pool-error
early-return — forcing a clean TLS session per host and preventing the documented
arduino-esp32 stale/half-open cross-host session reuse bug. `http.end()` is present on every
return path (no HTTPClient socket leak; IN-02 leak-check passes).

**Future hardening (optional, not required for v1):** pin the API CA roots via `setCACert()`
if flash budget and cert-rotation tracking permit.

### AR-02 — SD card optional; absence is non-fatal (T-06-SDABSENT / D-63)

**Component:** `src/sdstore.cpp`, `src/NerdMinerV2.ino.cpp` logShareEvent.
**Disposition:** ACCEPTED-RISK (intended availability tradeoff).
**Mechanism:** SD is optional by design. Mount returns false on no-card without crashing
(`sdstore.cpp:39-44`: CARD_NONE → SD.end() → false). logShareEvent returns immediately on
`!isReady()` (`NerdMinerV2.ino.cpp:305`). Mining and all other functions are unaffected when
no card is present.
**Rationale:** SD logging is a convenience feature; degraded logging (none) is preferable to
blocking mining on missing optional hardware. Accepted per D-63.

---

## Unregistered Flags

None. All security-relevant findings surfaced during implementation (06-REVIEW.md CR-01,
WR-01..05, IN-01..04) map to declared threats or the accepted-risk log above. No new attack
surface appeared without a threat mapping.

For traceability, the non-threat review findings are dispositioned in 06-VERIFICATION.md
(CR-01 closed by b84dca3; WR-01 by 0e1150a; WR-03 by 1c2bdfb; IN-03 by 9a476e7; WR-02/WR-04
documented-accepted). None of these is an open security gap.

---

## Conclusion

**SECURED.** All eight declared threats resolve to MITIGATED or documented ACCEPTED-RISK with
in-tree code evidence. Zero gaps. The load-bearing HW-04 mining-integrity invariant holds
exactly (single-line phase diff). The TLS finding is a documented accepted risk (public
read-only feeds, no secrets, impractical pinning) with its required WR-05 session-hygiene
mitigation verified on all paths — not a gap.

On-device behavioral confirmation (audible beep, visible LED, physical SD, live mining) is
out of Phase 6 scope and routed to Phase 7; it does not affect this static security verdict.

_Audited: 2026-06-18 · Auditor: Claude (gsd-security-auditor) · Implementation files unmodified._
