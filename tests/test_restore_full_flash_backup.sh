#!/usr/bin/env bash
# Safety regression checks. The ESP tool is a stub; no serial device is opened.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$root/logs/test-sandboxes"
sandbox="$(mktemp -d "$root/logs/test-sandboxes/restore-XXXXXX")"
trap 'rm -rf -- "$sandbox"' EXIT
mkdir -p "$sandbox/scripts" "$sandbox/.venv/bin" "$sandbox/run"
cp "$root/scripts/restore_full_flash_backup.sh" "$sandbox/scripts/restore.sh"
cat > "$sandbox/.venv/bin/python" <<'MOCK'
#!/usr/bin/env bash
set -euo pipefail
args=("$@")
command="${args[8]}"
printf '%s\n' "$command" >> "$MOCK_LOG"
if [[ "$command" == read_flash ]]; then
  [[ "$MOCK_FAIL" == read ]] && exit 9
  size=4194304
  [[ "$MOCK_FAIL" == short ]] && size=1024
  truncate -s "$size" "${args[11]}"
fi
if [[ "$command" == verify_flash ]]; then
  if [[ "$MOCK_FAIL" == verify_current && "${args[10]}" == *pre-restore* ]]; then exit 10; fi
  if [[ "$MOCK_FAIL" == verify_restored && "${args[10]}" == *target.bin ]]; then exit 11; fi
fi
if [[ "$command" == write_flash && "$MOCK_FAIL" == write ]]; then exit 12; fi
MOCK
chmod +x "$sandbox/.venv/bin/python"
export MOCK_LOG="$sandbox/calls"

check() {
  local name="$1" failure="$2" expected="$3" code=0
  export MOCK_FAIL="$failure"
  rm -f "$MOCK_LOG" "$sandbox/target.bin.sha256"
  truncate -s 4194304 "$sandbox/target.bin"
  [[ "$failure" == invalid ]] && truncate -s 1024 "$sandbox/target.bin"
  if [[ "$failure" == checksum ]]; then
    printf '%064d  target.bin\n' 0 > "$sandbox/target.bin.sha256"
  fi
  (cd "$sandbox/run" && bash "$sandbox/scripts/restore.sh" "$sandbox/target.bin") \
    > "$sandbox/output" 2>&1 || code=$?
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
check 'invalid restore image' invalid ''
check 'checksum mismatch' checksum ''
check 'write failure' write 'read_flash,verify_flash,write_flash'
check 'restored image verification failure' verify_restored 'read_flash,verify_flash,write_flash,verify_flash'
