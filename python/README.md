# RegimeFlow Python Bindings

`regimeflow` is a Python package for regime-aware quantitative research,
backtesting, reporting, visualization, and walk-forward analysis.

It is meant for Python users who want to:

- run historical backtests from Python
- write event-driven Python strategies
- turn results into dataframes, NumPy arrays, JSON, CSV, and HTML
- inspect regime-aware metrics instead of only a single total-return number
- run parity and research-session workflows
- generate strategy-tester dashboards from Python

This README is intentionally Python-only. It documents the published Python
package as a Python library and focuses on what users can do after
`pip install regimeflow`.

## What The Package Contains

The package groups its functionality into a few clear surfaces:

- Native engine bindings for backtests, orders, fills, portfolio state, market data,
  regime state, and walk-forward optimization.
- Python strategy support through `regimeflow.Strategy`.
- Analysis helpers for performance summaries, regime metrics, report export, HTML/CSV/JSON output,
  notebook helpers, and NumPy-friendly accessors.
- Data helpers for CSV ingestion, dataframe conversion, and simple preprocessing.
- Research helpers for parity workflows and notebook-facing research sessions.
- Visualization helpers for charts, dashboards, live dashboards, and HTML export.

## Installation

Basic installation:

```bash
pip install regimeflow
```

Visualization extras:

```bash
pip install "regimeflow[viz]"
```

Development extras:

```bash
pip install "regimeflow[dev]"
```

Everything exposed by the published Python package:

```bash
pip install "regimeflow[full]"
```

## Runtime Notes

- Python 3.9 through 3.12 are supported.
- Wheels are preferred when available.
- The package exposes a compiled extension behind a Python API, so import and usage
  remain normal Python.

## What You Can Do With The Package

The package is broad enough that it helps to think in workflows instead of only
in class names.

### Use Case 1: Run A Backtest From Python

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")

print(rf.analysis.performance_summary(results))
```

Use this when you already have a config file and want a direct research run.

### Use Case 2: Write A Python Strategy

```python
import regimeflow as rf

class ThresholdStrategy(rf.Strategy):
    def initialize(self, ctx):
        self.ctx = ctx

    def on_bar(self, bar):
        if bar.close > bar.open:
            self.ctx.submit_order(
                rf.Order("AAPL", rf.OrderSide.BUY, rf.OrderType.MARKET, 1.0)
            )

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
results = rf.BacktestEngine(cfg).run(ThresholdStrategy())
```

Use this when the strategy logic itself belongs in Python.

### Use Case 3: Export Reports

```python
rf.analysis.write_report_json(results, "report.json")
rf.analysis.write_report_csv(results, "report.csv")
rf.analysis.write_report_html(results, "report.html")
```

Use this when you want machine-readable and human-readable outputs from the same run.

### Use Case 4: Work With Pandas And NumPy

```python
tables = rf.data.results_to_dataframe(results)
equity_times, equity_values = rf.analysis.equity_to_numpy(results)
```

Use this when your workflow continues into notebooks, analytics pipelines, or custom reports.

### Use Case 5: Validate Regime Attribution

```python
ok, message = rf.metrics.validate_regime_attribution(results)
print(ok, message)
```

Use this when you need an independent check that regime metrics reconcile correctly.

### Use Case 6: Generate A Dashboard

```python
rf.visualization.export_dashboard_html(results, "strategy_tester_report.html")
```

Use this when you need a shareable HTML report from a Python run.

### Use Case 7: Run A Research Session

```python
session = rf.research.ResearchSession(
    config_path="examples/backtest_basic/config.yaml"
)
results = session.run_backtest("moving_average_cross")
parity = session.parity_check(
    live_config_path="examples/live_paper_alpaca/config.yaml"
)
```

Use this when a notebook or research tool wants one object to own config, runs,
and parity checks.

### Use Case 8: Run Walk-Forward Analysis

Use the exported walk-forward types when you need parameter search and rolling
out-of-sample validation:

- `ParameterDef`
- `WalkForwardConfig`
- `WalkForwardOptimizer`
- `WalkForwardResults`
- `WindowResult`

## Quick Start

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")

print(results.report_json())
```

