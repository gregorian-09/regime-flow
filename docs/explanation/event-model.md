# Event Model

The event model unifies backtest and live trading. Every update is normalized into
well‑defined event types so strategy and execution logic remain consistent.


## Event Categories

```mermaid
flowchart LR
  A[MarketDataUpdate] --> B[Event Loop]
  C[ExecutionReport] --> B
  D[PositionUpdate] --> B
  B --> E[Strategy]
  B --> F[Execution Pipeline]
  B --> G[Portfolio + Metrics]
```


## Interpretation

Events are time‑ordered updates that carry prices, fills, and positions. The event
loop guarantees consistent ordering for strategy logic.
