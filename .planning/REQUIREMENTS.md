# Requirements

## R1: Preserve Current Device State

- Full flash must be backed up before firmware writes.
- Backup checksum must be recorded.
- Recovered Wi-Fi credentials, wallet, pools, and license must be stored outside git.

## R2: Official Setup Procedure

- Setup docs must follow the NM-CYD / ESP32-2432S028 manual.
- Button behavior must be documented.
- Configuration AP path must be documented.
- Pool choices must prefer low-difficulty pools.

## R3: Firmware Upgrade Readiness

- Current and latest firmware versions must be recorded.
- Correct display target must be identified before flashing.
- Firmware files and checksums must be captured or reproducible.
- Restore path from backup must be documented.

## R4: Network Configuration

- Device must be configured only to a 2.4 GHz SSID.
- If serial config is used, use the same JSON schema as the official flasher.
- If serial config fails, use `nmap-2.4g` and `192.168.4.1`.

## R5: Monitoring

- After Wi-Fi setup, find the miner IP on the LAN.
- Use the embedded web monitor or HTTP API.
- Record useful endpoints and verify mining status, pool, wallet, RSSI, and firmware version.

## R6: Hardware Upgrade Review

- Only after baseline setup is stable, evaluate power supply, enclosure/thermals, screen timeout, and possible alternative ESP32 NMMiner boards.

## R7: Code, UI, And Mining-Math Audit

- Inventory the upstream repositories and determine which parts are source-available.
- Audit `NMController_web`, the flash tool JavaScript, and the device HTTP API for customization points.
- Inspect firmware and LittleFS binaries for web assets, LVGL/UI strings, config schema, and recoverable behavior.
- Determine whether on-device visual UI and mining math can be customized from source, configured through APIs, patched as assets, or only reverse engineered.
- Produce a customization backlog with risk levels before modifying firmware.
