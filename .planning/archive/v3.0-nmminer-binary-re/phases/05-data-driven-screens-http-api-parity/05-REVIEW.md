---
phase: 05-data-driven-screens-http-api-parity
reviewed: 2026-06-18T00:00:00Z
depth: standard
files_reviewed: 5
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/nm_market.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/nm_weather.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
findings:
  critical: 0
  warning: 4
  info: 3
  total: 7
status: issues_found
---

# Phase 5: Code Review Report

**Reviewed:** 2026-06-18
**Depth:** standard (time-boxed)
**Files Reviewed:** 5
**Status:** issues_found

## Summary

Memory-safety review of the .ss decoder, screensaver upload handler, market/weather
JSON parsers, and the screensaver state machine. **No memory-corruption or crash
vector found**: the .ss header is validated before any write/alloc; the LZSS ring
read/write is bounded by `% NM_SS_WINDOW`; the row buffer is clamped to the 320-px
panel; market/weather array indexing is bounded by `n < MAX` / `i < size()`. HTTP
paths all call `http.end()` and set timeouts. No secret leak in GETs/`/alive`.

The defects are correctness/robustness, not safety: a positional symbol-to-price
mismatch in the ticker parser (mislabeled prices), an LZSS offset-width vs window-size
mismatch that silently produces garbage pixels on non-stock params, and two minor
HTTP early-return leaks. Plus the carried-forward Brightness round-trip mismatch.

## Narrative Findings (AI reviewer)

## Critical Issues

None. The two HIGH-risk targets (.ss decoder memory safety, upload handler) are sound:

- **.ss decoder bounded:** header magic + dims validated before the row buffer / ring
  window are touched (`nm_screensaver.cpp:163-183`). LZSS back-ref read/write both go
  through `window[... % NM_SS_WINDOW]` (`:217-218`), so no OOB regardless of `off`/`len`.
  `start < 0` is rejected (`:215`). `sink_pixel` caps at `w*h` and clamps `col` to the
  row buffer width (`:120-123`). A hostile/truncated file cannot overflow or crash.
- **Upload handler bounded:** magic `0x53 0x4E` checked on first 2 bytes (`webapi.cpp:676`),
  cumulative size `> API_SS_MAX_BYTES` rejects mid-stream (`:680`), path is firmware-computed
  via `apiSsNextPath()` (no client filename, no traversal — `:669`), partial files are
  removed on `bad`/abort (`ssUploadFree(true)`, `:640-646,650`). Preflight bounds `size`
  to `(0, 200KB]` (`:600-601`).

## Warnings

### WR-01: Ticker prices labeled by request order, not response symbol (mislabeled data)

**File:** `src/nm_market.cpp:139-148`
**Issue:** `/api/v3/ticker/24hr?symbols=[...]` does NOT guarantee the response array is in
request order. The loop reads `o["symbol"]` (`:142`) but then ignores it and labels row
`n` with `bases[n]` (`:143`) — a positional assumption. If Binance reorders the array
(or omits a symbol), every price on the pricewall is attached to the wrong coin. The
`else` branch that DOES use the real `sym` is unreachable for `n < want` (the normal case).
**Fix:** match by symbol, not position:
```cpp
const char* sym = o["symbol"] | "";          // e.g. "BNBUSDT"
int slot = -1;
for (int b = 0; b < want; ++b) {
    size_t bl = strlen(bases[b]);
    if (strncmp(sym, bases[b], bl) == 0 && strcmp(sym + bl, "USDT") == 0) { slot = b; break; }
}
if (slot < 0) continue;
strncpy(g_ticker[slot].symbol, bases[slot], NM_MARKET_SYM_LEN - 1);
g_ticker[slot].symbol[NM_MARKET_SYM_LEN - 1] = 0;
g_ticker[slot].last_price = o["lastPrice"].as<float>();
g_ticker[slot].change_pct = o["priceChangePercent"].as<float>();
```

### WR-02: LZSS offset width can exceed the 1024-byte window — silent garbage decode

**File:** `src/ui_nm/nm_screensaver.cpp:180,210,217`
**Issue:** The param guard accepts `w_bits` up to 15 (`(unsigned)(w_bits-4) >= 0xC`), so
`off = br_bits(&br, w_bits)` can reach 32767, but the ring window is only `NM_SS_WINDOW`
(1024). The header comment (`:42`) claims "10-bit offset reaches at most 1023 bytes back"
— that invariant is NOT enforced. For any file with `w_bits > 10`, `window[(start+k)%1024]`
reads a slot that is no longer the intended back-reference: no crash (modulo bounds it),
but the decoded image is wrong. Memory-safe, correctness-broken on non-stock params.
**Fix:** reject params whose window exceeds the ring, OR size the ring to the max:
```cpp
if (w_bits > 10) { f.close(); return false; }   // window only holds 10-bit back-refs
```
(or size `window` to `1 << w_bits` cap — but 32KB is too large for dram0; rejecting is correct.)

### WR-03: `http.begin()` failure path leaks ticker state / skips `http.end()` is OK but state inconsistent

**File:** `src/nm_market.cpp:78,132,162`
**Issue:** When `http.begin(url)` returns false, the functions `return` without resetting
`g_kline_count`/`g_ticker_count`. `http.end()` is correctly skipped (begin failed, nothing
to close), so no socket leak — but stale counts persist and the screen keeps rendering the
previous fetch as if current. Lower-severity than a leak, but the failure is invisible.
**Fix:** on `begin()` failure set `g_market_ok = false` (klines) so the screen can show a
stale/error indicator instead of silently holding old data.

### WR-04: Brightness percent round-trip mismatch (carried forward)

**File:** `src/webapi.cpp:380,470-475`
**Issue:** POST `/api/setting/preference` stores `Settings.Brightness = b*255/100` (0-255,
`:473`) but GET returns `Settings.Brightness` raw (0-255, `:380`) while the dashboard
contract field is a percent. POST 50 -> GET 127. The GET and POST units disagree.
**Fix:** convert back to percent on GET: `doc["Brightness"] = (Settings.Brightness * 100 + 127) / 255;`
(or store percent and convert to PWM only at `ledcWrite`).

## Info

### IN-01: `g_ticker[n].symbol` not explicitly NUL-terminated in the n<want branch

**File:** `src/nm_market.cpp:143`
**Issue:** `strncpy(g_ticker[n].symbol, bases[n], NM_MARKET_SYM_LEN - 1)` leaves byte
`[LEN-1]` untouched. `g_ticker` is zero-init at definition but reused across fetches; a
shorter symbol overwriting a longer prior one is fine, but relying on prior-call NUL is
fragile. The `else` branch correctly sets `[LEN-1]=0`. (Subsumed by the WR-01 fix.)
**Fix:** add `g_ticker[n].symbol[NM_MARKET_SYM_LEN - 1] = 0;` (the WR-01 rewrite already does).

### IN-02: `apiSsNextPath()` falls back to overwriting slot 999 silently

**File:** `src/webapi.cpp:589-596`
**Issue:** When all 1000 slots exist, the function returns the `_999.ss` path and the
upload overwrites it with no signal to the client. Acceptable as a last-slot recycle, but
undocumented in the response. Minor.
**Fix:** none required; optionally surface `"action":"overwrite"` consistency in the done payload.

### IN-03: `utc_hour_index()` magic `86400`/`3600` and RTC sentinel

**File:** `src/nm_weather.cpp:60-68`
**Issue:** `now < 86400` as "RTC not set" is a heuristic (any real epoch is far larger);
fine in practice. Magic numbers are commented inline. No action needed.

---

_Reviewed: 2026-06-18_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
