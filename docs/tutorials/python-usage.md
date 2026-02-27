# Python Usage

This tutorial shows the typical path for running a backtest from Python and exporting results.

## 1. Build And Import

```bash
cmake -S . -B build
cmake --build build
export PYTHONPATH=python:build/lib
```

```python
import regimeflow as rf
```

## 2. Load Configuration

```python
cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
```

## 3. Run A Built-In Strategy

```python
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")
```

## 4. Run A Custom Python Strategy

```python
class MyStrategy(rf.Strategy):
    def initialize(self, ctx):
        self.ctx = ctx

    def on_bar(self, bar):
        pass

engine = rf.BacktestEngine(cfg)
results = engine.run(MyStrategy())
```

## 5. Export Reports

```python
with open("report.json", "w", encoding="utf-8") as f:
    f.write(results.report_json())

results.equity_curve().to_csv("equity.csv", index=True)
results.trades().to_csv("trades.csv", index=True)
```
