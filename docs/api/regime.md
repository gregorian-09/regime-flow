# Package: `regimeflow::regime`

## Summary

Regime detection and regime state management. Includes HMM, Kalman, constant detectors, feature extraction, and ensemble models. This package is extensible and intended for custom regime models.

Related diagrams:
- [Regime Detection](../[Regime Detection](explanation/regime-detection.md))
- [Regime Features](../[Regime Features](explanation/regime-features.md))
- [Regime Transitions](../[Regime Transitions](explanation/regime-transitions.md))

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/regime/constant_detector.h` | Constant regime detector. |
| `regimeflow/regime/ensemble.h` | Ensemble detector. |
| `regimeflow/regime/features.h` | Feature extraction for regimes. |
| `regimeflow/regime/hmm.h` | Hidden Markov Model detector. |
| `regimeflow/regime/kalman_filter.h` | Kalman filter model. |
| `regimeflow/regime/regime_detector.h` | Detector interface. |
| `regimeflow/regime/regime_factory.h` | Detector factory. |
| `regimeflow/regime/state_manager.h` | Regime state manager. |
| `regimeflow/regime/types.h` | Regime enums and data types. |

## Type Index

| Type | Description |
| --- | --- |
| `RegimeDetector` | Interface for regime models. |
| `HMMRegimeDetector` | HMM-based detector implementation. |
| `KalmanFilter1D` | Kalman filter for signal smoothing. |
| `EnsembleRegimeDetector` | Weighted detector composition. |
| `ConstantRegimeDetector` | Fixed-regime detector. |
| `RegimeStateManager` | Stores current regime and history. |
| `RegimeType` | Regime enumeration. |
| `RegimeState` | Regime state snapshot. |
| `RegimeTransition` | Regime transition payload. |
| `RegimeFactory` | Detector factory. |
| `RegimeFeatures` | Derived features used by detectors. |

## Lifecycle & Usage Notes

- Strategy selection can consume `RegimeStateManager` outputs to choose regime-specific strategies.
- The factory allows injection of custom detectors via plugins or config.

## Type Details

### `RegimeDetector`

Base interface for regime models. Produces regime labels and confidence estimates.

Methods:

| Method | Description |
| --- | --- |
| `on_bar(bar)` | Update with bar data. |
| `on_tick(tick)` | Update with tick data. |
| `on_book(book)` | Update with order book snapshot. |
| `train(features)` | Train with feature vectors. |
| `save(path)` | Persist detector state. |
| `load(path)` | Load detector state. |
| `configure(config)` | Configure detector parameters. |
| `num_states()` | Number of states in model. |
| `state_names()` | Human-readable state names. |

### `RegimeType` / `RegimeState` / `RegimeTransition`

Core regime types and transition payloads.

### `RegimeState`

Regime state snapshot.

### `RegimeTransition`

Regime transition payload.

### `HMMRegimeDetector`

Hidden Markov Model-based detector for regime identification.

Methods:

| Method | Description |
| --- | --- |
| `HMMRegimeDetector(states, window)` | Construct HMM detector. |
| `on_bar(bar)` | Update HMM with bar. |
| `on_tick(tick)` | Update HMM with tick. |
| `on_book(book)` | Update HMM with book. |
| `train(features)` | Train HMM. |
| `baum_welch(data, max_iter, tol)` | Baum-Welch training. |
| `log_likelihood(data)` | Compute log-likelihood. |
| `set_transition_matrix(matrix)` | Set transition matrix. |
| `set_emission_params(params)` | Set emission parameters. |
| `set_features(features)` | Set feature list. |
| `set_normalize_features(normalize)` | Enable normalization. |
| `set_normalization_mode(mode)` | Set normalization mode. |
| `save(path)` | Persist HMM. |
| `load(path)` | Load HMM. |
| `configure(config)` | Configure HMM parameters. |
| `num_states()` | Number of states. |
| `state_names()` | State names. |

### `GaussianParams`

Gaussian emission parameters for HMM states.

### `KalmanFilter1D`

Kalman filter for smoothing signals.

Methods:

| Method | Description |
| --- | --- |
| `KalmanFilter1D(process_noise, measurement_noise)` | Construct filter. |
| `configure(process_noise, measurement_noise)` | Configure noise parameters. |
| `reset()` | Reset filter state. |
| `update(measurement)` | Update filter and return estimate. |

### `EnsembleRegimeDetector` / `VotingMethod`

Combines multiple detectors with weights or voting.

Methods:

| Method | Description |
| --- | --- |
| `add_detector(detector, weight)` | Add detector to ensemble. |
| `on_bar(bar)` | Update ensemble with bar. |
| `on_tick(tick)` | Update ensemble with tick. |
| `train(features)` | Train all detectors. |

### `VotingMethod`

Ensemble voting method enum.

### `ConstantRegimeDetector`

Detector that always returns a fixed regime.

### `RegimeFactory`

Factory for detector construction from config.

### `RegimeStateManager`

Tracks regime history and transition statistics.

Methods:

| Method | Description |
| --- | --- |
| `RegimeStateManager(history_size)` | Construct with history size. |
| `update(state)` | Update with new regime state. |
| `current_regime()` | Current regime type. |
| `time_in_current_regime()` | Time spent in current regime. |
| `recent_transitions(n)` | Recent transitions. |
| `regime_frequencies()` | Regime frequency distribution. |
| `avg_regime_duration(regime)` | Average duration per regime. |
| `empirical_transition_matrix()` | Empirical transition matrix. |
| `register_transition_callback(callback)` | Register transition callback. |

### `RegimeFeatures`

Feature extraction pipeline used by detectors (returns, volatility, correlations).

Methods:

| Method | Description |
| --- | --- |
| `FeatureExtractor(window)` | Construct with lookback window. |
| `set_window(window)` | Set window size. |
| `set_features(features)` | Set feature list. |
| `set_normalize(normalize)` | Enable normalization. |
| `set_normalization_mode(mode)` | Set normalization mode. |
| `features()` | Get feature list. |
| `normalization_mode()` | Get normalization mode. |
| `on_bar(bar)` | Update and compute features from bar. |
| `on_book(book)` | Update and compute features from book. |
| `update_cross_asset_features(...)` | Update cross-asset features. |

### `FeatureExtractor` / `FeatureType` / `NormalizationMode`

Feature extraction class and enums.

### `FeatureType`

Feature type enum.

### `NormalizationMode`

Normalization mode enum.

## Method Details

Type Hints:

- `bar` → `data::Bar`
- `tick` → `data::Tick`
- `book` → `data::OrderBook`
- `features` → `std::vector<FeatureVector>`
- `config` → `Config`

### `RegimeDetector`

#### `on_bar(bar)` / `on_tick(tick)` / `on_book(book)`
Parameters: market data.
Returns: `RegimeState`.
Throws: None.

#### `train(features)` / `save(path)` / `load(path)` / `configure(config)`
Parameters: features/path/config.
Returns: `void`.
Throws: `Error::ConfigError` or `Error::IoError`.

### `HMMRegimeDetector`

#### `baum_welch(data, max_iter, tol)`
Parameters: feature vectors and tuning params.
Returns: `void`.
Throws: None.

#### `log_likelihood(data)`
Parameters: feature vectors.
Returns: log-likelihood.
Throws: None.

### `FeatureExtractor`

#### `on_bar(bar)` / `on_book(book)`
Parameters: bar/book.
Returns: `FeatureVector`.
Throws: None.

#### `set_window(window)` / `set_features(features)` / `set_normalize(flag)` / `set_normalization_mode(mode)`
Parameters: config values.
Returns: `void`.
Throws: None.

### `RegimeStateManager`

#### `update(state)`
Parameters: regime state.
Returns: `void`.
Throws: None.

#### `current_regime()` / `time_in_current_regime()`
Parameters: None.
Returns: current regime / duration.
Throws: None.

### `RegimeFactory`

#### `create(config)`
Parameters: config.
Returns: detector instance.
Throws: `Error::ConfigError`.

## Usage Examples

```cpp
#include "regimeflow/regime/hmm.h"

regimeflow::regime::HMMRegimeDetector detector(4, 50);
auto state = detector.on_bar(bar);
```
## See Also

- [Regime Detection](../[Regime Detection](explanation/regime-detection.md))
- [Regime Features](../[Regime Features](explanation/regime-features.md))
- [Regime Transitions](../[Regime Transitions](explanation/regime-transitions.md))
