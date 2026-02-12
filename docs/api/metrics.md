# Package: `regimeflow::metrics`

## Summary

Performance and attribution metrics for strategies, regimes, and portfolios. Includes drawdown analysis, regime attribution, transition metrics, and report writers.

Related diagrams:
- [Performance Metrics](../[Performance Metrics](explanation/performance-metrics.md))
- [Regime Transitions](../[Regime Transitions](explanation/regime-transitions.md))

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/metrics/attribution.h` | Return attribution helpers. |
| `regimeflow/metrics/drawdown.h` | Drawdown calculations. |
| `regimeflow/metrics/metrics_tracker.h` | Metrics tracking coordinator. |
| `regimeflow/metrics/performance.h` | Performance data structures. |
| `regimeflow/metrics/performance_calculator.h` | Computes metrics from trades and equity curve. |
| `regimeflow/metrics/performance_metric.h` | Metric interface and definitions. |
| `regimeflow/metrics/performance_metrics.h` | Collection of common metrics. |
| `regimeflow/metrics/regime_attribution.h` | Regime-level attribution. |
| `regimeflow/metrics/report.h` | Report data model. |
| `regimeflow/metrics/report_writer.h` | Report writers (CSV/JSON). |
| `regimeflow/metrics/transition_metrics.h` | Transition stats and counters. |

## Type Index

| Type | Description |
| --- | --- |
| `MetricsTracker` | Coordinates metric computation. |
| `PerformanceCalculator` | Computes returns, drawdowns, ratios. |
| `PerformanceMetric` | Interface for metric plugins. |
| `RegimeAttribution` | Attribution by regime. |
| `TransitionMetrics` | Tracks regime transition counts and costs. |
| `Report` / `ReportWriter` | Report containers and writers. |

## Lifecycle & Usage Notes

- Metrics are typically computed at end-of-run or on periodic snapshots.
- Transition metrics integrate with `RegimeTracker` in the engine.

## Type Details

### `MetricsTracker`

Coordinates metric collection and manages registered `PerformanceMetric` instances.

Methods:

| Method | Description |
| --- | --- |
| `update(timestamp, equity)` | Update tracker with equity only. |
| `update(timestamp, portfolio, regime)` | Update with full portfolio and regime. |
| `equity_curve()` | Access equity curve. |
| `drawdown()` | Access drawdown tracker. |
| `attribution()` | Access attribution tracker. |
| `regime_attribution()` | Access regime attribution. |
| `transition_metrics()` | Access transition metrics. |

Notes:
- Captures portfolio snapshots when `update(timestamp, portfolio, regime)` is used, enabling
  open-trade metrics in reports (open trades, unrealized PnL, and snapshot date).
- Stores full regime history when the engine supplies regime state.

### `PerformanceCalculator`

Computes returns, drawdowns, and risk-adjusted ratios from trades and equity curves.

Methods:

| Method | Description |
| --- | --- |
| `calculate(equity_curve, fills, risk_free_rate, benchmark)` | Compute overall performance summary. |
| `calculate_by_regime(equity_curve, fills, regimes, risk_free_rate)` | Compute performance by regime. |
| `calculate_transitions(equity_curve, transitions)` | Compute transition metrics. |
| `calculate_attribution(equity_curve, regimes, factor_returns)` | Compute factor attribution. |
| `regime_robustness_score(regime_metrics)` | Compute robustness score. |

### `PerformanceMetric`

Interface for custom metrics. Implementations register with `MetricsTracker`.

Methods:

| Method | Description |
| --- | --- |
| `name()` | Metric name. |
| `compute(curve, periods_per_year)` | Compute metric value. |

### `RegimeAttribution`

Allocates returns and risk to regimes, supporting attribution analysis.

Methods:

| Method | Description |
| --- | --- |
| `update(regime, equity_return)` | Update attribution state. |
| `results()` | Access computed results. |

### `TransitionMetrics`

Tracks regime transition statistics and costs.

Methods:

| Method | Description |
| --- | --- |
| `update(from, to, equity_return)` | Update transition metrics. |
| `results()` | Access transition stats. |

### `Report` / `ReportWriter`

Structured report output and writers (CSV/JSON).

Methods:

| Method | Description |
| --- | --- |
| `build_report(tracker, periods_per_year)` | Build report from tracker. |
| `build_report(tracker, fills, risk_free_rate, benchmark)` | Build report with fills. |
| `ReportWriter::to_csv(report)` | Serialize report to CSV. |
| `ReportWriter::to_json(report)` | Serialize report to JSON. |

### `EquityCurve`

Equity curve data structure (vector of portfolio snapshots).

### `DrawdownTracker` / `DrawdownSnapshot`

Drawdown tracking utilities.

### `DrawdownSnapshot`

Snapshot of drawdown statistics.

### `AttributionTracker` / `AttributionSnapshot`

Attribution tracking utilities.

### `AttributionSnapshot`

Snapshot of attribution statistics.

### `PerformanceStats` / `PerformanceSummary`

Aggregate performance statistics and summary metrics.

### `PerformanceSummary`

Summary of performance metrics.

Includes best/worst return values and their corresponding dates:
- `best_day`, `worst_day`
- `best_day_date`, `worst_day_date`
- `best_month`, `worst_month`
- `best_month_date`, `worst_month_date`
- `open_trades` (count of non-flat positions in the most recent snapshot that has open positions)
- `closed_trades`
- `open_trades_unrealized_pnl` (sum of unrealized PnL across those open positions)
- `open_trades_snapshot_date` (timestamp of the snapshot used to compute open-trade stats)

### `RegimePerformance` / `RegimeMetrics`

Regime-specific performance summaries.

### `RegimeMetrics`

Per-regime performance metrics.

### `TransitionStats` / `TransitionMetricsSummary`

Regime transition statistics.

### `TransitionMetricsSummary`

Summary of regime transition metrics.

### `AttributionResult`

Attribution decomposition for regimes/factors.

### `TradeSummary`

Aggregated trade summary derived from fills.

### `ReportWriter`

Report serialization helpers.

## Method Details

Type Hints:

- `equity_curve` → `std::vector<engine::PortfolioSnapshot>`
- `fills` → `std::vector<engine::Fill>`
- `regime` → `regime::RegimeType`
- `transitions` → `std::vector<regime::RegimeTransition>`

### `MetricsTracker`

#### `update(timestamp, equity)`
Parameters: update time and equity.
Returns: `void`.
Throws: None.

#### `update(timestamp, portfolio, regime)`
Parameters: time, portfolio, optional regime.
Returns: `void`.
Throws: None.

### `PerformanceCalculator`

#### `calculate(equity_curve, fills, risk_free_rate, benchmark)`
Parameters: equity curve, fills, risk-free rate, optional benchmark.
Returns: `PerformanceSummary`.
Throws: None.

#### `calculate_by_regime(...)`
Parameters: equity curve, fills, regimes, risk-free rate.
Returns: Map of regime metrics.
Throws: None.

#### `calculate_transitions(...)`
Parameters: equity curve, transitions.
Returns: Transition summaries.
Throws: None.

#### `calculate_attribution(...)`
Parameters: equity curve, regimes, factor returns.
Returns: `AttributionResult`.
Throws: None.

### `ReportWriter`

#### `to_csv(report)` / `to_json(report)`
Parameters: report.
Returns: CSV/JSON string.
Throws: None.

### `build_report(...)`
Parameters: metrics tracker (+ optional fills/benchmark).
Returns: `Report`.
Throws: None.

## Usage Examples

```cpp
#include "regimeflow/metrics/performance_calculator.h"

regimeflow::metrics::PerformanceCalculator calc;
auto summary = calc.calculate(equity_curve, fills, 0.02);
```

## See Also

- [Performance Metrics](../[Performance Metrics](explanation/performance-metrics.md))
- [Regime Transitions](../[Regime Transitions](explanation/regime-transitions.md))
