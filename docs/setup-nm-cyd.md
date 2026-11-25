# NM-CYD / ESP32-2432S028 Setup Procedure

> Current-state note, 2026-06-19: this document preserves the 2026-06-01
> official-firmware setup baseline. It is not the current live device state.
> The live unit is now the custom decoded-spec `ESP32-2432S028R` ST7789 fork in
> WiFiManager recovery AP mode (`nmap-2.4g` / `192.168.4.1`), reporting recovered
> `v2.0.02` identity. It is not currently proven on LAN, mining, or touch-working.
> Use `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md` for
> the active recovery state.

## What We Know

- Board: NM-CYD / ESP32-2432S028, CYD 2.8.
- Chip from `esptool`: ESP32-D0WD-V3, 4 MB flash.
- Original firmware from serial boot log: NMMiner `v1.8.28`.
- Historical official-firmware upgrade target: NMMiner `v2.0.02`.
- Latest official release found during the 2026-06-01 baseline audit: `v2.0.02`, published 2026-05-09.
- Current public flasher check on 2026-06-19 lists `v2.0.03` for `esp32-2432s028r-st7789`; this setup record remains scoped to the local `v2.0.02` official backup/baseline.
- Current display firmware target: `esp32-2432s028r-st7789`.
- Full flash backup completed before any write.
- Historical baseline LAN address: `http://192.168.1.136`.
- Historical baseline Wi-Fi SSID: `Homebase`.
- Historical baseline timezone: `-4`; NTP sync observed in the serial log.

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

During the 2026-06-01 baseline audit, the device was configured for `Homebase` and confirmed online at `192.168.1.136`. In the current Phase 7 recovery state, the custom firmware is in WiFiManager AP mode instead. Do not connect the laptop to the miner AP (`nmap-2.4g`) while relying on the laptop's Wi-Fi for internet access; use USB serial or the LAN API when LAN mode is available.

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

Runtime pool check from this network:

| Pool | Result |
| --- | --- |
| `pool.nerdminers.org:3333` | Responded to `mining.subscribe` and advertised difficulty `0.001`. |
| `public-pool.io:21496` | TCP connected, but did not return a subscribe response. |
| `pool.tazmining.ch:33333` | Timed out from this network. |
| `solobtc.nmminer.com:3333` | Responded to `mining.subscribe`. |
| `au.solobtc.nmminer.com:3333` | Responded to `mining.subscribe`. |

Historical baseline configured pools:

- Primary: `stratum+tcp://pool.nerdminers.org:3333`
- Fallback: `stratum+tcp://pool.nerdminers.org:3333`

The 2026-06-01 post-reboot API verification showed active mining on `pool.nerdminers.org:3333`, accepted shares increasing, and zero rejected shares. The current Phase 7 custom recovery state has not yet re-proven Wi-Fi, pool connection, hashrate, or accepted shares.

## Firmware Upgrade Notes

Original: `v1.8.28`.

Local backed-up official baseline: `v2.0.02`.

Current public flasher listing checked on 2026-06-19: `v2.0.03` is also listed
for `esp32-2432s028r-st7789`. Do not silently switch the local RE/custom-edit
baseline from `v2.0.02` to `v2.0.03` without a new provenance record and fresh
extraction/decompilation pass.

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

Reflash command:

```bash
./scripts/flash_v2_0_02.sh
```

Historical 2026-06-01 restore example only. For the current Phase 7 state, use the latest restore target/runbook in `.planning/STATE.md` and `.planning/phases/07-on-device-validation/07-VALIDATION.md`; do not restore or flash without a fresh full 4 MB backup and an explicit operator decision.

```bash
.venv/bin/python -m esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash -z 0x0 hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin
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
./scripts/send_serial_config.py --ssid 'Homebase' --password '<wifi password>' --wallet '<btc wallet>' --delay 4.5
```

If the serial config path does not take, use the AP configuration path instead: long-press `BOOT`, join `nmap-2.4g`, open `http://192.168.4.1`.

The successful setup used a single timed serial write after opening `/dev/ttyUSB0`; repeated writes are supported by the helper for timing experiments, but a single correctly timed write avoids concatenating multiple JSON objects in the firmware parser.

## API Verification

Historical LAN-mode readbacks from the 2026-06-01 baseline. These commands are not expected to work while the current custom firmware is in recovery AP mode:

```bash
curl http://192.168.1.136/probe
curl http://192.168.1.136/api/system/info
curl http://192.168.1.136/api/setting/network
curl http://192.168.1.136/api/setting/mining
curl http://192.168.1.136/api/setting/time
```

Historical known-good final official-firmware state:

- Firmware: `v2.0.02`
- Hostname: `cyd2.8_1a76f7`
- Wi-Fi SSID: `Homebase`
- Timezone: `-4`
- Primary pool: `stratum+tcp://pool.nerdminers.org:3333`
- Secondary pool: `stratum+tcp://pool.nerdminers.org:3333`
- Recovered wallet configured for both primary and secondary pool users
