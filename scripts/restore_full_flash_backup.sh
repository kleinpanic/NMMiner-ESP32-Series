#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-/dev/ttyUSB0}"
BAUD="${BAUD:-921600}"
BACKUP="${1:-hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin}"

if [[ ! -f "${BACKUP}" ]]; then
  echo "Missing backup: ${BACKUP}" >&2
  exit 1
fi

.venv/bin/python -m esptool \
  --chip esp32 \
  --port "${PORT}" \
  --baud "${BAUD}" \
  write-flash -z 0x0 "${BACKUP}"

