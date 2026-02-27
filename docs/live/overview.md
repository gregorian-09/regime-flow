# Live Trading Overview

The live engine reuses the same strategy contract and event pipeline as backtests. It connects to a broker adapter, streams market data, and enforces risk limits in real time.

## Entry Point

The CLI is implemented in `src/tools/live_main.cpp`:

```bash
./build/bin/regimeflow_live --config <path>
```

The live CLI loads `.env` automatically if present and merges environment variables with `live.broker_config`.

## What The Live Engine Does

- Connects to the broker adapter.
- Subscribes to market data.
- Runs strategy callbacks on incoming events.
- Applies risk limits and order throttling.
- Emits audit logs and system health events.
- Optionally tracks live performance drift vs a backtest baseline.

## Live Performance Drift Tracking

Enable live drift tracking by setting `metrics.live.enable: true`. When enabled, the engine writes:

- `live_drift.csv` for time-series snapshots.
- `live_performance.json` for a summary.

You can also point the tracker at a backtest report JSON via `metrics.live.baseline_report`.

## Next Steps

- `live/config.md`
- `live/brokers.md`
- `live/resilience.md`
