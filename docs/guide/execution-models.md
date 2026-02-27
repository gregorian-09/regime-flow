# Execution Models

Execution models convert strategy intent into fills while modeling cost and latency. Configuration is under `execution`.

## Execution Model

- `execution.model` or `execution.type` selects a model.
- `basic` is the built-in model and supports slippage, commission, market impact, and latency.
- Plugins can provide custom execution models.

## Slippage

Config keys:

- `execution.slippage.type`: `zero`, `fixed_bps`, `regime_bps`, or plugin name.
- `execution.slippage.bps` for `fixed_bps`.
- `execution.slippage.default_bps` for `regime_bps`.
- `execution.slippage.regime_bps.<regime>` per-regime bps.

## Commission

Config keys:

- `execution.commission.type`: `zero`, `fixed`, or plugin name.
- `execution.commission.amount` for `fixed`.

## Transaction Cost

Config keys:

- `execution.transaction_cost.type`: `zero`, `fixed_bps`, `per_share`, `per_order`, `tiered`.
- `execution.transaction_cost.bps` for `fixed_bps`.
- `execution.transaction_cost.per_share` for `per_share`.
- `execution.transaction_cost.per_order` for `per_order`.
- `execution.transaction_cost.tiers` for `tiered` with `bps` and optional `max_notional`.

## Market Impact

Config keys:

- `execution.market_impact.type`: `zero`, `fixed_bps`, `order_book`.
- `execution.market_impact.bps` for `fixed_bps`.
- `execution.market_impact.max_bps` for `order_book`.

## Latency

- `execution.latency.ms` adds a fixed latency model.

## Time-In-Force Semantics

Time-in-force is configured per order via `Order.tif`:

- `DAY`: orders expire at the first market event of the next calendar day.
- `IOC`: fill immediately and cancel any remaining quantity.
- `FOK`: reject if the order cannot be filled in full.
- `GTD`: requires `expire_at` and cancels when the timestamp is reached.

## Example

```yaml
execution:
  model: basic
  slippage:
    type: fixed_bps
    bps: 1.0
  commission:
    type: fixed
    amount: 0.005
  transaction_cost:
    type: tiered
    tiers:
      - bps: 0.5
        max_notional: 100000
      - bps: 0.3
  market_impact:
    type: order_book
    max_bps: 50
  latency:
    ms: 5
```
