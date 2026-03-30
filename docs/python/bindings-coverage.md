# Python Bindings Coverage

This page is the Python-binding coverage index. Its purpose is simple:

- list the Python-visible surface
- point to the page that explains it
- make documentation drift obvious

Coverage here means the documented surface matches the package surface exported
from:

- `python/regimeflow/__init__.py`
- `python/regimeflow/config.py`
- `python/regimeflow/strategy/__init__.py`
- `python/regimeflow/metrics/__init__.py`
- `python/regimeflow/analysis/__init__.py`
- `python/regimeflow/data/__init__.py`
- `python/regimeflow/research/__init__.py`
- `python/regimeflow/visualization/__init__.py`
- `python/bindings.cpp`

## Top-Level Package Coverage

| Export | Coverage Page |
| --- | --- |
| `Config`, `load_config`, `Timestamp` | `api/python.md` |
| `Order`, `OrderSide`, `OrderType`, `OrderStatus`, `TimeInForce`, `Fill` | `api/python.md` |
| `RegimeType`, `RegimeState`, `RegimeTransition` | `api/python.md` |
| `BacktestConfig`, `BacktestEngine`, `BacktestResults`, `Portfolio`, `Position` | `api/python.md`, `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md` |
| `Bar`, `Tick`, `Quote`, `OrderBook`, `BookLevel`, `BarType` | `api/python.md` |
| `Strategy`, `StrategyContext`, `register_strategy` | `api/python.md`, `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md` |
| `ParameterDef`, `WalkForwardConfig`, `WalkForwardOptimizer`, `WalkForwardResults`, `WindowResult` | `api/python.md`, `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md` |
| `analysis`, `config`, `metrics`, `research`, `visualization` | `api/python.md`, `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md` |
| `walkforward`, `core_strategy`, `strategy_module` | `api/python.md`, `python/overview.md`, `python/README.md` |

## Helper Module Coverage

| Module | Exact Exports | Covered In |
| --- | --- | --- |
| `regimeflow.config` | `Config`, `load_config` | `api/python.md` |
| `regimeflow.strategy` | `Strategy`, `StrategyContext`, `register_strategy`, `StrategyRegistry` | `api/python.md`, `python/workflow.md`, `tutorials/python-usage.md` |
| `regimeflow.metrics` | `validate_regime_attribution` | `api/python.md`, `tutorials/python-usage.md`, `python/README.md` |
| `regimeflow.analysis` | `report_from_results`, `performance_summary`, `performance_stats`, `regime_performance`, `transition_metrics`, `equity_curve`, `trades`, `summary_dataframe`, `stats_dataframe`, `regime_dataframe`, `transitions_dataframe`, `report_json`, `report_csv`, `report_html`, `write_report_json`, `write_report_csv`, `write_report_html`, `equity_to_numpy`, `trades_to_numpy`, `display_report`, `display_equity` | `api/python.md`, `python/overview.md`, `tutorials/python-usage.md`, `python/README.md` |
| `regimeflow.data` | `bars_to_dataframe`, `dataframe_to_bars`, `ticks_to_dataframe`, `dataframe_to_ticks`, `results_to_dataframe`, `DataFrameDataSource`, `load_csv_bars`, `load_csv_ticks`, `load_csv_dataframe`, `normalize_timezone`, `fill_missing_time_bars` | `api/python.md`, `python/workflow.md`, `tutorials/python-usage.md`, `python/README.md` |
| `regimeflow.research` | `ParityResult`, `ResearchSession`, `parity_check` | `api/python.md`, `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md`, `python/README.md` |
| `regimeflow.visualization` | `plot_results`, `create_dashboard`, `create_strategy_tester_dashboard`, `create_live_dashboard`, `dashboard_snapshot_to_live_dashboard`, `create_interactive_dashboard`, `create_dash_app`, `launch_dashboard`, `create_live_dash_app`, `launch_live_dashboard`, `export_dashboard_html` | `api/python.md`, `python/overview.md`, `tutorials/python-usage.md`, `python/README.md` |

## Python User Tasks Covered

| Task | Coverage Page |
| --- | --- |
| Run a backtest from Python | `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md`, `python/README.md` |
| Write a Python strategy | `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md`, `python/README.md` |
| Export JSON / CSV / HTML reports | `python/workflow.md`, `tutorials/python-usage.md`, `api/python.md`, `python/README.md` |
| Use Pandas / NumPy helpers | `python/workflow.md`, `tutorials/python-usage.md`, `api/python.md`, `python/README.md` |
| Validate regime attribution | `tutorials/python-usage.md`, `api/python.md`, `python/README.md` |
| Use research sessions and parity checks | `python/overview.md`, `python/workflow.md`, `tutorials/python-usage.md`, `api/python.md`, `python/README.md` |
| Use visualization / dashboard helpers | `python/overview.md`, `tutorials/python-usage.md`, `api/python.md`, `python/README.md` |
| Use the CLI | `python/cli.md`, `python/README.md` |
| Use walk-forward types | `python/overview.md`, `python/workflow.md`, `api/python.md`, `python/README.md` |

## Packaging Coverage

| Surface | Coverage Page |
| --- | --- |
| PyPI package overview | `python/README.md` |
| Optional extras | `python/README.md` |
| CLI entry point | `python/README.md`, `python/cli.md` |
| Top-level aliases and helper modules | `python/README.md`, `api/python.md`, `python/overview.md` |
| Package boundaries and limitations | `python/README.md`, `python/overview.md` |

## Coverage Statement

The documentation now provides:

- a Python-first PyPI README
- a Python overview page
- a Python workflow page
- a Python CLI reference
- a Python use-case tutorial
- a Python API reference
- an explicit export-by-export coverage index

Under that definition, the published Python package surface is documented at
full coverage for the currently exported bindings and helper modules.
