# Package: `regimeflow::walkforward`

## Summary

Walk-forward optimization and parameter search. Provides optimizers, sampling distributions, and trial bookkeeping.

Related diagrams:
- [Walk-Forward](../walk-forward.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/walkforward/optimizer.h` | Walk-forward optimizer and trial control. |

## Type Index

| Type | Description |
| --- | --- |
| `WalkForwardOptimizer` | Runs train/validate/test splits. |
| `WalkForwardConfig` | Optimization configuration. |
| `ParameterDef` | Parameter definition for optimization. |
| `ParameterSet` | Parameter name/value map. |
| `WindowResult` | Single window outcome. |
| `WalkForwardResults` | Aggregated walk-forward outputs. |

## Lifecycle & Usage Notes

- Optimizers integrate with `BacktestRunner` to execute trial batches.
- Results can be exported through `ReportWriter` in metrics.

## Type Details

### `WalkForwardOptimizer`

Coordinates train/validate/test splits and aggregates trial outcomes.

Methods:

| Method | Description |
| --- | --- |
| `WalkForwardOptimizer(config)` | Construct optimizer. |
| `optimize(params, strategy_factory, data_source, full_range, detector_factory)` | Run walk-forward optimization. |
| `on_window_complete(callback)` | Register window-complete callback. |
| `on_trial_complete(callback)` | Register trial-complete callback. |
| `on_regime_train(callback)` | Register pre-regime-train hook. |
| `on_regime_trained(callback)` | Register post-regime-train callback. |
| `cancel()` | Cancel running optimization. |
### `WalkForwardConfig` / `ParameterDef` / `ParameterSet`

Configuration and parameter definitions for optimization.

Fields:

| Member | Description |
| --- | --- |
| `WalkForwardConfig` | Optimization configuration fields and enums. |
| `ParameterDef` | Name/type/range/distribution. |
| `ParameterSet` | Parameter name/value map. |

### `ParameterDef`

Parameter definition for optimization.

### `Distribution`

Sampling distribution enum.

### `OptMethod`

Optimization method enum.

### `WindowType` / `OptMethod` / `Distribution`

Optimization enums for windowing and sampling.

### `RegimeTrainingContext`

Context passed to regime training callbacks.

### `TrialOutcome`

Result of a single optimization trial.
### `WindowResult` / `WalkForwardResults`

Per-window and aggregated optimization outputs.

### `WalkForwardResults`

Aggregated walk-forward results.

## Method Details

Type Hints:

- `params` → `std::vector<ParameterDef>`
- `strategy_factory` → `std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)>`
- `data_source` → `data::DataSource*`
- `full_range` → `TimeRange`

### `WalkForwardOptimizer`

#### `optimize(params, strategy_factory, data_source, full_range, detector_factory)`
Parameters: parameters, strategy factory, data source, range, optional detector factory.
Returns: `WalkForwardResults`.
Throws: None.

#### `on_window_complete(callback)` / `on_trial_complete(callback)`
Parameters: callbacks.
Returns: `void`.
Throws: None.

#### `on_regime_train(callback)` / `on_regime_trained(callback)`
Parameters: callbacks.
Returns: `void`.
Throws: None.

#### `cancel()`
Parameters: None.
Returns: `void`.
Throws: None.

## Usage Examples

```cpp
#include "regimeflow/walkforward/optimizer.h"

regimeflow::walkforward::WalkForwardConfig cfg;
regimeflow::walkforward::WalkForwardOptimizer optimizer(cfg);
```
## See Also

- [Walk-Forward](../walk-forward.md)
