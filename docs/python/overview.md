# Python Overview

The Python package (`python/regimeflow`) exposes the backtest engine, strategies, data sources, and reporting utilities. The module is backed by the compiled `_core` extension built from `python/bindings.cpp`.

## Key Types

- `BacktestConfig` for configuration and YAML loading.
- `BacktestEngine` for running a strategy over historical data.
- `Strategy` base class for Python strategies.
- `analysis` helpers for performance summaries.

## BacktestConfig Fields

These fields are defined in `python/bindings.cpp`:

- `data_source` string, default `csv`.
- `data_config` dict.
- `symbols` list of strings.
- `start_date`, `end_date` as `YYYY-MM-DD`.
- `bar_type` string, default `1d`.
- `initial_capital`, `currency`.
- `regime_detector`, `regime_params`.
- `plugins_search_paths`, `plugins_load`.
- `slippage_model`, `slippage_params`.
- `commission_model`, `commission_params`.
- `risk_params`.
- `strategy_params`.

`BacktestConfig.from_yaml(path)` loads the YAML file into this structure.

## Python Strategy Contract

A Python strategy must implement the same lifecycle methods as the C++ strategy interface. The core ones are:

- `initialize(ctx)`
- `on_bar(bar)`
- `on_tick(tick)`
- `on_order_book(book)`
- `on_order_update(order)`
- `on_fill(fill)`

## Example

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("quickstart.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")

print(results.report_json())
```

## Research Session And Parity

`regimeflow.research` adds a notebook-friendly workflow and parity checks:

```python
import regimeflow as rf

session = rf.research.ResearchSession(config_path="quickstart.yaml")
report = session.parity_check(live_config_path="examples/live_paper_alpaca/config.yaml")
print(report.status, report.warnings)
```

## Next Steps

- `python/cli.md`
- `python/workflow.md`
- `guide/backtesting.md`
