# Package: Python Interfaces

## Summary

Python bindings and analysis utilities. This layer mirrors the C++ engine concepts while exposing a Pythonic API for backtests, analysis, and dashboards.

Related docs:
- [Python Interfaces](../[Python Interfaces](reference/python-interfaces.md))
- [Python Usage](../[Python Usage](tutorials/python-usage.md))
- [Performance Metrics](../[Performance Metrics](explanation/performance-metrics.md))

## Structure Overview

- `python/regimeflow/` provides the Python package root.
- `python/regimeflow/analysis/` exposes analysis utilities and report helpers.
- `python/regimeflow/bindings/` exposes native bindings.
- `python/regimeflow/strategies/` contains Python strategy implementations and helpers.

## Type Index (Python)

| Module | Purpose |
| --- | --- |
| `regimeflow.bindings` | Python bindings to core engine types. |
| `regimeflow.analysis` | Metrics, reporting, and attribution utilities. |
| `regimeflow.strategies` | Strategy base classes and built-ins. |

## Lifecycle & Usage Notes

- The Python layer is designed for research workflows and reporting. Use the C++ engine for production execution.
- The analysis utilities consume outputs from backtests and live runs through standardized report objects.

## Type Details

### `regimeflow.bindings`

Native bindings to core C++ engine types (data sources, backtest runner, metrics).

See `api/python-metrics-validation.md` for the regime attribution validation helper.

### `regimeflow.analysis`

Analysis utilities for performance, attribution, and reporting pipelines.

### `regimeflow.strategies`

Python strategy base classes and built-in strategies for research.

## See Also

- [Python Interfaces](../[Python Interfaces](reference/python-interfaces.md))
- [Python Usage](../[Python Usage](tutorials/python-usage.md))
