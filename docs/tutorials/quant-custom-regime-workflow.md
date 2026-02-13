# Quant Workflow: Custom Regime Model + Complex Strategy

This workflow walks through building a **custom regime detector**, defining its **feature set**, and wiring a **complex strategy** into RegimeFlow. It includes configuration, plugin registration, and execution flow diagrams.

## 1. Define Regime Features

Start by deciding which features determine regimes. Common choices:
- Trend + volatility
- Momentum + dispersion
- Liquidity + drawdown

Example feature plan (daily bars):
- `trend_20`: 20-day return
- `vol_20`: 20-day realized volatility
- `drawdown_60`: 60-day drawdown

If a feature is not available in RegimeFlow, compute it inside your custom detector. The detector can own a feature builder with its own rolling windows (example in `examples/plugins/custom_regime/`).

Related:
- [Regime Features](../explanation/regime-features.md)
- [HMM Math](../explanation/hmm-math.md)

## 2. Implement a Custom Regime Detector (Plugin)

Create a detector that consumes feature vectors and outputs a regime label.

Pseudo‑API (conceptual):
```
class CustomRegimeDetector : public RegimeDetector {
public:
  void fit(const FeatureMatrix& X) override;
  RegimeState predict(const FeatureVector& x) override;
};
```

Example feature builder inside the detector (custom `skew` feature):
```
class CustomFeatureBuilder {
public:
  explicit CustomFeatureBuilder(int window = 60);
  FeatureVector on_bar(const Bar& bar); // [trend, vol, drawdown, skew]
};
```

Register your detector in the plugin system (engine config):
```
plugins:
  search_paths:
    - examples/plugins/custom_regime/build
  load:
    - libcustom_regime_detector.so
```

Related:
- [Plugin API](../reference/plugin-api.md)
- [Regime Detection](../explanation/regime-detection.md)

## 3. Configure Regime Model and Features

Configure the detector and pass feature params:
```
regime:
  detector: custom_regime
  params:
    window: 60
    trend_threshold: 0.02
    vol_threshold: 0.015

strategy:
  name: custom_regime_strategy
  params:
    symbol: AAPL
    base_qty: 10
    trend_qty: 20
    stress_qty: 5
```

## 4. Build a Complex Strategy

Design a strategy that is **regime‑aware** and **multi‑signal**:

Inputs:
- Regime label (e.g., Trend, Range, Stress)
- Signal ensemble (momentum + mean reversion + carry)
- Risk throttles (volatility & drawdown)

Example strategy config:
```
strategy: my_regime_ensemble
strategy_params:
  regime_map:
    Trend: momentum
    Range: mean_reversion
    Stress: defensive
  signal_weights:
    momentum: 0.6
    mean_reversion: 0.3
    carry: 0.1
  throttle:
    max_vol: 0.18
    max_dd: 0.10
```

Related:
- [Strategy Selection](../explanation/strategy-selection.md)
- [Execution Models](../explanation/execution-models.md)

## 5. Wire Everything into the Backtest

Complete backtest config (minimal excerpt):
```
data_source: mmap
data:
  root_dir: /data/mmap
  symbols: AAPL,MSFT

regime:
  detector: custom_regime
  params:
    window: 60

strategy:
  name: custom_regime_strategy
  params:
    symbol: AAPL

execution_model: basic
risk:
  max_symbol_exposure: 0.2
  max_drawdown: 0.15
```

Full example config:
- `examples/custom_regime_ensemble/config.yaml`

## 6. Run and Validate

Run the backtest:
```
./build/bin/regimeflow_backtest --config configs/regime_ensemble.yaml
```

Or run the C++ example driver:
```
./examples/custom_regime_ensemble/build/run_custom_regime_backtest \
  --config examples/custom_regime_ensemble/config.yaml
```

Validate regime attribution:
```
import regimeflow as rf
from regimeflow.metrics.validation import validate_regime_attribution

cfg = rf.BacktestConfig.from_yaml("configs/regime_ensemble.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("my_regime_ensemble")
validate_regime_attribution(results)
```

Related:
- [Performance Metrics](../explanation/performance-metrics.md)
- [Metrics Validation](../api/python-metrics-validation.md)

---

## Diagram: Custom Regime + Strategy Flow

```mermaid
flowchart LR
  A[Market Data] --> B[Feature Builder]
  B --> C[Custom Regime Detector]
  C --> D[Regime State]
  D --> E[Strategy Ensemble]
  E --> F[Execution Model]
  F --> G[Portfolio + Metrics]
```

## Diagram: Plugin Registration

```mermaid
sequenceDiagram
  participant Config as config.yaml
  participant Registry as PluginRegistry
  participant Detector as CustomRegimeDetector
  Config->>Registry: load plugin metadata
  Registry->>Detector: instantiate detector
  Detector-->>Registry: ready
```

## Diagram: Regime-Aware Strategy Routing

```mermaid
flowchart LR
  A[Regime Label] --> B{Regime Map}
  B -->|Trend| C[Momentum Signal]
  B -->|Range| D[Mean Reversion Signal]
  B -->|Stress| E[Defensive Signal]
  C --> F[Position Sizing]
  D --> F
  E --> F
  F --> G[Order Generation]
```
