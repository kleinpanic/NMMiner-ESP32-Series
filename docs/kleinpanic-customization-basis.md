# Kleinpanic Customization Basis

Date: 2026-06-01.

This note uses public GitHub context from `kleinpanic` plus the NMMiner audit to guide customization decisions.

## GitHub Profile Signals

GitHub account inspected with `gh`:

- User: `kleinpanic`
- Display name: Klein
- Site: `https://kleinpanic.com`
- Public repos: 57 at audit time
- Active languages across non-archived, non-fork public repos:
  - Python: 18
  - C: 13
  - Shell: 8
  - TypeScript: 6
  - HTML: 3
  - JavaScript: 3

Recurring themes from profile/repositories:

- Linux/Debian-first workflows.
- Open source, no DRM, privacy-first positioning.
- Terminal-compatible tools, curl-friendly interfaces, and TUI/CLI utilities.
- Low-level systems work in C plus pragmatic Python automation.
- Homelab-style monitoring and control: UPS orchestration, alarm Pi, OBD dashboard, Mission Control.
- Agent tooling and memory/orchestration projects: `memory-spark`, `mission-control`, OpenClaw-related repos.
- Visual/terminal identity: ASCII/ANSI, badges, status dashboards, progress bars, and dense technical readouts.

## Official NMMiner Software Repos

Official account inspected: `NMminer1024`.

Relevant repos:

| Repo | Purpose | Status for this device |
| --- | --- | --- |
| `NMminer1024/NMMiner` | Official ESP32 NMMiner firmware bundle, docs, flasher assets, prebuilt binaries | The official repo for the software running on this CYD miner. Firmware source was not published in this repo. |
| `NMminer1024/NMController_web` | Python/Flask LAN monitor | Source-available and useful for local customization. |
| `NMminer1024/NMController_client` | Windows C#/WPF monitor/config client | Source-available, less useful on this Linux laptop. |
| `NMminer1024/ESP-Miner-NMAxe` | ESP32 controller software for NMAxe ASIC hardware | Official, but for different hardware than this ESP32 lottery miner. |
| `NMminer1024/PoolSimulator` | SHA256D mining pool simulator | Useful later for hashrate/protocol experiments, not firmware customization. |
| `NMminer1024/btc-ckpool-solo` | Solo pool software | Pool-side reference, not device firmware. |

GitHub code searches for exact firmware strings and this board target did not find a public firmware source mirror:

- `"NMMiner::begin: all tasks started"`
- `"src/drivers/storage/settings.cpp" "NMMiner"`
- `"esp32-2432s028r-st7789"`

Conclusion: the official repo for this hardware/software is `NMminer1024/NMMiner`, but the current CYD ESP32 firmware should still be treated as binary-only.

## Customization Direction

Best fit for Kleinpanic-style customization:

1. **Local-first control surface**
   - Build a LAN-only dashboard that polls the miner HTTP API.
   - Keep it curl-friendly: every important UI action should also have a documented CLI command.
   - Avoid cloud dependencies for normal operation.

2. **`nmminerctl` CLI**
   - Python first, because the project already uses Python tooling and the device API is simple.
   - Commands:
     - `status`
     - `watch`
     - `set-pool`
     - `set-display`
     - `set-weather`
     - `upload-screensaver`
     - `backup-config`
   - Output modes:
     - human table
     - JSON
     - Prometheus-style metrics

3. **Kleinpanic dashboard**
   - Dense operational UI, closer to Mission Control/UPS dashboard than a marketing page.
   - Panels:
     - miner health
     - pool connection
     - accepted/rejected shares
     - best difficulty
     - Wi-Fi RSSI
     - LittleFS space
     - current config
     - actions/reboot log
   - Visual identity:
     - terminal-inspired, high contrast
     - compact cards/tables
     - ASCII/status-bar motifs
     - restrained magenta/green accents from the profile identity

4. **Device screen personalization**
   - Use the official screensaver path first.
   - Generate 320x240 `.ss` screensaver files from:
     - Kleinpanic ASCII logo
     - mining stats/status panels
     - profile-style progress bars
   - This is low risk because `/api/update/screensaver` is documented and the firmware validates `.ss` files.

5. **Avoid copying unlicensed upstream UI code**
   - `NMminer1024/NMMiner` and `NMController_web` did not report licenses via GitHub API.
   - For a redistributable project, write a clean dashboard against the HTTP API instead of copying their embedded HTML/CSS/JS wholesale.
   - Local-only personal modifications are lower risk, but still harder to share.

## Safe First Implementation

The first real customization should be a small local package in this repo:

```text
tools/nmminerctl/
  pyproject.toml
  nmminerctl/
    __main__.py
    api.py
    screensaver.py
    render.py
```

Start with:

- `nmminerctl status --host 192.168.1.136`
- `nmminerctl watch --host 192.168.1.136`
- `nmminerctl config --host 192.168.1.136`
- `nmminerctl screensaver from-text --text kleinpanic --out kleinpanic.ss`

Then add a local dashboard after the API client is stable.

## Boundaries

Do not start with firmware patching. The firmware source is not published, and the device is already stable on `v2.0.02`.

Do not depend on an SD/TF card. The official manual says the TF slot is reserved for a future NMMiner version, and the current setup works without it.
