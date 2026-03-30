# Python Workflow

This page describes the typical Python workflow for `regimeflow` users from first
import to analysis and export.

## Workflow 1: Standard Research Backtest

### Step 1: Import The Package

```python
import regimeflow as rf
```

### Step 2: Load A Backtest Config

```python
cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
```

### Step 3: Run A Strategy

Built-in or registered strategy:

```python
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")
```

Custom Python strategy:

```python
class MyStrategy(rf.Strategy):
    def initialize(self, ctx):
        self.ctx = ctx

    def on_bar(self, bar):
        pass

engine = rf.BacktestEngine(cfg)
results = engine.run(MyStrategy())
```

### Step 4: Inspect Results

```python
summary = rf.analysis.performance_summary(results)
stats = rf.analysis.performance_stats(results)
equity = results.equity_curve()
trades = results.trades()
```

### Step 5: Export Reports

```python
rf.analysis.write_report_json(results, "report.json")
rf.analysis.write_report_csv(results, "report.csv")
rf.analysis.write_report_html(results, "report.html")
```

## Workflow 2: Python-Driven Strategy Development

This is the common loop when the strategy itself lives in Python.

1. subclass `rf.Strategy`
2. use `initialize()` to keep state/context references
3. act on `on_bar`, `on_tick`, or `on_order_book`
4. use `StrategyContext` to submit/cancel orders and query state
5. run, inspect, and iterate

Useful `StrategyContext` methods:

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

## Workflow 3: Dataframe-Centric Research

If your workflow starts in Pandas, use `regimeflow.data` to bridge Python dataframes and native types.

Typical helpers:

- `bars_to_dataframe`
- `dataframe_to_bars`
- `ticks_to_dataframe`
- `dataframe_to_ticks`
- `load_csv_bars`
- `load_csv_ticks`
- `load_csv_dataframe`
- `normalize_timezone`
- `fill_missing_time_bars`

Example:

```python
from regimeflow.data import load_csv_dataframe, normalize_timezone

df = load_csv_dataframe("bars.csv")
df = normalize_timezone(df)
```

## Workflow 4: Report And Dashboard Production

The Python package is also the reporting layer.

Use:

- `rf.analysis` when you need structured metrics and report export
- `rf.visualization` when you need charts, dashboards, and HTML deliverables

Examples:

```python
dashboard = rf.visualization.create_strategy_tester_dashboard(results)
html_path = rf.visualization.export_dashboard_html(results, "report.html")
```

## Workflow 5: Research Session And Parity

Use `rf.research.ResearchSession` when you want a higher-level Python object
that owns a backtest config and can run parity checks.

```python
session = rf.research.ResearchSession(config_path="examples/backtest_basic/config.yaml")
results = session.run_backtest("moving_average_cross")
parity = session.parity_check(live_config_path="examples/live_paper_alpaca/config.yaml")
```

This workflow is useful in notebooks and research tooling where you want
a single object for config, runs, and parity checks.

## Workflow 6: Walk-Forward Optimization

Use the walk-forward exports when parameter stability matters more than a single
in-sample run.

Relevant types:

- `ParameterDef`
- `WalkForwardConfig`
- `WalkForwardOptimizer`
- `WalkForwardResults`
- `WindowResult`

## Practical Python Guidance

- use `BacktestConfig.from_yaml(...)` when your workflow already has a checked-in config
- use `BacktestConfig.from_dict(...)` when your workflow is generated programmatically
- use `rf.analysis` helpers instead of hand-parsing `report_json()` everywhere
- use `rf.data` helpers when Pandas is your entry point
- use `rf.visualization` only when you need dashboards or plots; keep core research loops data-oriented
- use `rf.research` when you need parity-oriented or notebook-oriented session management

## Related Pages

- `python/overview.md`
- `python/cli.md`
- `tutorials/python-usage.md`
- `api/python.md`
