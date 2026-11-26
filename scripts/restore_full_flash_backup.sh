#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# The CH340 can return with a different ttyUSB number after USB is reconnected.
if [[ -z "${PORT:-}" ]]; then
  PORT="/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"
  [[ -e "$PORT" ]] || PORT="/dev/ttyUSB0"
fi
BAUD="${BAUD:-460800}"
BACKUP="${1:?usage: $0 <full-flash-backup.bin>}"
ESPTOOL=("$ROOT/.venv/bin/python" -m esptool --chip esp32 --port "$PORT" --baud "$BAUD")

if [[ ! -f "$BACKUP" || $(stat -c %s -- "$BACKUP") -ne 4194304 ]]; then
  echo "Expected an existing full 4 MB backup: $BACKUP" >&2
  exit 1
fi
if [[ ! -x "$ROOT/.venv/bin/python" ]]; then
  echo "Missing project Python: $ROOT/.venv/bin/python" >&2
  exit 1
fi
if [[ -f "$BACKUP.sha256" ]]; then
  expected="$(awk 'NR == 1 { print $1 }' "$BACKUP.sha256")"
  actual="$(sha256sum -- "$BACKUP")"
  if [[ ! "$expected" =~ ^[[:xdigit:]]{64}$ || "${actual%% *}" != "$expected" ]]; then
    echo "Backup checksum mismatch: $BACKUP" >&2
    exit 1
  fi
fi

# Keep the current device recoverable even when restoring an older image.
# mktemp prevents a second restore from overwriting the recovery copy.
umask 077
mkdir -p "$ROOT/hardware/backups"
current="$(mktemp "$ROOT/hardware/backups/cyd-backup-$(date +%Y%m%d-%H%M%S)-pre-restore-XXXXXX.bin")"
"${ESPTOOL[@]}" read_flash 0x0 0x400000 "$current"
if [[ $(stat -c %s -- "$current") -ne 4194304 ]]; then
  echo "Current-device backup is not 4 MB; restore aborted: $current" >&2
  exit 1
fi
sha256sum -- "$current" > "$current.sha256"
"${ESPTOOL[@]}" verify_flash 0x0 "$current"

"${ESPTOOL[@]}" write_flash -z 0x0 "$BACKUP"
"${ESPTOOL[@]}" verify_flash 0x0 "$BACKUP"
echo "Restored and verified $BACKUP; previous device image: $current"

