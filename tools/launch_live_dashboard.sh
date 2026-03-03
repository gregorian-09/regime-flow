#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export PYTHONPATH="build/lib:build/python:python${PYTHONPATH:+:${PYTHONPATH}}"
export HOST="${HOST:-127.0.0.1}"
export PORT="${PORT:-8050}"

exec .venv/bin/python -c 'import os; import regimeflow as rf; from regimeflow.visualization import launch_dashboard; from examples.python_engine_regime.run_python_engine_regime import EngineRegimeStrategy; cfg = rf.BacktestConfig.from_yaml("examples/python_engine_regime/config.yaml"); results = rf.BacktestEngine(cfg).run(EngineRegimeStrategy(symbol=cfg.symbols[0])); launch_dashboard(results, host=os.environ.get("HOST", "127.0.0.1"), port=int(os.environ.get("PORT", "8050")), debug=False)'
