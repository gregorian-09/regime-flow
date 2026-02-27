# Python Workflow

This page outlines a typical research workflow for quant developers using the Python API.

## 1. Build And Import

```bash
cmake -S . -B build
cmake --build build
export PYTHONPATH=python:build/lib
```

```python
import regimeflow as rf
```

## 2. Load Config

```python
cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
```

## 3. Run A Backtest

```python
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")
```

## 4. Analyze Results

```python
summary = rf.analysis.performance_summary(results)
curve = results.equity_curve()
trades = results.trades()
```

## 5. Export Reports

```python
with open("report.json", "w", encoding="utf-8") as f:
    f.write(results.report_json())
```

## 6. Iterate On Parameters

Use Python to generate parameter sets, then run `engine.run_parallel` or the walk-forward optimizer for robust selection.

## Next Steps

- `guide/backtesting.md`
- `guide/walkforward.md`
