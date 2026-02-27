# Slippage Math

Slippage models the difference between the reference price and the executed fill price.

## Slippage Diagram

```mermaid
flowchart LR
  A[Reference Price] --> B[Slippage Model]
  B --> C[Adjusted Fill Price]
```

## Fixed BPS

The fixed basis-point model applies a constant penalty based on notional:

- Buy: `fill = price * (1 + bps/10000)`
- Sell: `fill = price * (1 - bps/10000)`

## Regime BPS

The regime model applies a regime-specific bps value using the current regime label. This is useful for widening costs in volatile regimes.

See `guide/execution-models.md` for configuration.
