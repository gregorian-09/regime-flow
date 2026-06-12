# Regime Detection

Regime detection classifies market state and can be consumed by strategies, risk limits, or reporting. The detector is configured under `regime` (C++ config) or `regime_detector` and `regime_params` (Python config).

## Detector Types

- `constant` returns a fixed regime label.
- `hmm` uses a Hidden Markov Model.
- `ensemble` aggregates multiple detectors.
- Plugins can provide custom detectors.

## Constant Detector

Config keys:

- `regime.detector: constant`
- `regime.regime` value: `bull`, `neutral`, `bear`, `crisis`.

## HMM Detector

Config keys:

- `regime.detector: hmm`
- `regime.hmm.states` integer.
- `regime.hmm.window` integer.
- `regime.hmm.normalize_features` boolean.
- `regime.hmm.normalization` string, e.g. `zscore`.
- `regime.hmm.kalman_enabled` boolean.
- `regime.hmm.kalman_process_noise` double.
- `regime.hmm.kalman_measurement_noise` double.

Example:

```yaml
regime:
  detector: hmm
  hmm:
    states: 4
    window: 20
    normalize_features: true
    normalization: zscore
    kalman_enabled: true
    kalman_process_noise: 1e-3
    kalman_measurement_noise: 1e-2
```

## Ensemble Detector

Config keys:

- `regime.detector: ensemble`
- `regime.ensemble.voting_method` values: `weighted_average`, `majority`, `confidence_weighted`, `bayesian`.
- `regime.ensemble.detectors` list of detector configs.
- `regime.ensemble.detectors[].weight` weighting per detector.

Example:

```yaml
regime:
  detector: ensemble
  ensemble:
    voting_method: weighted_average
    detectors:
      - type: hmm
        weight: 0.7
        params:
          hmm:
            states: 4
            window: 20
      - type: constant
        weight: 0.3
        params:
          regime: neutral
```

## Plugin Detectors

Plugin detectors are created through `RegimeFactory`. Provide the plugin name in `regime.detector`, and pass plugin parameters in `regime.params`.


## Model Governance Metadata

Regime detectors expose `model_metadata()` so backtest, live, and replay artifacts can record
which detector version produced each prediction. HMM persistence now writes and reads:

- detector type;
- model version;
- training start/end timestamps in microseconds;
- feature schema;
- parameter digest.

Use this metadata in audit records and model promotion reports so a regime decision can be traced
back to the exact training window and feature contract.

## Next Steps

- `guide/strategies.md`
- `guide/risk-management.md`
- `reference/configuration.md`
