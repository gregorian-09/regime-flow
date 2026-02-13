# Python Transformer Regime Example

This example uses a lightweight transformer-style model to produce regime signals.
If PyTorch is available, a small transformer encoder is used; otherwise a numpy
attention fallback is used.

Optional dependency:
- `torch` (only required if you want the PyTorch transformer path)

## Run Strategy

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_transformer_regime/run_python_transformer_strategy.py \
  --config examples/python_transformer_regime/config.yaml
```

## Generate Regime CSV (for C++ plugin)

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_transformer_regime/generate_regime_csv.py \
  --config examples/python_transformer_regime/config.yaml \
  --output examples/python_transformer_regime/regime_signals.csv
```
