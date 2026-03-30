# Examples

This section documents runnable examples intended for local testing and validation. They are designed to be deterministic where possible and safe by default.

## Safety Rules

- No real money keys or production broker credentials should ever be committed.
- Live examples are paper-trading only and require environment variables.
- CI must not execute any live broker example.

## Example Index

- **Backtest Basic**: deterministic backtest on local CSV data.
- **Backtest Controls Demo**: compiled C++ demo for market closures and runtime halt/resume.
- **Strategy Tester CLI**: compiled C++ terminal strategy tester with live dashboard refresh and JSON export.
- **Custom Regime Ensemble**: custom regime detector + strategy (plugin example).
- **Python Custom Regime Ensemble**: Python strategy with custom regime logic and signal ensemble.
- **Python Execution Realism**: Python strategy covering synthetic ticks, queueing, routing, venue overrides, and account enforcement.
- **Python Engine Regime**: Python strategy that uses `ctx.current_regime()` from engine detectors.
- **Python Transformer Regime**: Python transformer-style model for regimes + CSV export.
- **Transformer Regime Plugin**: C++ plugin reading transformer regime CSV output.
- **Data Ingest**: CSV normalization and validation pipeline.
- **Live Paper Alpaca**: paper-trading example for Alpaca (env-gated).
- **Live Paper Binance**: paper or demo trading example for Binance Spot (env-gated).
- **Live Paper IB**: paper-trading example for Interactive Brokers (env-gated, broker access may still be blocked by IB policy or region).

## Backtest Basic

Path: `examples/backtest_basic/`

```bash
./build/bin/regimeflow_backtest --config examples/backtest_basic/config.yaml
```

## Backtest Controls Demo

Path: `examples/backtest_controls_demo/`

```bash
./build/bin/regimeflow_backtest_controls_demo
```

This compiled C++ example demonstrates:

- a configured holiday via `execution.session.closed_dates`
- a runtime symbol halt via `TradingHalt`
- a runtime resume via `TradingResume`

It prints the order status after each stage so users can see that the order remains live but non-executable until the control state is cleared.

## Strategy Tester CLI

Path: `examples/strategy_tester_cli/`

```bash
./build/bin/regimeflow_strategy_tester
./build/bin/regimeflow_strategy_tester --tui
./build/bin/regimeflow_strategy_tester --no-live --no-ansi --tab=venues
./build/bin/regimeflow_strategy_tester --json
./build/bin/regimeflow_strategy_tester --snapshot-file=/tmp/strategy_tester_snapshot.json
```

This compiled C++ tool demonstrates the native terminal strategy tester surface powered by:

- `BacktestEngine::dashboard_snapshot()`
- `BacktestEngine::dashboard_snapshot_json()`
- `BacktestEngine::dashboard_terminal()`

It also supports:

- tab-specific terminal rendering via `--tab=...`
- real-time split-pane TUI mode via `--interactive-tabs` or `--tui`
- non-blocking key controls: `0-8`, `n`, `p`, `q`
- continuous snapshot file export via `--snapshot-file=...`

It uses injected quote events and a small built-in strategy, so it runs without external data files.

## Custom Regime Ensemble

Path: `examples/custom_regime_ensemble/`

This example builds a custom regime detector and a regime‑aware strategy via plugins.

```bash
cmake -S examples/plugins/custom_regime -B examples/plugins/custom_regime/build \
  -DREGIMEFLOW_ROOT=$(pwd) -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/plugins/custom_regime/build

cmake -S examples/custom_regime_ensemble -B examples/custom_regime_ensemble/build \
  -DREGIMEFLOW_ROOT=$(pwd) -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/custom_regime_ensemble/build

./examples/custom_regime_ensemble/build/run_custom_regime_backtest \
  --config examples/custom_regime_ensemble/config.yaml
```

## Python Custom Regime Ensemble

Path: `examples/python_custom_regime_ensemble/`

```bash
.venv/bin/python tools/download_intraday_spx_sample.py
.venv/bin/python tools/download_intraday_spx_sample.py --multi

PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_custom_regime_ensemble/run_python_custom_regime_ensemble.py \
  --config examples/python_custom_regime_ensemble/config.yaml

PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_custom_regime_ensemble/run_python_custom_regime_ensemble.py \
  --config examples/python_custom_regime_ensemble/config_intraday_multi.yaml
```

Latest intraday report:
- `docs/reports/multi_intraday_report.md`
- `docs/reports/intraday_strategy_tradecheck.md`
- `docs/reports/transformer_regime_report.md`

## Python Execution Realism

Path: `examples/python_execution_realism/`

```bash
PYTHONPATH=build/lib:build/python:python .venv/bin/python \
  examples/python_execution_realism/run_python_execution_realism.py \
  --config examples/python_execution_realism/config.yaml
```

This example is the direct reference for the advanced backtesting controls:

- `execution.simulation.*`
- `execution.policy.*`
- `execution.queue.*`
- `execution.routing.*`
- `execution.account.*`

It also prints `results.account_state()` and `results.venue_fill_summary()` so you can inspect the new diagnostics after the run.

## Python Engine Regime

Path: `examples/python_engine_regime/`

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_engine_regime/run_python_engine_regime.py \
  --config examples/python_engine_regime/config.yaml
