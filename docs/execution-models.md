# Execution Models

Execution models control how orders are filled in backtests and how costs are modeled.

## Symbols

Let:
- $Q$ = order quantity
- $P_0$ = reference price
- $\delta$ = slippage amount
- $I$ = market impact
- $c$ = commission per unit

## Execution Model Flow

```mermaid
flowchart LR
  A[Order] --> B[Latency Model]
  B --> C[Slippage]
  C --> D[Market Impact]
  D --> E[Fill Simulator]
  E --> F[Portfolio Update]
```


## Formula Example (LaTeX)

$$
P_f = P_0 + \delta + I
$$

Interpretation: final fill price equals reference price plus slippage and impact.
