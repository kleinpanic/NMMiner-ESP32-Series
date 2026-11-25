# NMMiner Control UI Review

Date: 2026-06-01.

Surface audited: `apps/nmminer-control`, served at `http://127.0.0.1:8788`.

> Current-state note, 2026-06-19: this is a historical local-dashboard review
> against the official LAN baseline. The live unit is now the custom decoded-spec
> ST7789 fork in WiFiManager recovery AP mode. Custom JSON API, dashboard,
> screensaver upload, LAN, and mining behavior are not current validation proof
> until Phase 7 LAN/API/mining gates pass.

## Result

Overall score: 21 / 24.

| Pillar | Score | Notes |
| --- | ---: | --- |
| Copywriting | 4 | Labels are direct and operational. Status values are terse enough for repeated use. |
| Visuals | 4 | Dense terminal-style monitor fits the miner/operator use case and avoids marketing layout. |
| Color | 3 | Magenta/green/red status language is clear. The palette is intentionally narrow, but not a single-hue theme. |
| Typography | 3 | Monospace fits the device/debugging context. Metric type is now fixed-size rather than viewport-scaled. |
| Spacing | 3 | Desktop and mobile spacing are stable with no horizontal overflow. The mobile header is tall but readable. |
| Experience Design | 4 | The dashboard surfaces live state, pool health, sanctioned settings, and screen saver upload without touching firmware internals. |

## Findings Fixed

1. The original background used a radial glow. Replaced it with linear treatments to keep the UI quieter and avoid decorative orb styling.
2. Metric numbers used viewport-based font scaling. Replaced with fixed responsive sizes so layout is predictable.
3. Icon-only topbar buttons depended entirely on the CDN icon script. Added visible fallback labels when Lucide fails and hide them once icons initialize.
4. Partial miner API failures could show an unhelpful `HTTP 200` toast. The client now treats dashboard/settings partial payloads as renderable and preserves actual errors for true request failures.
5. The live dashboard poll interval was reduced to 10 seconds to put less pressure on the ESP32 web server.

## Rendered Verification

Playwright/browser-library audit was run through Chromium because the Browser plugin was not available.

| Check | Desktop 1440x1000 | Mobile 390x844 |
| --- | --- | --- |
| Page identity | Pass | Pass |
| Meaningful content | Pass | Pass |
| Framework overlay | Pass | Pass |
| Console/page errors | Pass | Pass |
| Horizontal overflow | Pass | Pass |
| Interaction proof | Pass: Device, Screen, Weather tabs | Pass: Device, Screen, Weather tabs |

Screenshots captured outside the repo:

- `/tmp/nmminer-ui-audit-desktop.png`
- `/tmp/nmminer-ui-audit-mobile.png`

## Historical Device UI Customization Surface

Safe on-device personalization exposed by the official 2026-06-01 firmware/API baseline:

- Hostname and time display: hostname, timezone, 12/24-hour time, date format.
- Display behavior: brightness, rotation, LED enable, screensaver timeout, screensaver mode.
- Status pages: market main coin/watchlist/K-line settings and weather location/units.
- Visual assets: upload valid NMMiner `.ss` screensaver files through `/api/update/screensaver`.

Not safely customizable from the current public source bundle:

- The physical LVGL page layout/order.
- Mining math, stratum internals, and share validation behavior.
- Firmware UI assets inside `firmware.bin`.

Those require upstream firmware source or risky binary/LittleFS patching on the stock image. For the current custom fork, treat the local dashboard and `.ss` screensaver path as historical context until Phase 7 proves LAN/API/mining behavior again.
