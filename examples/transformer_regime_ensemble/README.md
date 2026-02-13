# Transformer Regime Plugin (C++)

This example shows how to use transformer-generated regime signals in the C++ engine
via a CSV-backed regime detector plugin.

## Build Plugin

```bash
cmake -S examples/plugins/transformer_regime -B examples/plugins/transformer_regime/build \
  -DREGIMEFLOW_ROOT=$(pwd) -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/plugins/transformer_regime/build
```

## Generate Signals (Python)

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_transformer_regime/generate_regime_csv.py \
  --config examples/python_transformer_regime/config.yaml \
  --output examples/python_transformer_regime/regime_signals.csv
```

## Run Backtest (C++)

```bash
./examples/custom_regime_ensemble/build/run_custom_regime_backtest \
  --config examples/transformer_regime_ensemble/config.yaml
```
