# PanicMiner local HTTP API

Served by the firmware on port 80 after WiFi connects (see `src/webapi.cpp`).
Consumed by the local dashboard `apps/nmminer-control`. All responses are JSON
unless noted. The API uses LAN trust, has no authentication, and does not send
a wildcard CORS header. Keep it on a trusted local network.

## Status

`GET /api/status` — also aliased as `GET /api/system/info`

```json
{
  "miner": {
    "sharesAccepted": 0, "sharesRejected": 0, "poolAuthorized": false,
    "bestVerifiedDifficulty": 0, "poolDifficulty": 0,
    "hashrate10sKhs": 0, "hashrate60sKhs": 0,
    "heapFreeBytes": 0, "heapTotalBytes": 0, "chipTempC": 0,
    "totalMHashes": 0, "uptimeSeconds": 0
  },
  "sd": { "mounted": false, "rowsSaved": 0, "total": 0, "used": 0, "free": 0 },
  "speaker": { "volume": 20, "mute": false },
  "wallet": { "valid": false }
}
```

When `wallet.valid` is true, `wallet` also contains `balanceSats` (confirmed
address balance), `pendingSats` (signed unconfirmed transaction delta), and
`ageSeconds` (time since the last successful address query). These fields are
omitted until a successful query; a valid zero balance is distinct from missing
data. Refreshes use certificate-verified mempool.space HTTPS approximately every
45 seconds, reusing the authenticated connection when available. The address
comes from the saved pool worker with its suffix
removed; the API does not return that address or any signing material. The
balance can include transfers unrelated to mining and is not a share reward.

## Speaker

| Method | Path | Body | Effect |
|--------|------|------|--------|
| GET  | `/api/speaker` | — | `{ "volume": 0-100, "mute": bool }` |
| POST | `/api/speaker/volume` | `{ "volume_percent": 0-100 }` | set volume; 400 if out of range |
| POST | `/api/speaker/mute` | `{ "muted": true|false }` | set mute |
| POST | `/api/speaker/test-tone` | optional `{ "freq_hz", "duration_ms" }` | play tone (defaults if omitted) |

PanicMiner stays silent at boot and disconnect. A 65 ms chirp marks a
pool-confirmed share, limited to once per second; a prioritized melody marks a
pool-accepted network-target block. These report work events, not wallet payments.
Playback runs without delays on LEDC GPIO26/channel 1, separate from the TFT
backlight. Test tones accept 100–10,000 Hz and cap duration at two seconds.

## SD card

All `path` inputs pass `SDStore::sanitizePath` (rejects traversal, ambiguous
separators and control characters, constrains to `/sd/`). Config and secrets
paths are denied at the API boundary. Downloads stream from the card.
Only one SD download runs at a time; a second receives `503` with
`download busy`. The transfer sends at most 512 bytes per main-loop pass,
retains its original Content-Length even if logging appends rows, and closes
on failed progress, insufficient memory or a 180-second deadline. Clients must
verify Content-Length before treating a download as complete.

| Method | Path | Input | Returns |
|--------|------|-------|---------|
| GET  | `/api/sd/list?path=/sd` | query `path` | `{ ok, path, entries:[...] }` |
| GET  | `/api/sd/download?path=/sd/x` | query `path` | file bytes, `attachment` + `nosniff` |
| POST | `/api/sd/delete` | `{ "path": "/sd/x" }` | `{ ok:true }` |
| GET  | `/api/sd/log` | — | CSV of measured stats (`/sd/mining_stats_v2.csv`) |

The CSV records uptime, 10/60-sample rate, accepted/rejected work, best verified
difficulty, mining time, free heap and chip temperature once per minute. Older
CSV files remain intact. `rowsSaved` counts successful writes since this boot.

Errors: `400` invalid/missing path, `404` not found, `503` SD unavailable —
`{ "ok": false, "error": "..." }`. If no card is present the miner keeps running;
SD endpoints report unavailable rather than blocking.
