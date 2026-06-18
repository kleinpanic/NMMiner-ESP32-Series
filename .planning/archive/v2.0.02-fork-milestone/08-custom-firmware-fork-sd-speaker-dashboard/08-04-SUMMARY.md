# 08-04 SUMMARY — Custom JSON Web API

**Status:** Complete (executed by orchestrator — background subagents were sandbox-blocked from Bash/build).

## What was built
- `firmware/custom/NerdMiner-CYD-Custom/include/webapi.h` + `src/webapi.cpp`: a `WebServer` on port 80, started after WiFi (in setup), serviced non-blocking from loop().
- Endpoints (see fork `docs/API.md`): `GET /api/status` (+ `/api/system/info` alias) → mining + `sd` + `speaker` blocks; `GET /api/speaker`, `POST /api/speaker/volume|mute|test-tone`; `GET /api/sd/list`, `GET /api/sd/download`, `POST /api/sd/delete`, `GET /api/sd/log`.
- Wired to `Speaker::` and the global `sdStore` (SDStore). All SD paths pass `SDStore::sanitizePath`.

## Contract fix
- Dashboard (08-05) posts `volume_percent` / `muted` / `freq_hz,duration_ms`; firmware originally read `value`. Firmware aligned to the dashboard keys. Verified by the E2E test.

## Security (08-06 overlap, done here)
- Removed wildcard `Access-Control-Allow-Origin: *` — the dashboard talks to its own Flask proxy server-side, so CORS is unneeded and its removal cuts CSRF surface. Documented the LAN-trust model.
- Whitelisted the `Content-Disposition` filename (header-injection / quote-breakout guard).
- Dashboard proxy hardened earlier: path allowlist + forced `octet-stream`+`nosniff` + sanitized name (`apps/nmminer-control/server.py`).

## Validation
- `pio run -e ESP32-2432S028R` → SUCCESS, Flash 68.9% (2.17 MB / 3.1 MB), RAM 16.9%.
- Test suite: `tests/test_server.py`, `test_server_endpoints.py`, `test_e2e.py` — **73 passed, 94% coverage (statement + branch)** including a real mock-firmware↔proxy E2E chain. On-device E2E is the hardware-gated 08-08.

## Notes / follow-ups
- Firmware `sdstore.cpp::sanitizePath` has a dead null-byte loop (moot for C strings; proxy rejects `\x00`). Cosmetic — left for a follow-up.
- The base already ships `drivers/storage/SDCard.h`; `sdstore` is a higher-level FAT32/CSV/config/sanitized wrapper layered on top.
