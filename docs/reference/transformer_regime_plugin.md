# Transformer Regime Plugin (TorchScript)

This section describes a TorchScript-backed regime detector plugin. It requires libtorch.

## Overview

- Load a TorchScript model (`.pt`) at plugin initialization.
- Compute features from bars/ticks in C++.
- Run the model to produce regime probabilities.
- Map probabilities to RegimeFlow regimes.

## Expected Config

```yaml
regime:
  detector: transformer_torchscript
  params:
    model_path: /path/to/regime_transformer.pt
    feature_window: 120
    feature_dim: 8
```

## Build Requirements

- libtorch (C++ API for PyTorch)
- CMake find_package(Torch) integration

## Implementation

- Implementation lives in:
  - `examples/plugins/transformer_regime/transformer_torchscript_detector.cpp`
  - `examples/plugins/transformer_regime/transformer_torchscript_detector.h`

- Normalize features identically to the Python training pipeline.

## Notes

- TorchScript model should output logits for 4 regimes (bull/neutral/bear/crisis).
- Ensure deterministic inference (set eval mode, disable grads).

The plugin loads a TorchScript module, runs inference on a `[1, window, feature_dim]` tensor,
and maps the probabilities to RegimeFlow regime labels.

This file documents the integration; implementation can be added once libtorch is available.
