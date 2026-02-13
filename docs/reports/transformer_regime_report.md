# Transformer Regime Report

This report captures the Python transformer-regime backtest and the artifacts generated from it.

## Artifacts

- `docs/assets/transformer_report.json`
- `docs/assets/transformer_equity.png`
- `docs/assets/transformer_drawdown.png`

## Plots

![Transformer Equity](../assets/transformer_equity.png)
![Transformer Drawdown](../assets/transformer_drawdown.png)

## Run Command

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_transformer_regime/run_python_transformer_strategy.py \
  --config examples/python_transformer_regime/config.yaml
```
