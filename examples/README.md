# Examples

This folder contains runnable examples intended for local testing and documentation.

Safety rules:
- No real money keys or production broker credentials should ever be committed.
- Live examples are paper-trading only and require environment variables.
- CI should not execute any live broker example.

## Contents

- `backtest_basic/` - deterministic backtest on local CSV data
- `custom_regime_ensemble/` - custom regime detector + strategy (plugin example)
- `python_custom_regime_ensemble/` - Python strategy with custom regime + signal ensemble
- `python_custom_regime_ensemble/config_intraday_multi.yaml` - multi-symbol intraday config
- `docs/reports/multi_intraday_report.md` - latest intraday report artifacts
- `docs/reports/intraday_strategy_tradecheck.md` - intraday pairs/harmonic trade check
- `python_engine_regime/` - Python strategy using engine-provided regime detection
- `data_ingest/` - CSV ingestion and normalization pipeline
- `live_paper_alpaca/` - paper trading example (env-gated)
- `live_paper_ib/` - paper trading example (env-gated)

## Quick Start

```bash
# build (adjust to your build workflow)
cmake -S . -B build
cmake --build build

# run an example
./build/bin/regimeflow_backtest --config examples/backtest_basic/config.yaml
```
