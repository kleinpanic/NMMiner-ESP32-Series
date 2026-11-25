#!/usr/bin/env bash
# Flash a PlatformIO build of the fork to the CYD.
# Always takes a full 4 MB backup first, then writes, then verifies.
# Usage: scripts/flash_fork_build.sh <label> [build_dir]
set -euo pipefail

PORT="${PORT:-/dev/ttyUSB0}"
BAUD="${BAUD:-921600}"
LABEL="${1:?usage: $0 <label> [build_dir]}"
BUILD="${2:-firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R}"
BOOT_APP0="${BOOT_APP0:-$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin}"
ESPTOOL=(.venv/bin/esptool.py --chip esp32 --port "$PORT" --baud "$BAUD")

for f in "$BUILD/bootloader.bin" "$BUILD/partitions.bin" "$BUILD/firmware.bin" "$BOOT_APP0"; do
  [[ -f "$f" ]] || { echo "missing $f" >&2; exit 1; }
done

backup="hardware/backups/cyd-backup-$(date +%Y%m%d-%H%M%S)-pre-$LABEL.bin"
"${ESPTOOL[@]}" read_flash 0x0 0x400000 "$backup"
[[ $(stat -c %s "$backup") -eq 4194304 ]] || { echo "backup is not 4 MB: $backup" >&2; exit 1; }
sha256sum "$backup" > "$backup.sha256"

images=(0x1000 "$BUILD/bootloader.bin" 0x8000 "$BUILD/partitions.bin"
        0xe000 "$BOOT_APP0" 0x10000 "$BUILD/firmware.bin")
"${ESPTOOL[@]}" write_flash -z "${images[@]}"
"${ESPTOOL[@]}" verify_flash "${images[@]}"

echo "flashed $LABEL; backup $backup"
echo "restore: ${ESPTOOL[*]} write_flash 0x0 $backup"
