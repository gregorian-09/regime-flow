# Regime Attribution Validation (Python)

RegimeFlow ships a small validation helper that recomputes regime attribution
from the equity curve and recorded regime history, then checks it against the
engine’s internal attribution metrics.

## Usage

```python
import regimeflow as rf
from regimeflow.metrics import validate_regime_attribution

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("pairs_trading")

ok, message = validate_regime_attribution(results)
print(ok, message)
```

## What It Checks

- Rebuilds regime assignment from `results.regime_history()` over the equity-return intervals.
- Recomputes per-regime total return using compounded returns, not arithmetic summation.
- Recomputes regime participation using duration-weighted time share, not sample-count share.
- Compares recomputed per-regime totals to `results.regime_metrics()`.
- Compares per-regime observation counts.

## Notes

- Uses a numeric tolerance of `1e-6` by default.
- Requires `results.regime_history()` to be present (captured during the run).
- This helper validates the regime-aware report conventions used by the engine:
  compounded regime return, duration-weighted regime share, and interval-based regime assignment.
```