The shortest mental model is:

1. load a `BacktestConfig`
2. construct `BacktestEngine`
3. run a built-in strategy name or a Python `Strategy`
4. inspect `BacktestResults`

## Python Package Layout

The package has one top-level surface and several helper modules.

### Top-Level Runtime Objects

| Category | Symbols |
| --- | --- |
| Config and timestamps | `Config`, `load_config`, `Timestamp` |
| Orders and fills | `Order`, `Fill`, `OrderSide`, `OrderType`, `OrderStatus`, `TimeInForce` |
| Regime state | `RegimeType`, `RegimeState`, `RegimeTransition` |
| Engine runtime | `BacktestConfig`, `BacktestEngine`, `BacktestResults`, `Portfolio`, `Position` |
| Market data | `Bar`, `Tick`, `Quote`, `OrderBook`, `BookLevel`, `BarType` |
| Strategy surface | `Strategy`, `StrategyContext`, `register_strategy` |
| Walk-forward | `ParameterDef`, `WalkForwardConfig`, `WalkForwardOptimizer`, `WalkForwardResults`, `WindowResult` |

### Top-Level Module Exports

These are available directly from the package:

- `regimeflow.analysis`
- `regimeflow.config`
- `regimeflow.data`
- `regimeflow.metrics`
- `regimeflow.research`
- `regimeflow.visualization`

Compatibility aliases are also exported:

- `regimeflow.walkforward`
- `regimeflow.core_strategy`
- `regimeflow.strategy_module`

## Top-Level Imports

The top-level package exports the core types most research workflows need:

- `BacktestConfig`
- `BacktestEngine`
- `BacktestResults`
- `Portfolio`
- `Position`
- `Order`
- `Fill`
- `OrderSide`
- `OrderType`
- `OrderStatus`
- `TimeInForce`
- `Timestamp`
- `RegimeType`
- `RegimeState`
- `RegimeTransition`
- `Strategy`
- `StrategyContext`
- `Bar`
- `Tick`
- `Quote`
- `OrderBook`
- `BookLevel`
- `BarType`
- `WalkForwardConfig`
- `WalkForwardOptimizer`
- `WalkForwardResults`
- `WindowResult`
- `ParameterDef`

Helper modules and aliases also exposed from the package root:

- `regimeflow.analysis`
- `regimeflow.config`
- `regimeflow.data`
- `regimeflow.metrics`
- `regimeflow.research`
- `regimeflow.visualization`
- `regimeflow.walkforward`
- `regimeflow.core_strategy`
- `regimeflow.strategy_module`

## Backtest Configuration

`BacktestConfig` is the main configuration object. It supports YAML loading and
lets Python workflows opt into the same execution and risk structure used by the
native engine.

Typical fields include:

- data-source selection and data-source-specific settings
- symbol list, date bounds, and bar type
- capital and currency
- regime detector selection and detector parameters
- strategy parameters
- execution model and execution parameters
- slippage and commission settings
- plugin search paths and plugin load lists
- risk parameters

YAML loading:

```python
cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
```

The Python bindings also expose convenience helpers for richer execution realism
configuration, including:

- session windows and halted dates/symbols
- queue-dynamics settings
- account-margin configuration
- enforcement rules for margin calls and stop-out behavior
- financing parameters

This matters because the Python surface is not limited to a flat
`commission + slippage` model. It can drive richer session, queue, account,
margin, and financing controls directly from Python.

In practice, `BacktestConfig` is where you set:

- symbols
- date range
- bar type
- capital
- data source and source parameters
- regime detector and regime parameters
- strategy parameters
- execution and account assumptions
- slippage and transaction-cost assumptions
- plugin/search-path settings

