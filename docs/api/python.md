# Package: Python Interfaces

This page is the Python API reference for the `regimeflow` package. It focuses on
the Python-visible bindings, helper modules, and Python-specific workflows.

## Top-Level Exports

### Configuration And Utilities

| Symbol | Purpose |
| --- | --- |
| `Config` | Raw config object for generic config access |
| `load_config(path)` | Load raw config from disk |
| `Timestamp` | Python-visible timestamp type with datetime conversion |

### Order And Execution Model

| Symbol | Purpose |
| --- | --- |
| `Order` | Python order object for strategy submission |
| `OrderSide` | Buy / sell enum |
| `OrderType` | Market / limit / stop / stop-limit and related order kinds |
| `OrderStatus` | Order lifecycle state enum |
| `TimeInForce` | TIF enum |
| `Fill` | Fill object for execution callbacks and trade inspection |

### Regime Types

| Symbol | Purpose |
| --- | --- |
| `RegimeType` | Regime enum/type classification |
| `RegimeState` | Snapshot of current regime |
| `RegimeTransition` | Transition record between regimes |

### Engine Surface

| Symbol | Purpose |
| --- | --- |
| `BacktestConfig` | Backtest configuration object |
| `BacktestEngine` | Main engine runner |
| `BacktestResults` | Result/output container |
| `Portfolio` | Portfolio/account state access |
| `Position` | Position snapshot |

### Market Data Types

| Symbol | Purpose |
| --- | --- |
| `Bar` | OHLCV event |
| `Tick` | Tick event |
| `Quote` | Best-bid/best-ask event |
| `OrderBook` | Book snapshot/update |
| `BookLevel` | Single book level |
| `BarType` | Bar frequency/type enum |

### Strategy Surface

| Symbol | Purpose |
| --- | --- |
| `Strategy` | Base class for Python strategies |
| `StrategyContext` | Event-time context passed to strategies |
| `register_strategy` | Register Python strategy class by name |

### Top-Level Module And Alias Exports

| Symbol | Purpose |
| --- | --- |
| `analysis` | Analysis/reporting helper module |
| `config` | Raw config helper module |
| `data` | Market-data and dataframe helper module |
| `metrics` | Validation helper module |
| `research` | Research-session and parity module |
| `visualization` | Charts, dashboards, and HTML export |
| `walkforward` | Top-level alias for walk-forward types |
| `core_strategy` | Compatibility alias for the strategy binding module |
| `strategy_module` | Lazy-loaded Python strategy helper module |

### Walk-Forward Surface

| Symbol | Purpose |
| --- | --- |
| `ParameterDef` | Parameter definition for optimization |
| `WalkForwardConfig` | Walk-forward configuration |
| `WalkForwardOptimizer` | Python-facing optimizer |
| `WalkForwardResults` | Walk-forward output |
| `WindowResult` | Per-window result |

## Core Native Binding Methods

### `Timestamp`

Methods:

- `to_datetime()`
- `to_string()`

### `Config`

Methods:

- `has(key)`
- `get(key)`
- `get_path(path)`
- `set(key, value)`
- `set_path(path, value)`

Top-level functions:

- `load_config(path)`
- `parity_check(backtest, live)`

### `Order`

Python constructor:

- `Order(symbol, side, type, quantity, limit_price=None, stop_price=None)`

Use it with `StrategyContext.submit_order(...)`.

### `Bar`

Methods:

- `mid()`
- `typical()`
- `range()`
- `is_bullish()`
- `is_bearish()`

### `Quote`

Methods:

- `mid()`
- `spread()`
- `spread_bps()`

### `Portfolio`

Methods:

- `get_position(symbol)`
- `get_all_positions()`
- `equity_curve()`
- `margin_state()`

### `BacktestConfig`

Key helpers exposed in Python:

- `from_yaml(path)`
- `from_dict(dict)`
- `set_session_window(...)`
- `set_session_halts(...)`
- `set_session_calendar(...)`
- `set_queue_dynamics(...)`
- `set_account_margin(...)`
- `set_account_enforcement(...)`
- `set_account_financing(...)`

Use these when you want to control execution realism, queue behavior, session rules,
margin behavior, stop-out policy, and financing assumptions directly from Python.

### `BacktestResults`

Methods exposed to Python:

- `equity_curve()`
- `account_curve()`
- `trades()`
- `account_state()`
- `venue_fill_summary()`
- `dashboard_snapshot()`
- `dashboard_snapshot_json()`
- `dashboard_terminal(ansi_colors=False)`
- `tester_report()`
- `tester_journal()`
- `report_csv()`
- `report_json()`
- `performance_summary()`
- `performance_stats()`
- `regime_performance()`
- `transition_metrics()`
- `regime_metrics()`
- `regime_history()`

### `BacktestEngine`

Methods exposed to Python:

- `prepare()`
- `run(strategy_or_name)`
- `step()`
- `results()`
- `is_complete()`
- `current_time()`
- `run_parallel(...)`
- `dashboard_snapshot()`
- `dashboard_snapshot_json()`
- `dashboard_terminal(...)`
- `get_close_prices(...)`
- `get_bars_array(...)`

### `StrategyContext`

Methods:

- `submit_order(order)`
- `cancel_order(order_id)`
- `portfolio()`
- `get_position(symbol)`
- `current_regime()`
- `current_time()`
- `get_latest_bar(symbol)`
- `get_latest_quote(symbol)`
- `get_latest_book(symbol)`
- `get_bars(symbol, n)`
- `schedule_timer(...)`
- `cancel_timer(...)`

### `Strategy`

Callbacks available to Python strategies:

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

### `StrategyRegistry`

Exposed from `regimeflow.strategy`:

- `register(name, strategy_cls)`
- `decorator(name)`

## Python Helper Modules

### `regimeflow.config`

Exports:

- `Config`
- `load_config`

### `regimeflow.strategy`

Exports:

- `Strategy`
- `StrategyContext`
- `register_strategy`
- `StrategyRegistry`

Use this module when you want:

- an explicit Python strategy base-class import path
- decorator-based or programmatic strategy registration
- a stable module surface for Python strategy tooling

`StrategyRegistry` adds:

- `register(name, strategy_cls)`
- `decorator(name)`

### `regimeflow.metrics`

Exports:

- `validate_regime_attribution(results, tolerance=1.0e-6)`

Use this when you want an independent validation of regime-attribution numbers.

### `regimeflow.analysis`

Exports:

- `report_from_results`
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
- `equity_to_numpy`
- `trades_to_numpy`
- `display_report`
- `display_equity`

### `regimeflow.data`

Exports:

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

### `regimeflow.research`

Exports:

- `ParityResult`
- `ResearchSession`
- `parity_check`

`ResearchSession` is the notebook-friendly orchestration surface. Use it when you
want one object to own config loading, backtest runs, and parity checks.

### `regimeflow.visualization`

Exports:

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

Use this module when your Python workflow needs:

- static charts
- HTML dashboard export
- Dash application construction
- interactive live-style result review

## Python-First Usage Guidance

- Use `BacktestConfig` and `BacktestEngine` for core runs.
- Use `Strategy` and `StrategyContext` for custom event-driven logic.
- Use `analysis` for summaries/report export.
- Use `data` for dataframe integration.
- Use `research` for parity and session-oriented work.
- Use `visualization` for dashboards and chart output.

## Coverage Links

- `python/overview.md`
- `python/workflow.md`
- `python/cli.md`
- `tutorials/python-usage.md`
