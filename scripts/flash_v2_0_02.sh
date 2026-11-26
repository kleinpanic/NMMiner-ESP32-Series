#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -z "${PORT:-}" ]]; then
  PORT="/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"
  [[ -e "$PORT" ]] || PORT="/dev/ttyUSB0"
fi
BAUD="${BAUD:-460800}"
TARGET="${1:-esp32-2432s028r-st7789}"
BASE="$ROOT/firmware/v2.0.02/${TARGET}"
ESPTOOL=("$ROOT/.venv/bin/python" -m esptool --chip esp32 --port "$PORT" --baud "$BAUD")

if [[ "${TARGET}" != "esp32-2432s028r-st7789" && "${TARGET}" != "esp32-2432s028r-ili9341" ]]; then
  echo "Unsupported target: ${TARGET}" >&2
  echo "Expected esp32-2432s028r-st7789 or esp32-2432s028r-ili9341" >&2
  exit 2
fi

for file in bootloader.bin partitions.bin boot_app0.bin firmware.bin littlefs.bin; do
  if [[ ! -f "${BASE}/${file}" ]]; then
    echo "Missing ${BASE}/${file}" >&2
    exit 1
  fi
done

if [[ ! -x "$ROOT/.venv/bin/python" ]]; then
  echo "Missing project Python: $ROOT/.venv/bin/python" >&2
  exit 1
fi

# Stock recovery also replaces its filesystem image. Preserve the whole
# current device and compare it with flash before changing any partition.
umask 077
mkdir -p "$ROOT/hardware/backups"
backup="$(mktemp "$ROOT/hardware/backups/cyd-backup-$(date +%Y%m%d-%H%M%S)-pre-stock-XXXXXX.bin")"
"${ESPTOOL[@]}" read_flash 0x0 0x400000 "$backup"
if [[ $(stat -c %s -- "$backup") -ne 4194304 ]]; then
  echo "Current-device backup is not 4 MB; stock flash aborted: $backup" >&2
  exit 1
fi
sha256sum -- "$backup" > "$backup.sha256"
"${ESPTOOL[@]}" verify_flash 0x0 "$backup"

images=(0x1000 "$BASE/bootloader.bin" 0x8000 "$BASE/partitions.bin"
        0xe000 "$BASE/boot_app0.bin" 0x10000 "$BASE/firmware.bin"
        0x390000 "$BASE/littlefs.bin")
"${ESPTOOL[@]}" write_flash -z "${images[@]}"
"${ESPTOOL[@]}" verify_flash "${images[@]}"
echo "Stock images written and verified; previous device image: $backup"

