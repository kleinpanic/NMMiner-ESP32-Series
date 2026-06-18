# 08-06 SUMMARY — Security, Optimization, Docs

**Status:** Complete (executed by orchestrator).

## Security
- Dashboard proxy (`apps/nmminer-control/server.py`): path allowlist + `..`/null-byte reject on all `/api/sd/*`, forced `Content-Type: application/octet-stream` + `X-Content-Type-Options: nosniff` on download, `secure_filename`-style sanitized download name. (Findings #1-3 from automated review — fixed.)
- Firmware web API (`webapi.cpp`): removed wildcard `Access-Control-Allow-Origin` (proxy is server-side → CORS unneeded; reduces CSRF surface, documented LAN-trust model); whitelisted `Content-Disposition` filename (header-injection guard). (Findings from automated review — fixed.)
- Secret hygiene: full git-history sweep of the fork — wallet/SSID/password NOT present; `.gitignore` excludes `config.json`, `secrets.*`, `*.bin`, logs. Config is runtime (WiFiManager portal / SD `config.json`), never committed.

## Optimization
- Mining hot path unaffected: speaker is non-blocking (LEDC + millis() state machine, no delay()), web API uses `WebServer.handleClient()` (non-blocking), CSV logging is throttled (~60s) and rotates at 1 MB. No blocking calls added to loop().
- Firmware size: Flash 68.9% (2.17 MB / 3.1 MB app partition) — comfortable headroom; RAM 16.9%.

## Docs
- Fork `docs/API.md` (endpoint contract), `NOTICE` (BitMaker-hub MIT attribution + fork changes), README fork header, `docs/FLASH_GUIDE.md` (backup-first flash + on-device checklist).

## Tests (full coverage + branching, per user)
- `tests/test_server.py`, `test_server_endpoints.py`, `test_e2e.py`: 73 passed, **94% coverage (statement + branch)** — every branch of the security helpers, all endpoint success/error paths, and a real mock-firmware↔proxy E2E chain. Remaining uncovered = cache-hit paths + `main()` entrypoint.

## Known follow-ups
- `sdstore.cpp::sanitizePath` dead null-byte loop (moot for C strings; proxy rejects `\x00`).
- ST7789 vs ILI9341 display driver to confirm on-device.
