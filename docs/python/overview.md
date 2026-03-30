# Python Overview

The `regimeflow` Python package is the research-facing surface of RegimeFlow.
It is the layer you use when you want to:

- run backtests from Python
- write custom Python strategies
- export reports and dataframes
- work with parity checks and research sessions
- generate charts and dashboards
- drive walk-forward optimization from Python

This page is intentionally Python-first. It describes the package as a Python
library.

## Main Use Cases

### 1. Run Backtests In Python

Load a YAML config, run a strategy, and inspect results:

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")
```

### 2. Write A Python Strategy

Subclass `regimeflow.Strategy` and implement whichever callbacks your workflow needs.

```python
import regimeflow as rf

class MyStrategy(rf.Strategy):
    def initialize(self, ctx):
        self.ctx = ctx

    def on_bar(self, bar):
        pass
```

### 3. Analyze Results With Pandas/NumPy

Use built-in helpers instead of re-parsing raw JSON everywhere.

```python
summary = rf.analysis.performance_summary(results)
equity = results.equity_curve()
trades = results.trades()
times, equity_np = rf.analysis.equity_to_numpy(results)
```

### 4. Export Reports And Dashboards

Use the analysis and visualization modules for JSON, CSV, HTML, charts, and dashboards.

```python
html = rf.analysis.report_html(results)
rf.visualization.export_dashboard_html(results, "strategy_tester_report.html")
```

### 5. Run Parity And Research Sessions

Use the research helpers when you want a notebook-friendly wrapper around backtest configs and parity checks.

```python
session = rf.research.ResearchSession(config_path="examples/backtest_basic/config.yaml")
report = session.parity_check(live_config_path="examples/live_paper_alpaca/config.yaml")
```

### 6. Run Walk-Forward Optimization

Use the exported walk-forward types directly from Python when you want rolling-window parameter selection.

## Top-Level Package Surface

The top-level `regimeflow` package exports these primary symbols:

| Category | Symbols |
| --- | --- |
| Core config/helpers | `Config`, `load_config`, `Timestamp` |
| Order model | `Order`, `OrderSide`, `OrderType`, `OrderStatus`, `TimeInForce`, `Fill` |
| Regime state | `RegimeType`, `RegimeState`, `RegimeTransition` |
| Backtest runtime | `BacktestConfig`, `BacktestEngine`, `BacktestResults`, `Portfolio`, `Position` |
| Market data types | `Bar`, `Tick`, `Quote`, `OrderBook`, `BookLevel`, `BarType` |
| Strategy layer | `Strategy`, `StrategyContext`, `register_strategy` |
| Walk-forward | `ParameterDef`, `WalkForwardConfig`, `WalkForwardOptimizer`, `WalkForwardResults`, `WindowResult` |
| Package modules | `analysis`, `config`, `data`, `metrics`, `research`, `visualization` |

## Package Modules

### `regimeflow.analysis`

Performance summaries, dataframe helpers, report export, notebook helpers, and NumPy adapters.

### `regimeflow.data`

CSV loaders, dataframe conversion helpers, timezone normalization, and simple bar-filling utilities.

### `regimeflow.metrics`

Validation-oriented helpers such as independent regime-attribution checks.

### `regimeflow.research`

Parity workflows and notebook-facing session helpers.

### `regimeflow.visualization`

Charts, dashboards, interactive dashboards, and HTML export.

### `regimeflow.config`

Thin Python access to the raw config object and config loader.

## BacktestConfig In Practice

`BacktestConfig` is the center of most Python workflows.

You use it to control:

- data source and source-specific configuration
- symbols and time ranges
- capital and currency
- regime detector and regime parameters
- plugins and plugin search paths
- execution model and execution parameters
- slippage and commission settings
- risk parameters
- strategy parameters

The Python bindings expose richer execution controls as configuration helpers,
including session windows, queue dynamics, account margin, account enforcement,
and financing assumptions.

## Strategy Contract

The Python strategy callbacks are the same callbacks the engine understands conceptually.
Implement only the hooks you need:

- `initialize`
- `on_start`
- `on_stop`
- `on_bar`
- `on_tick`
- `on_quote`
- `on_order_book`
- `on_order_update`
- `on_fill`
- `on_regime_change`
- `on_end_of_day`
- `on_timer`

This lets a Python strategy remain event-driven instead of becoming a loose script
wrapped around a single loop.

## Results Contract

`BacktestResults` gives you several distinct surfaces:

- summary-level metrics
- full serialized reports
- equity/account curves
- trade tables
- regime metrics and regime history
- venue fill summary
- dashboard/tester payloads

Common methods:

- `report_json()`
- `report_csv()`
- `equity_curve()`
- `account_curve()`
- `trades()`
- `account_state()`
- `venue_fill_summary()`
- `regime_metrics()`
- `regime_history()`
- `dashboard_snapshot()`
- `tester_report()`
- `tester_journal()`

## Python-Only Boundary

The browser-based dashboard belongs to the Python layer.

That is an intentional boundary:

- the native engine produces results and snapshot data
- the Python package provides the dashboard and charting surface

For Python users, that means reporting and visualization are first-class workflows,
not separate tooling bolted on after the fact.

## Where To Go Next

- `python/workflow.md`
- `python/cli.md`
- `tutorials/python-usage.md`
- `api/python.md`
