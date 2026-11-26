# Temporary Wi-Fi isolation firmware

This CYD diagnostic uses the same Espressif platform as PanicMiner. It reads the
existing `/config.json` without formatting or changing the filesystem, and never
prints network credentials, wallet, pool address or device addresses. Wi-Fi driver
configuration is kept in RAM (`WiFi.persistent(false)`).

It disables the backlight and deselects SD without starting display/touch SPI,
mining, SD storage, TLS or audio. It tries four radio profiles for 25 seconds each.
If joined, it enables the backlight at 64/255 after one minute and 255/255 after
two, allowing display power load to be checked independently of display traffic.
Serial output is prefixed `[RADIO]`; `JOINED` means DHCP succeeded, not pool work.

Build from the repository root:

```bash
./.venv/bin/python -m platformio run -d firmware/diagnostics/wifi-only -e cyd-wifi
```

Use `scripts/flash_fork_build.sh` with this build directory only for a bounded
hardware test; it takes and verifies a fresh full backup before writing. The
screen will be dark initially. Restore the normal PanicMiner build afterward,
again through the backup/verify helper. This firmware does not mine and is not
the normal device build.
