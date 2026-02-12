# Architecture

This section maps the core components to the concrete code modules and shows
how they interact in both backtest and live trading flows.

## Component Diagram

```mermaid
flowchart TB
  subgraph Common
    C1[common/*]
    C2[result.h, json.h, time.h]
  end

  subgraph Data
    D1[data/*]
    D2[CSV, Tick, OrderBook]
    D3[WebSocketFeed]
    D4[Mmap + DB + API]
  end

  subgraph Regime
    R1[regime/*]
    R2[FeatureExtractor]
    R3[HMM + Ensemble]
  end

  subgraph Engine
    E1[engine/*]
    E2[EventLoop]
    E3[ExecutionPipeline]
    E4[Portfolio + Metrics]
  end

  subgraph Strategy
    S1[strategy/*]
    S2[StrategyFactory]
  end

  subgraph Risk
    K1[risk/*]
    K2[RiskManager]
  end

  subgraph Live
    L1[live/*]
    L2[BrokerAdapter]
    L3[LiveOrderManager]
    L4[MessageQueue]
  end

  Common --> Data
  Common --> Engine
  Common --> Live

  Data --> Engine
  Regime --> Engine
  Strategy --> Engine
  Risk --> Engine
  Engine --> Live
  Live --> Data
```


## Module-to-File Map

- `common/`: core utilities, results, time, JSON parsing
- `data/`: sources, validation, CSV readers, mmap storage, websocket feeds
- `regime/`: HMM, ensemble detection, features, state transitions
- `engine/`: event loop, execution pipeline, portfolio, order management
- `execution/`: execution models, slippage, market impact, latency models
- `risk/`: risk limits, position sizing, stop-loss
- `strategy/`: strategy interface, factories, built-ins
- `live/`: broker adapters, live engine, MQ adapters

## Key Interactions

1. `data::DataSource` emits bars/ticks into `engine::EventLoop`.
2. `regime::FeatureExtractor` computes features and updates `RegimeDetector`.
3. `strategy::Strategy` consumes market events and regime state to create orders.
4. `execution::ExecutionPipeline` simulates or routes orders and updates portfolio.
5. `metrics::*` compute performance and regime attribution.


## Interpretation

Interpretation: the architecture diagram maps each code module to the system layers and shows the dependencies between them.

