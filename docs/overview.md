# Overview

RegimeFlow is a C++ core with Python bindings that provides:
- Data ingestion, validation, and canonicalization
- Backtesting and live trading engines
- Regime detection (HMM + ensembles) and regime-aware analytics
- Risk controls and execution models
- Broker adapters and message bus integration

The system is built around a consistent event pipeline that feeds data through
feature extraction, regime detection, strategy logic, execution models, and risk.

## System Map

```mermaid
flowchart LR
  subgraph Data
    A[CSV/Tick/OrderBook] --> B[Data Sources]
    B --> C[Validation + Normalization]
    C --> D[Bar Builder]
  end

  subgraph Engine
    D --> E[Event Generator]
    E --> F[Event Loop]
    F --> G[Strategy Context]
    G --> H[Strategy]
    H --> I[Execution Pipeline]
    I --> J[Portfolio + Metrics]
  end

  subgraph Regime
    D --> R1[Feature Extractor]
    R1 --> R2[Regime Detector]
    R2 --> H
    R2 --> J
  end

  subgraph Live
    L1[Broker Adapter] --> F
    F --> L1
    F --> L2[Message Bus]
  end
```


## Key Ideas

1. Data flow is consistent between backtest and live. The same event types and
   execution logic are used for both, with adapters at the edges.
2. Regime detection is a first-class signal that can influence strategy selection,
   risk limits, and reporting.
3. Execution models are pluggable. Users can extend the execution layer without
   rewriting the rest of the system.


## Interpretation

Interpretation: the overview diagram shows the end‑to‑end flow from data ingestion through regime detection, strategy decisions, and execution.

