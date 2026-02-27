# Backtest Methodology

RegimeFlow backtests are event-driven and model execution costs explicitly. The methodology mirrors live behavior wherever possible.

## Methodology Diagram

```mermaid
flowchart LR
  A[Historical Data] --> B[Data Source]
  B --> C[Event Loop]
  C --> D[Strategy]
  D --> E[Execution Models]
  E --> F[Portfolio + Metrics]
```

## Key Principles

- **Event-driven**: bars, ticks, and order books drive strategy callbacks.
- **Explicit costs**: slippage, commission, impact, and latency are modeled.
- **Single pipeline**: the same strategy contract is used in backtest and live.

## Fill Modeling

Fills are produced by the execution pipeline and update portfolio state. Costs are applied before the fill is recorded.

## Risk Enforcement

Risk limits are checked before orders are accepted, and portfolio-level limits can prevent additional exposure.

## Determinism

CSV-based backtests are deterministic when the same data and config are used.
