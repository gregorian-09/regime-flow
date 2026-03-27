# Examples

This folder contains runnable examples intended for local testing and documentation.

Safety rules:
- No real money keys or production broker credentials should ever be committed.
- Live examples are paper-trading only and require environment variables.
- CI should not execute any live broker example.

## Contents

- `backtest_basic/` - deterministic backtest on local CSV data
- `backtest_controls_demo/` - compiled C++ demo for holidays and runtime halt/resume
- `strategy_tester_cli/` - compiled C++ terminal strategy tester dashboard
- `custom_regime_ensemble/` - custom regime detector + strategy (plugin example)
- `python_custom_regime_ensemble/` - Python strategy with custom regime + signal ensemble
- `python_custom_regime_ensemble/config_intraday_multi.yaml` - multi-symbol intraday config
- `python_execution_realism/` - Python strategy covering advanced backtest execution controls
- `python_transformer_regime/` - transformer-style Python regime model + CSV export
- `transformer_regime_ensemble/` - C++ plugin using transformer CSV signals
- `docs/reports/multi_intraday_report.md` - latest intraday report artifacts
- `docs/reports/intraday_strategy_tradecheck.md` - intraday pairs/harmonic trade check
- `python_engine_regime/` - Python strategy using engine-provided regime detection
- `data_ingest/` - CSV ingestion and normalization pipeline
- `live_paper_alpaca/` - paper trading example (env-gated)
- `live_paper_binance/` - paper or demo trading example (env-gated)
- `live_paper_ib/` - paper trading example (env-gated)

## Quick Start

```bash
# build (adjust to your build workflow)
cmake -S . -B build
cmake --build build

# run an example
./build/bin/regimeflow_backtest --config examples/backtest_basic/config.yaml

# run the C++ control demo
./build/bin/regimeflow_backtest_controls_demo

# run the C++ strategy tester terminal dashboard
./build/bin/regimeflow_strategy_tester
./build/bin/regimeflow_strategy_tester --tui
./build/bin/regimeflow_strategy_tester --snapshot-file=/tmp/strategy_tester_snapshot.json

# run the full execution-realism example
PYTHONPATH=build/lib:build/python:python .venv/bin/python \
  examples/python_execution_realism/run_python_execution_realism.py \
  --config examples/python_execution_realism/config.yaml
```
