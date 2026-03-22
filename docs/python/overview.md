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
- `execution_model`, `execution_params`.
- `slippage_model`, `slippage_params`.
- `commission_model`, `commission_params`.
- `risk_params`.
- `strategy_params`.

`BacktestConfig.from_yaml(path)` loads the YAML file into this structure.

`execution_params` can carry the full execution block used by the C++ engine, which lets Python backtests opt into richer execution behavior without relying only on the legacy flat slippage/commission fields. This includes simulation controls such as `execution_params["simulation"]["tick_mode"] = "real_ticks"`, `execution_params["simulation"]["synthetic_tick_profile"] = "ohlc_4tick"`, or `execution_params["simulation"]["bar_mode"] = "intrabar_ohlc"`, session controls such as `execution_params["session"]["start_hhmm"] = "09:30"`, `execution_params["session"]["weekdays"] = ["mon", "tue", "wed", "thu", "fri"]`, `execution_params["session"]["closed_dates"] = ["2026-12-25"]`, and `execution_params["session"]["halted_symbols"] = ["AAPL"]`, execution policy controls such as `execution_params["policy"]["price_drift_action"] = "requote"`, queue controls such as `execution_params["queue"]["enabled"] = True`, `execution_params["queue"]["depth_mode"] = "full_depth"`, `execution_params["queue"]["aging_fraction"] = 0.25`, `execution_params["queue"]["replenishment_fraction"] = 0.5`, account controls such as `execution_params["account"]["margin"]["initial_margin_ratio"] = 0.5`, `execution_params["account"]["enforcement"]["stop_out_action"] = "liquidate_worst_first"`, `execution_params["account"]["financing"]["short_borrow_bps_per_day"] = 12.0`, plus transaction-cost variants such as `execution_params["transaction_cost"]["type"] = "maker_taker"`.

`engine.portfolio.equity_curve()` now includes account-state columns in addition to the core exposure columns, including `initial_margin`, `maintenance_margin`, `available_funds`, `margin_excess`, `buying_power`, `margin_call`, and `stop_out`.

`BacktestResults` now also exposes:

- `results.account_curve()` for snapshot-level account history
- `results.account_state()` for the latest recorded account state
- `results.venue_fill_summary()` for per-venue execution diagnostics

The browser-based strategy tester dashboard belongs to the Python visualization layer. The C++ side exposes terminal dashboards, TUI-style tester tools, and JSON snapshots, but not the web UI itself.

`BacktestConfig` also has convenience helpers for the richer execution controls:

- `set_session_window(...)`
- `set_session_halts(...)`
- `set_session_calendar(...)`
- `set_queue_dynamics(...)`
- `set_account_margin(...)`
- `set_account_enforcement(...)`
- `set_account_financing(...)`

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
