# Flashing NerdMiner-CYD-Custom (ESP32-2432S028R)

> ⚠️ Flashing **overwrites the currently running miner**. Back up first.
> The stock unit on the LAN keeps mining until you flash. Flashing requires the
> board connected over **USB serial** (it shows up as `/dev/ttyUSB0`, CH340).

## 1. Back up the existing firmware FIRST (do not skip)

```bash
# 4 MB full-flash backup (restore path if anything goes wrong)
esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  read_flash 0x0 0x400000 stock-backup-$(date +%F).bin
sha256sum stock-backup-*.bin   # record the checksum
```
(A prior backup already exists in the build workspace at
`hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin`.)

## 2. Build

```bash
pio run -e ESP32-2432S028R
```
Display note: this env uses the upstream CYD driver (ILI9341). If your panel is
the **ST7789** variant (this unit's stock target was `esp32-2432s028r-st7789`)
and colors are wrong/inverted after flashing, switch the TFT_eSPI driver to
ST7789 and rebuild.

## 3. Flash

```bash
pio run -e ESP32-2432S028R -t upload --upload-port /dev/ttyUSB0
```

## 4. Configure (no secrets are committed)

On first boot it starts a WiFiManager AP portal — set WiFi SSID, pool, and BTC
wallet there, **or** drop a `config.json` on the SD card. To keep this unit's
existing identity: pool `pool.nerdminers.org:3333` + your recovered BTC wallet.

## 5. On-device validation checklist

- [ ] Boots; display renders (correct colors → driver matches the panel)
- [ ] Joins WiFi; pool connects (boot chime plays on connect)
- [ ] Accepted share → short beep
- [ ] Pull WiFi/pool → alarm tone
- [ ] `curl http://<miner-ip>/api/status` → JSON with `sd` + `speaker` blocks
- [ ] `curl -X POST http://<miner-ip>/api/speaker/test-tone` → audible tone
- [ ] microSD inserted → `/api/sd/list?path=/sd` lists files; CSV log grows
- [ ] Dashboard (`apps/nmminer-control`) shows SD browser + speaker panel live

## Rollback

```bash
esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x0 stock-backup-YYYY-MM-DD.bin
```
