# Backtesting

This guide covers how backtests are constructed, how the engine consumes data, and how results are produced.

## Backtest Pipeline

1. **Load config** and build data source, execution, risk, and regime modules.
2. **Create iterators** over bars, ticks, and order books.
3. **Load data** into the engine.
4. **Initialize strategy** with `StrategyContext`.
5. **Run** the event loop and produce fills and portfolio updates.
6. **Emit results** including equity curve and performance metrics.

## Backtest Engine Lifecycle

- `engine::EngineFactory::create` builds a `BacktestEngine` from a config object.
- `BacktestEngine::configure_execution` wires slippage, commission, market impact, and latency.
- `BacktestEngine::configure_risk` sets limits and stop-loss rules.
- `BacktestEngine::configure_regime` constructs the detector from config.
- `BacktestRunner::run` binds a strategy, loads data iterators, and executes the loop.

## Configuration Modes

RegimeFlow supports two config formats:

1. **C++ Engine Config**: nested `engine`, `data`, `strategy`, `risk`, `execution`, `regime`, `plugins`.
2. **Python BacktestConfig**: flat keys used by the Python bindings and CLI.

For both, see `reference/configuration.md`.

## Selecting The Strategy

- Built-in strategies are registered in `strategy::StrategyFactory`.
- Plugins can provide additional strategies.
- Python backtests can use a `module:Class` strategy via `regimeflow.cli`.

Example (built-in):

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy moving_average_cross
```

Example (custom Python):

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy my_strategies:MyStrategy
```

## Backtest Results

`BacktestResults` exposes:

- Equity curve and portfolio snapshots.
- Trades and fills.
- Performance metrics and report exports.

In Python, you can export these directly:

```python
results = engine.run("moving_average_cross")
results.report_json()
results.report_csv()
results.equity_curve()
results.trades()
```

## Parallel Backtests

`BacktestEngine::run_parallel` can run multiple parameter sets. Use this for grid searches or random sweeps, and combine with walk-forward optimization for robust selection.

See `guide/walkforward.md` for the optimizer flow.
