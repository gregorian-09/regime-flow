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

exec .venv/bin/python - <<'PY'
import os

from regimeflow.visualization.dashboard_app import launch_live_dashboard
from regimeflow.visualization.live_dashboard_entry import build_intraday_live_dashboard_runtime

snapshot_provider, advance_step = build_intraday_live_dashboard_runtime()

launch_live_dashboard(
    snapshot_provider,
    host=os.environ.get("HOST", "127.0.0.1"),
    port=int(os.environ.get("PORT", "8050")),
    debug=False,
    interval_ms=int(os.environ.get("INTERVAL_MS", "800")),
    advance_step=advance_step,
)
PY
