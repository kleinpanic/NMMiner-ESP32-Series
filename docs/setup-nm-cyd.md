# NM-CYD / ESP32-2432S028 Setup Procedure

## What We Know

- Board: NM-CYD / ESP32-2432S028, CYD 2.8.
- Chip from `esptool`: ESP32-D0WD-V3, 4 MB flash.
- Current firmware from serial boot log: NMMiner `v1.8.28`.
- Latest official release found via GitHub/API and flash manifest: `v2.0.02`, published 2026-05-09.
- Current display firmware target: `esp32-2432s028r-st7789`.
- Full flash backup completed before any write.

The current flash backup contains the old Wi-Fi credentials, wallet, pools, and device license. Those recovered values are written in `secrets/recovered-device-config-2026-06-01.txt`, which is intentionally ignored by git.

## Official Manual Flow

For NM-CYD / ESP32-2432S028:

- `BOOT` double click: switch to next page.
- `BOOT` long press: force configuration.
- `RESET` click: reset the miner.
- The TF slot and external I/O are reserved for a later NMMiner version.

Configuration behavior:

1. On boot, the miner tries the last saved Wi-Fi.
2. If it cannot connect within about 15 seconds, it enters configuration mode.
3. Join the open AP named `nmap-2.4g`.
4. On a PC, browse to `http://192.168.4.1`.
5. Save settings and wait 10-15 seconds for the automatic restart.

## Wi-Fi Constraint

The ESP32 supports 2.4 GHz Wi-Fi only. A scan from this laptop saw:

- `nmap-2.4g` on channel 1, 2412 MHz, signal 100.
- `Homebase5` on channels 36 and 161, both 5 GHz.

The intended long-term SSID is `Homebase`, which should be the 2.4 GHz network. Do not use `Homebase5`; as scanned here, `Homebase5` is the 5 GHz network and the ESP32 cannot join it.

## Pool References

The Amazon manual lists these BTC low-difficulty pools:

| Pool | Type | Port | Minimum difficulty |
| --- | --- | --- | --- |
| `pool.nerdminers.org` | tcp | 3333 | 0.001 |
| `public-pool.io` | tcp | 21496 | 0.001 |
| `pool.tazmining.ch` | tcp | 33333 | 0.001 |

The official GitHub README for newer NMMiner firmware also lists:

- `stratum+tcp://solobtc.nmminer.com:3333`
- `stratum+tcp://au.solobtc.nmminer.com:3333`
- `stratum+tcp://dgb-stratum.solominer.net:3333` for DGB

For this setup, keep BTC pools low difficulty and avoid normal high-difficulty pools.

## Firmware Upgrade Notes

Current: `v1.8.28`.

Latest official manifest/release: `v2.0.02`.

Downloaded local release files are under `firmware/v2.0.02/` and are gitignored. The correct target for this unit is:

```text
firmware/v2.0.02/esp32-2432s028r-st7789/
```

The official flasher writes these offsets for `v2.0.02` on this board:

| Offset | File |
| --- | --- |
| `0x1000` | `bootloader.bin` |
| `0x8000` | `partitions.bin` |
| `0xe000` | `boot_app0.bin` |
| `0x10000` | `firmware.bin` |
| `0x390000` | `littlefs.bin` |

Prepared command:

```bash
./scripts/flash_v2_0_02.sh
```

Recovery command from full backup:

```bash
.venv/bin/python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 write-flash -z 0x0 hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin
```

## Serial Configuration

The official flash tool sends a JSON object over USB serial with these keys:

```json
{
  "ssid": "SSID",
  "password": "PASSWORD",
  "address": "BTC_WALLET",
  "address2": "BTC_WALLET",
  "primary_pool_url": "stratum+tcp://public-pool.io:21496",
  "primary_pool_password": "x",
  "fallback_pool_url": "stratum+tcp://pool.tazmining.ch:33333",
  "fallback_pool_password": "x"
}
```

Use the helper:

```bash
./scripts/send_serial_config.py --ssid 'Homebase' --password '<wifi password>' --wallet '<btc wallet>'
```

If the serial config path does not take, use the AP configuration path instead: long-press `BOOT`, join `nmap-2.4g`, open `http://192.168.4.1`.
