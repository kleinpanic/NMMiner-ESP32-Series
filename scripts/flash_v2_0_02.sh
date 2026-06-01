#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-/dev/ttyUSB0}"
BAUD="${BAUD:-921600}"
TARGET="${1:-esp32-2432s028r-st7789}"
BASE="firmware/v2.0.02/${TARGET}"

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

.venv/bin/python -m esptool \
  --chip esp32 \
  --port "${PORT}" \
  --baud "${BAUD}" \
  write-flash -z \
  0x1000 "${BASE}/bootloader.bin" \
  0x8000 "${BASE}/partitions.bin" \
  0xe000 "${BASE}/boot_app0.bin" \
  0x10000 "${BASE}/firmware.bin" \
  0x390000 "${BASE}/littlefs.bin"

