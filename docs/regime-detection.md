# Regime Detection

RegimeFlow uses market features to classify the market into regimes (e.g., trend, mean-revert, volatile).

## Regime Detection Pipeline

```mermaid
flowchart LR
  A[Market Data] --> B[Feature Extractor]
  B --> C[HMM / Ensemble]
  C --> D[Kalman Smoothing]
  D --> E[Regime State]
  E --> F[Strategy + Risk]
```


## What It Means

- The system **measures market behavior** (volatility, momentum, etc.).
- A **regime model** classifies the market into categories.
- A **smoother** reduces noisy regime flips.
- Strategies can react differently depending on the regime.


## Interpretation

Interpretation: features are mapped to regime probabilities, then smoothed into a stable state.

