# Code, UI, And Mining-Math Audit Notes

## Initial Finding

The official `NMminer1024/NMMiner` repository is not a full firmware source tree. The current tree is mainly:

- README and API docs.
- Images/screenshots.
- USB serial driver installer.
- Prebuilt firmware binaries for supported boards.
- NMController tool folders.

That means the on-device LVGL visual UI and mining core are not immediately source-auditable from that repo. The repo does expose enough artifacts to audit behavior and customization boundaries.

## Source-Available Or Inspectable Targets

- `NMController_web`: Python/Flask LAN monitor source.
- `NMController_web/templates/web_monitor.html`: browser monitor UI.
- Official flash tool JavaScript saved as `docs/research/flash-nmminer-script.js`.
- Device HTTP API reference saved as `docs/research/nmminer-api-reference.md`.
- Firmware/LittleFS binaries and extracted strings.
- Runtime serial logs.

## Early Audit Observations

- `NMController_web` listens for UDP broadcast miner status on port `12345`.
- `NMController_web` serves a Flask monitor on port `7877`.
- The monitor tracks IP, board, hashrate, shares, network difficulty, last difficulty, best difficulty, block hits, temperature, RSSI, free heap, uptime, last seen, and firmware version.
- The official flash tool uses serial JSON config keys: `ssid`, `password`, `address`, `address2`, `primary_pool_url`, `primary_pool_password`, `fallback_pool_url`, `fallback_pool_password`.
- For firmware `v2.0.02`, the flasher writes `firmware.bin` at `0x10000` and `littlefs.bin` at `0x390000`.
- Firmware strings reveal LVGL, TFT_eSPI, Wi-Fi, stratum, wallet validation, monitor web UI, and REST/API behavior.
- Runtime API on `v2.0.02` exposes enough configuration to customize Wi-Fi, pools, timezone/date format, display preferences, market watch symbols, and weather without firmware patching.
- Runtime mining verification showed `pool.nerdminers.org:3333` works for this device/network with difficulty `0.001`; `public-pool.io:21496` TCP connects but did not complete subscribe during testing.

## Customization Paths

Low risk:

- Customize NMController Web UI locally.
- Build a separate local dashboard against the NMMiner HTTP API.
- Tune configuration values: pools, wallet/worker, selected coins, timezone, refresh interval, screen timeout, brightness, auto brightness.

Medium risk:

- Inspect and potentially replace LittleFS web assets if a reliable unpack/repack tool is introduced and tested against a restore path.
- Use the documented HTTP API to automate configuration and monitoring.
- Add a local dashboard/custom controller that treats the device HTTP API as the stable boundary.

High risk:

- Patch `firmware.bin` directly.
- Modify on-device LVGL screens or mining math without upstream source.
- Flash an unverified display target.

## Audit Questions For The Phase

- Can upstream provide firmware source or a build recipe?
- Can `littlefs.bin` be unpacked and repacked reproducibly?
- Which UI is worth customizing: on-device screen, embedded web monitor, NMController Web, or a new local dashboard?
- Is the mining math exposed in source anywhere, or only in prebuilt firmware?
- Can customization be done without compromising license activation or NVS config?
