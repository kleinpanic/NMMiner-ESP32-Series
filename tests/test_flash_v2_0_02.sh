#!/usr/bin/env bash
# Production recovery script with a stub ESP tool: no serial hardware access.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$root/logs/test-sandboxes"
sandbox="$(mktemp -d "$root/logs/test-sandboxes/stock-flash-XXXXXX")"
trap 'rm -rf -- "$sandbox"' EXIT
mkdir -p "$sandbox/scripts" "$sandbox/.venv/bin" "$sandbox/run"
cp "$root/scripts/flash_v2_0_02.sh" "$sandbox/scripts/flash.sh"
base="$sandbox/firmware/v2.0.02/esp32-2432s028r-st7789"
mkdir -p "$base"
for image in bootloader partitions boot_app0 firmware littlefs; do
  truncate -s 1024 "$base/$image.bin"
done
cat > "$sandbox/.venv/bin/python" <<'MOCK'
#!/usr/bin/env bash
set -euo pipefail
args=("$@")
operation="${args[8]}"
printf '%s\n' "$operation" >> "$MOCK_LOG"
case "$operation" in
  read_flash)
    [[ "$MOCK_FAIL" == read ]] && exit 9
    size=4194304
    [[ "$MOCK_FAIL" == short ]] && size=1024
    truncate -s "$size" "${args[11]}"
    ;;
  verify_flash)
    if [[ "${args[10]}" == *pre-stock* ]]; then
      [[ "$MOCK_FAIL" == verify_current ]] && exit 10
    else
      [[ "${#args[@]}" == 19 && "${args[17]}" == 0x390000 && "${args[18]}" == */littlefs.bin ]] || exit 20
      [[ "$MOCK_FAIL" == verify_written ]] && exit 11
    fi
    ;;
  write_flash)
    [[ "${#args[@]}" == 20 && "${args[9]}" == -z && "${args[18]}" == 0x390000 && "${args[19]}" == */littlefs.bin ]] || exit 21
    [[ "$MOCK_FAIL" == write ]] && exit 12
    ;;
  *) exit 22 ;;
esac
exit 0
MOCK
chmod +x "$sandbox/.venv/bin/python"
export MOCK_LOG="$sandbox/calls"

check() {
  local name="$1" failure="$2" expected="$3" code=0
  export MOCK_FAIL="$failure"
  rm -f "$MOCK_LOG"
  local target=esp32-2432s028r-st7789
  [[ "$failure" == target ]] && target=invalid
  [[ "$failure" == missing ]] && mv "$base/littlefs.bin" "$base/littlefs.saved"
  (cd "$sandbox/run" && bash "$sandbox/scripts/flash.sh" "$target") > "$sandbox/output" 2>&1 || code=$?
  [[ "$failure" == missing ]] && mv "$base/littlefs.saved" "$base/littlefs.bin"
  local actual=""
  [[ -f "$MOCK_LOG" ]] && actual="$(paste -sd ',' "$MOCK_LOG")"
  if [[ "$actual" != "$expected" || ( "$name" == success && "$code" != 0 ) ||
        ( "$name" != success && "$code" == 0 ) ]]; then
    echo "FAIL: $name ($code): $actual" >&2
    cat "$sandbox/output" >&2
    exit 1
  fi
  printf 'PASS: %s\n' "$name"
}

check success '' 'read_flash,verify_flash,write_flash,verify_flash'
check 'failed backup read' read 'read_flash'
check 'short current backup' short 'read_flash'
check 'failed backup verification' verify_current 'read_flash,verify_flash'
check 'invalid target' target ''
check 'missing filesystem image' missing ''
check 'write failure' write 'read_flash,verify_flash,write_flash'
check 'written image verification failure' verify_written 'read_flash,verify_flash,write_flash,verify_flash'