## Strategy Contract

Custom Python strategies subclass `regimeflow.Strategy`.

The core lifecycle methods are:

- `initialize(ctx)`
- `on_start()`
- `on_stop()`
- `on_bar(bar)`
- `on_tick(tick)`
- `on_quote(quote)`
- `on_order_book(book)`
- `on_order_update(order)`
- `on_fill(fill)`
- `on_regime_change(transition)`
- `on_end_of_day()`
- `on_timer(timer_id)`

Minimal example:

```python
import regimeflow as rf

class MyStrategy(rf.Strategy):
    def initialize(self, ctx):
        self.ctx = ctx

    def on_bar(self, bar):
        pass

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run(MyStrategy())
```

The engine also accepts a registered or built-in strategy name:

```python
results = engine.run("moving_average_cross")
```

`StrategyContext` gives Python strategies the operational methods they usually need:

- `submit_order`
- `cancel_order`
- `portfolio`
- `get_position`
- `current_regime`
- `current_time`
- `get_latest_bar`
- `get_latest_quote`
- `get_latest_book`
- `get_bars`
- `schedule_timer`
- `cancel_timer`

## Results Surface

`BacktestResults` is the main output object. Common downstream workflows include:

- summary reporting
- equity-curve export
- trade export
- regime attribution inspection
- account-state analysis
- dashboard generation

Representative usage:

```python
report_json = results.report_json()
report_csv = results.report_csv()
equity = results.equity_curve()
trades = results.trades()
```

Additional result surfaces exposed to Python:

- `results.account_curve()`
- `results.account_state()`
- `results.venue_fill_summary()`
- `results.performance_summary()`
- `results.performance_stats()`
- `results.regime_performance()`
- `results.transition_metrics()`
- `results.regime_metrics()`
- `results.regime_history()`
- `results.dashboard_snapshot()`
- `results.dashboard_snapshot_json()`
- `results.dashboard_terminal()`
- `results.tester_report()`
- `results.tester_journal()`

The portfolio equity history now includes account-state columns such as:

- `initial_margin`
- `maintenance_margin`
- `available_funds`
- `margin_excess`
- `buying_power`
- `margin_call`
- `stop_out`

That matters for users who want margin-aware or enforcement-aware backtests from Python,
not only mark-to-market equity.

## Analysis Module

`regimeflow.analysis` provides report and metric helpers for turning native engine
results into research outputs.

Available helpers include:

- `performance_summary`
- `performance_stats`
- `regime_performance`
- `transition_metrics`
- `equity_curve`
- `trades`
- `summary_dataframe`
- `stats_dataframe`
- `regime_dataframe`
- `transitions_dataframe`
- `report_json`
- `report_csv`
- `report_html`
- `write_report_json`
- `write_report_csv`
- `write_report_html`
- `display_report`
- `display_equity`
- `equity_to_numpy`
- `trades_to_numpy`

Example:

```python
import regimeflow as rf

summary = rf.analysis.performance_summary(results)
html = rf.analysis.report_html(results)
```

Use `analysis` when you want:

- summary statistics
- regime-aware performance slices
- ready-to-export reports
- notebook display helpers
- NumPy-friendly access to equity/trade data

## Data Module

`regimeflow.data` provides dataframe conversion and loader helpers for Python-side
research preprocessing.

Exposed helpers include:

- `bars_to_dataframe`
- `dataframe_to_bars`
- `ticks_to_dataframe`
- `dataframe_to_ticks`
- `results_to_dataframe`
- `DataFrameDataSource`
- `load_csv_bars`
- `load_csv_ticks`
- `load_csv_dataframe`
- `normalize_timezone`
- `fill_missing_time_bars`

This is useful when you want to keep feature engineering and exploratory work in
Pandas while still feeding the native engine predictable structures.

Typical patterns:

