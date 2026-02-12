# Examples

This folder contains runnable examples intended for local testing and documentation.

Safety rules:
- No real money keys or production broker credentials should ever be committed.
- Live examples are paper-trading only and require environment variables.
- CI should not execute any live broker example.

## Contents

- `backtest_basic/` - deterministic backtest on local CSV data
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
