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
