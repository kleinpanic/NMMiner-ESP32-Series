#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
if [[ ! -x "$ROOT/.venv/bin/python" ]]; then
  echo 'Create the project .venv with uv and install requirements.txt first.' >&2
  exit 1
fi

export NMMINER_URL="${NMMINER_URL:-http://192.168.1.136}"
export NMMINER_CONTROL_HOST="${NMMINER_CONTROL_HOST:-127.0.0.1}"
export NMMINER_CONTROL_PORT="${NMMINER_CONTROL_PORT:-8788}"

exec "$ROOT/.venv/bin/python" apps/nmminer-control/server.py
