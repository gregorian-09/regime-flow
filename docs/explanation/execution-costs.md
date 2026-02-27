# Execution Costs

Execution costs are modeled explicitly to avoid overly optimistic backtests. Costs are configured under `execution.*`.

## Cost Stack Diagram

```mermaid
flowchart LR
  A[Reference Price] --> B[Slippage]
  B --> C[Commission]
  C --> D[Impact]
  D --> E[Final Fill]
```

## Commission

Commission is modeled per fill and can be:

- `zero`
- `fixed` (fixed amount per fill)

## Transaction Costs

Transaction costs model non-commission fees and can be:

- `fixed_bps` (bps on notional)
- `per_share`
- `per_order`
- `tiered` (bps tiers by notional)

## Market Impact

Market impact models price movement caused by trade size:

- `fixed_bps` adds a constant bps penalty.
- `order_book` uses depth to cap impact.

See `guide/execution-models.md` for configuration examples.
