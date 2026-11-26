# PanicMiner local HTTP API

Served by the firmware on port 80 after WiFi connects (see `src/webapi.cpp`).

After queuing a complete declared response, the server gives the client up to
two seconds to close first. This avoids retaining a server-side TCP TIME_WAIT
entry when a normal client reuses its source port. A client waiting for server
EOF is closed at that deadline. Request half-close does not cancel a response
body still being sent. SD headers hand off to the retained file sender; only
the end of the complete file starts the close grace period.
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

All API headers and JSON bodies use a retained response buffer, capped at 8 KB,
and at most one nonblocking 512-byte send per loop pass. One response is queued
at a time, with a ten-second total deadline. While it is pending, bulk SD writes
pause so control/status replies receive the next send opportunity. Clients must
check complete HTTP bodies; a disconnected or unresponsive peer can still fail
its own request.

All `path` inputs pass the shared SD normalizer (rejects traversal, ambiguous
separators, control characters and leading/trailing spaces or trailing dots in
components; constrains to `/sd/`). The API additionally denies configuration
and secrets paths case-insensitively and rejects tilde short-name aliases.
Invalid/private paths receive `400` before list, open or delete operations.
Public filenames retain their case and internal spaces. Downloads stream from
the card.
Only one SD download runs at a time; a second receives `503` with
`download busy`. The transfer uses at most one nonblocking socket send per main-loop pass,
with a retained block of at most 512 bytes. Short sends and temporary backpressure
retain unsent bytes for the next pass. It retains its original Content-Length
even if logging appends rows, and closes
on failed progress or a 180-second deadline. Temporary memory pressure pauses
the transfer without discarding pending bytes; it reserves 20 KB free heap
and a 6 KB contiguous block before queuing more bulk data. This leaves
space for concurrent API connections and authenticated feeds even when a
download client stops reading. Zero-timeout write readiness also honors the
SDK TCP send low-water mark, rather than filling the entire send queue. File stdio buffering is limited to 512 bytes.
A stalled transfer closes after 15 seconds without progress, within the
three-minute total deadline; a half-closed HTTP request can still receive its
response. Clients must verify Content-Length before treating a download as complete.

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