- CSV to dataframe for preprocessing
- dataframe to bars/ticks for engine input
- results to dataframe tables for downstream analysis
- timezone normalization before runs
- filling missing bars for more uniform time-series processing

## Research Utilities

`regimeflow.research` exposes notebook-oriented helpers, especially for parity
workflows where you want to compare a backtest/research baseline against another
runtime configuration.

Available types:

- `ResearchSession`
- `ParityResult`
- `parity_check`

Example:

```python
import regimeflow as rf

session = rf.research.ResearchSession(config_path="examples/backtest_basic/config.yaml")
report = session.parity_check(live_config_path="examples/live_paper_alpaca/config.yaml")
print(report.status)
```

Use this module when:

- you want a notebook-friendly session object
- you want parity checks near the research loop
- you want one object to own config path and run orchestration

## Visualization

The browser-based strategy tester dashboard belongs to the Python package.

`regimeflow.visualization` exports:

- `plot_results`
- `create_dashboard`
- `create_strategy_tester_dashboard`
- `create_live_dashboard`
- `dashboard_snapshot_to_live_dashboard`
- `create_interactive_dashboard`
- `create_dash_app`
- `launch_dashboard`
- `create_live_dash_app`
- `launch_live_dashboard`
- `export_dashboard_html`

This gives Python users a direct route from engine output to a sharable dashboard or
interactive session.

Typical use cases:

- static chart generation
- shareable HTML strategy-tester reports
- interactive dashboard sessions
- conversion from dashboard snapshots to a live-style dashboard view

## Command-Line Entry Point

The package installs:

```bash
regimeflow-backtest
```

It supports:

- YAML config loading
- strategy selection
- JSON strategy-parameter injection
- report export
- equity/trade CSV export
- optional printed summary output

Typical usage:

```bash
regimeflow-backtest \
  --config examples/backtest_basic/config.yaml \
  --strategy moving_average_cross \
  --output-json report.json \
  --output-equity equity.csv \
  --output-trades trades.csv \
  --print-summary
```

For Python strategies, the CLI accepts the `module:Class` form and expects the class
to inherit from `regimeflow.Strategy`.

This is useful when your team wants:

- one reproducible shell command per run
- YAML-driven backtests without writing a wrapper script
- report export in CI or scheduled jobs

## Walk-Forward Optimization

The top-level package also exports walk-forward optimization types:

- `ParameterDef`
- `WalkForwardConfig`
- `WalkForwardOptimizer`
- `WalkForwardResults`
- `WindowResult`

These are intended for parameterized research loops where you need rolling-window
evaluation rather than a single static in-sample run.

Use this surface when:

- one in-sample backtest is not enough
- you want repeated train/test windows
- you need parameter selection logic exposed in Python

## What This Package Is Good At

- Python-first research workflows backed by a native engine
- repeatable backtests
- regime-aware strategy experiments
- analysis/report export
- dashboard generation
- parity-oriented research tooling

## What This Package Does Not Claim

- It does not turn PyPI installation alone into a fully configured live-trading stack.
- Broker access, venue permissions, account balances, and regional restrictions remain external constraints.
- The Python web dashboard is a visualization surface, not proof of production readiness.

## Documentation And Examples

Project documentation:

- Docs site: <https://gregorian-09.github.io/regime-flow/>
- Python overview: <https://gregorian-09.github.io/regime-flow/python/overview/>
- Python workflow: <https://gregorian-09.github.io/regime-flow/python/workflow/>
- Python tutorial: <https://gregorian-09.github.io/regime-flow/tutorials/python-usage/>
- Python bindings coverage: <https://gregorian-09.github.io/regime-flow/python/bindings-coverage/>
- API design: <https://gregorian-09.github.io/regime-flow/api/python/>

Repository:

- Source: <https://github.com/gregorian-09/regime-flow>

## Maintainer

- Gregorian Rayne
- gregorianrayne09@gmail.com
