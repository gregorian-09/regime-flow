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

- Sums equity returns by regime based on the engine’s `regime_history`.
- Compares recomputed per‑regime total returns to `results.regime_metrics()`.
- Compares per‑regime observation counts.

## Notes

- Uses a numeric tolerance of `1e-6` by default.
- Requires `results.regime_history()` to be present (captured during the run).
```
