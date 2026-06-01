# Project Instructions

This is a hardware bring-up and recovery repo for an ESP32 NMMiner novelty miner.

- Do not commit secrets, Wi-Fi passwords, wallet values, device licenses, serial logs, or flash backups.
- Preserve a full flash backup before any firmware write.
- Prefer official NMMiner docs, release metadata, and flasher behavior over guesses from third-party listings.
- The observed unit is `esp32-2432s028r-st7789`; avoid flashing the `ili9341` build unless the hardware is re-identified.
- ESP32 Wi-Fi is 2.4 GHz only. Do not configure it to a 5 GHz-only SSID.
- Manual recovery/config path for this board: long-press `BOOT`, join `nmap-2.4g`, browse to `192.168.4.1`.

