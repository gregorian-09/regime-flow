# Python Custom Regime + Signal Ensemble

This example shows a Python strategy that computes its own regime state (trend/volatility/drawdown) and combines
multiple signals into a single trading decision. The regime model lives entirely in Python and does not require
a compiled plugin.

## Run

```bash
.venv/bin/python tools/download_intraday_spx_sample.py
.venv/bin/python tools/download_intraday_spx_sample.py --multi

PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_custom_regime_ensemble/run_python_custom_regime_ensemble.py \
  --config examples/python_custom_regime_ensemble/config.yaml

PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_custom_regime_ensemble/run_python_custom_regime_ensemble.py \
  --config examples/python_custom_regime_ensemble/config_intraday_multi.yaml
```

## Notes

- Uses intraday 1-minute BTCUSDT data from Binance public data.
- `tools/download_intraday_spx_sample.py` converts the raw format into RegimeFlow CSVs.
- Use `--multi` to download BTCUSDT and ETHUSDT and run `config_intraday_multi.yaml`.
- Strategy inputs are configured inside the Python script (lookback, thresholds, weights).
- The ensemble combines momentum, mean reversion, and breakout signals with normalized weights.
- The regime gate reduces risk in crisis/bear states.
