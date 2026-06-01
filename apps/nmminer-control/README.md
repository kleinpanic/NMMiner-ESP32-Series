# Kleinpanic NMMiner Control

Canonical local dashboard for the NM-CYD / ESP32-2432S028 miner in this repo.

Run it from the repo root:

```bash
./scripts/run_nmminer_control.sh
```

Default target:

```text
NMMINER_URL=http://192.168.1.136
```

Default local URL:

```text
http://127.0.0.1:8788
```

The Flask server proxies miner API calls so the browser does not need direct cross-origin access to the device.

To view it from another machine on the same LAN, bind the control server to the laptop's LAN interface:

```bash
NMMINER_CONTROL_HOST=0.0.0.0 ./scripts/run_nmminer_control.sh
```

Then open the laptop's LAN address, for example:

```text
http://192.168.1.109:8788
```

This dashboard does not run on the ESP32 itself. Accessing it through the miner's own IP would require replacing/repacking the miner's embedded LittleFS web UI.

Safe device personalization currently exposed here:

- Device hostname and time display.
- Brightness, rotation, LED, screensaver timeout, and screensaver mode.
- Market, weather, and pool settings.
- `.ss` screensaver preflight and upload.
