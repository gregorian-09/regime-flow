#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export PYTHONPATH="build/lib:build/python:python${PYTHONPATH:+:${PYTHONPATH}}"
export REGIMEFLOW_TEST_ROOT="${REGIMEFLOW_TEST_ROOT:-$ROOT_DIR}"
export HOST="${HOST:-127.0.0.1}"
export PORT="${PORT:-8050}"
export DASHBOARD_PROFILE="${DASHBOARD_PROFILE:-balanced}"

case "${DASHBOARD_PROFILE}" in
  low_latency)
    export STEPS_PER_TICK="${STEPS_PER_TICK:-1}"
    export INTERVAL_MS="${INTERVAL_MS:-250}"
    ;;
  high_history)
    export STEPS_PER_TICK="${STEPS_PER_TICK:-6}"
    export INTERVAL_MS="${INTERVAL_MS:-1200}"
    ;;
  *)
    export STEPS_PER_TICK="${STEPS_PER_TICK:-4}"
    export INTERVAL_MS="${INTERVAL_MS:-800}"
    ;;
esac

export GUNICORN_THREADS="${GUNICORN_THREADS:-4}"

exec .venv/bin/gunicorn \
  --bind "${HOST}:${PORT}" \
  --workers 1 \
  --threads "${GUNICORN_THREADS}" \
  --worker-class gthread \
  --timeout 120 \
  "regimeflow.visualization.live_dashboard_entry:create_intraday_live_dashboard_server()"
