# Custom Regime Plugins

This example contains two plugins:

- `custom_regime_detector`: a bespoke regime detector with custom features.
- `custom_regime_strategy`: a regime‑aware strategy that routes signals per regime.

## Build

From repo root:

```bash
cmake -S examples/plugins/custom_regime -B examples/plugins/custom_regime/build \
  -DREGIMEFLOW_ROOT=$(pwd) \
  -DREGIMEFLOW_BUILD=$(pwd)/build
cmake --build examples/plugins/custom_regime/build
```

Outputs:

- `examples/plugins/custom_regime/build/libcustom_regime_detector.so`
- `examples/plugins/custom_regime/build/libcustom_regime_strategy.so`
