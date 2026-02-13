# Custom Regime + Strategy Example

This example shows how to build and run a custom regime detector and a complex regime‑aware strategy using plugins.

## Build Plugins

From the repo root:

```bash
cmake -S examples/plugins/custom_regime -B examples/plugins/custom_regime/build \
  -DREGIMEFLOW_ROOT=$(pwd) \
  -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/plugins/custom_regime/build
```

This produces:

- `examples/plugins/custom_regime/build/libcustom_regime_detector.so`
- `examples/plugins/custom_regime/build/libcustom_regime_strategy.so`

## Run the Backtest (C++)

```bash
cmake -S examples/custom_regime_ensemble -B examples/custom_regime_ensemble/build \
  -DREGIMEFLOW_ROOT=$(pwd) \
  -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/custom_regime_ensemble/build
./examples/custom_regime_ensemble/build/run_custom_regime_backtest \
  --config examples/custom_regime_ensemble/config.yaml
```

## Notes

- The custom detector computes a feature that is **not** part of the built‑in `FeatureType` enum.
- The detector owns its own feature calculation logic, which is typical when you need bespoke signals.
