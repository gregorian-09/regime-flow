# Multi-Symbol Intraday Report (50k rows)

This report summarizes the latest multi-symbol intraday backtest run (BTCUSDT + ETHUSDT) using the Python
custom regime + signal ensemble strategy.

## Artifacts

- Report JSON: `examples/python_custom_regime_ensemble/multi_intraday_report.json`
- Equity curve: `examples/python_custom_regime_ensemble/multi_intraday_equity.png`
- Drawdown: `examples/python_custom_regime_ensemble/multi_intraday_drawdown.png`

## Run Command

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_custom_regime_ensemble/run_python_custom_regime_ensemble.py \
  --config examples/python_custom_regime_ensemble/config_intraday_multi.yaml
```