```

## Python Transformer Regime

Path: `examples/python_transformer_regime/`

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_transformer_regime/run_python_transformer_strategy.py \
  --config examples/python_transformer_regime/config.yaml
```

Generate a CSV for the C++ plugin:

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_transformer_regime/generate_regime_csv.py \
  --config examples/python_transformer_regime/config.yaml \
  --output examples/python_transformer_regime/regime_signals.csv
```

## Transformer Regime Plugin

Path: `examples/transformer_regime_ensemble/`

```bash
cmake -S examples/plugins/transformer_regime -B examples/plugins/transformer_regime/build \
  -DREGIMEFLOW_ROOT=$(pwd) -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/plugins/transformer_regime/build

./examples/custom_regime_ensemble/build/run_custom_regime_backtest \
  --config examples/transformer_regime_ensemble/config.yaml
```

TorchScript backtest (requires libtorch):

```bash
./examples/custom_regime_ensemble/build/run_custom_regime_backtest \
  --config examples/transformer_regime_ensemble/config_torchscript.yaml
```

## Data Ingest

Path: `examples/data_ingest/`

```bash
./build/bin/regimeflow_ingest --config examples/data_ingest/config.yaml
```

## Live Paper Alpaca

Path: `examples/live_paper_alpaca/`

Env vars:
- `ALPACA_API_KEY`
- `ALPACA_API_SECRET`
- `ALPACA_PAPER_BASE_URL`
- `ALPACA_STREAM_URL`

```bash
export ALPACA_API_KEY=...
export ALPACA_API_SECRET=...
export ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
export ALPACA_STREAM_URL=wss://stream.data.alpaca.markets/v2/iex
./build/bin/regimeflow_live --config examples/live_paper_alpaca/config.yaml
```

You can also store these values in a `.env` file in the project root; the live CLI
will load it automatically if present.

When the live CLI runs, it prints connection status and heartbeat health to stdout.

Lifecycle validation:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_alpaca/config.yaml \
  --mode submit_cancel_reconcile --quantity 1
```

## Live Paper Binance

Path: `examples/live_paper_binance/`

Env vars:
- `BINANCE_API_KEY`
- `BINANCE_SECRET_KEY`
- `BINANCE_BASE_URL`
- `BINANCE_STREAM_URL`

```bash
export BINANCE_API_KEY=...
export BINANCE_SECRET_KEY=...
export BINANCE_BASE_URL=https://demo-api.binance.com
export BINANCE_STREAM_URL=wss://demo-stream.binance.com/ws
./build/bin/regimeflow_live --config examples/live_paper_binance/config.yaml
```

Current official Binance Spot endpoints:

- Live REST: `https://api.binance.com`
- Live stream: `wss://stream.binance.com:9443/ws`
- Demo Mode REST: `https://demo-api.binance.com`
- Demo Mode stream: `wss://demo-stream.binance.com/ws`
- Spot Testnet stream: `wss://stream.testnet.binance.vision/ws`

The example stays environment-driven so users can match the current venue they operate.

Lifecycle validation:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_binance/config.yaml \
  --mode submit_cancel_reconcile --quantity 0.001
```

## Alpaca Data REST (Python)

```python
import regimeflow as rf

client = rf.data.AlpacaDataClient({
    "api_key": "...",
    "secret_key": "...",
    "trading_base_url": "https://paper-api.alpaca.markets",
    "data_base_url": "https://data.alpaca.markets",
})

assets = client.list_assets()
bars = client.get_bars(["AAPL", "MSFT"], "1Day", "2024-01-01", "2024-06-30")
trades = client.get_trades(["AAPL"], "2024-01-01", "2024-01-02")
snapshot = client.get_snapshot("AAPL")
```

## Alpaca Data REST (C++)

Fetch assets, bars, trades, and snapshots using the built-in helper (bars/trades paginate and merge all pages):

```bash
export ALPACA_API_KEY=...
export ALPACA_API_SECRET=...
./build/bin/regimeflow_alpaca_fetch --symbols=AAPL,MSFT --start=2024-01-01 --end=2024-01-05 --timeframe=1Day
```

## Alpaca Data Source (Backtest)

Use the Alpaca REST bars as a `data_source`:

```yaml
data_source: alpaca
data:
  api_key: ${ALPACA_API_KEY}
  secret_key: ${ALPACA_API_SECRET}
  trading_base_url: https://paper-api.alpaca.markets
  data_base_url: https://data.alpaca.markets
  timeout_seconds: 10
  symbols: AAPL,MSFT
```

## Live Paper IB

Path: `examples/live_paper_ib/`

Env vars:
- `IB_HOST`
- `IB_PORT`
- `IB_CLIENT_ID`

The example config demonstrates mixed IB contract coverage:

- `AAPL` through smart-routed US equity defaults
- `EURUSD` through `CASH` and `IDEALPRO`
- `FGBL` through a futures `local_symbol`
- `BMW_C72_DEC25` through an option contract override

```bash
export IB_HOST=127.0.0.1
export IB_PORT=7497
export IB_CLIENT_ID=1
./build/bin/regimeflow_live --config examples/live_paper_ib/config.yaml
```

Lifecycle validation:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_ib/config.yaml \
  --mode submit_cancel_reconcile --quantity 1
```
