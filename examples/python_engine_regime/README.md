# Python Engine-Regime Strategy

This example shows a Python strategy that relies on the engine's regime detector (e.g., HMM) and reacts via
`ctx.current_regime()`.

## Run

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_engine_regime/run_python_engine_regime.py \
  --config examples/python_engine_regime/config.yaml
```

## Notes

- The regime detector is configured in the YAML config.
- To use a custom detector plugin, set `plugins_search_paths` and/or `plugins_load` in the YAML config or in
  `BacktestConfig` before creating the engine.

Example using a custom plugin detector:

```bash
PYTHONPATH=python:build/lib .venv/bin/python \
  examples/python_engine_regime/run_python_engine_regime.py \
  --config examples/python_engine_regime/config_custom_plugin.yaml
```
