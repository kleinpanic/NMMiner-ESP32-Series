#!/usr/bin/env bash
set -euo pipefail

export NMMINER_URL="${NMMINER_URL:-http://192.168.1.136}"
export NMMINER_CONTROL_HOST="${NMMINER_CONTROL_HOST:-127.0.0.1}"
export NMMINER_CONTROL_PORT="${NMMINER_CONTROL_PORT:-8788}"

exec python3 apps/nmminer-control/server.py
