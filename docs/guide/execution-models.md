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
- `execution.transaction_cost.maker_rebate_bps` and `execution.transaction_cost.taker_fee_bps` for `maker_taker`.
- `execution.transaction_cost.tiers` for `tiered` with `bps` and optional `max_notional`.

## Market Impact

Config keys:

- `execution.market_impact.type`: `zero`, `fixed_bps`, `order_book`.
- `execution.market_impact.bps` for `fixed_bps`.
- `execution.market_impact.max_bps` for `order_book`.

## Latency

- `execution.latency.ms` adds a fixed latency model.

## Backtest Simulation

Execution can also control how OHLC bars are replayed during backtests:

- `execution.simulation.bar_mode`: `close_only`, `open_only`, or `intrabar_ohlc`.
- `execution.simulation.tick_mode`: `synthetic_ticks` or `real_ticks`.
- `execution.simulation.synthetic_tick_profile`: `bar_close`, `bar_open`, or `ohlc_4tick`.

`close_only` uses the bar close, `open_only` uses the bar open, and `intrabar_ohlc` replays a deterministic synthetic path so resting orders can react to the bar range even when no tick stream is available.

`synthetic_ticks` turns each bar into an internal execution tick stream. `bar_close` emits one synthetic tick at the close, `bar_open` emits one at the open, and `ohlc_4tick` emits an ordered four-price path. `real_ticks` prefers live tick/quote/order-book events for execution; once a symbol has seen real tick-like data, later bars no longer trigger fills for that symbol.

## Fill Policy And Drift Controls

The execution pipeline can also enforce broker-style fill handling for orders that do not already request `IOC` or `FOK` explicitly.

- `execution.policy.fill`: `preserve`, `return`, `ioc`, or `fok`.
- `execution.policy.max_deviation_bps`: allowed adverse move before a drift rule is applied.
- `execution.policy.price_drift_action`: `ignore`, `reject`, or `requote`.

`return` keeps the residual quantity working, `ioc` cancels any remainder, and `fok` rejects the order if it cannot be completed in full.

When `max_deviation_bps` is set, RegimeFlow compares the current executable price to the order's requested price when the order becomes executable:

- `reject` fails the order when the move exceeds the allowed deviation.
- `requote` emits an order update with the new executable price and waits for the next market event before retrying.

Latency now gates when an order becomes eligible for execution. The execution pipeline records the order's activation time as `created_at + latency`, then evaluates fills, rejects, or requotes only when market data reaches or passes that timestamp.

## Queue Position And Maker/Taker

RegimeFlow can simulate resting queue position for limit orders that become maker orders at the touch.

- `execution.queue.enabled`: enable queue progression.
- `execution.queue.progress_fraction`: fraction of visible queue cleared on each qualifying touch event.
- `execution.queue.default_visible_qty`: fallback displayed size when quotes/books do not expose one.
- `execution.queue.depth_mode`: `top_only`, `price_level`, or `full_depth`.

When enabled:

- a resting limit order that fills exactly at its limit price advances through a simulated queue first
- if the queue is not yet depleted, the order stays live
- once the simulated queue is cleared, the fill is marked as maker
- if the market moves through the limit price before the queue clears, the fill is marked as taker

`top_only` uses only the best visible same-side level, `price_level` uses the resting order's own price level even when it is not at the top of book, and `full_depth` sums better-priced same-side levels plus the order's own price level.

## Smart Order Routing

Smart routing can auto-convert market orders to limit orders when spreads are tight
and tag orders with a default venue. Routing settings live under `execution.routing`.

Config keys:

- `execution.routing.enabled` enables routing.
- `execution.routing.mode`: `smart` or `none`.
- `execution.routing.max_spread_bps`: max spread for auto limit routing.
- `execution.routing.passive_offset_bps`: limit price offset for passive routes.
- `execution.routing.aggressive_offset_bps`: limit price offset for aggressive routes.
- `execution.routing.default_venue`: venue tag applied when none is supplied by the strategy.
- `execution.routing.venues`: optional array of `{name, weight}` entries. The highest weight is used
  as the default venue tag.

Each venue entry can also carry venue-specific microstructure overrides:

- `maker_rebate_bps`
- `taker_fee_bps`
- `price_adjustment_bps`
- `latency_ms`
- `queue_enabled`
- `queue_progress_fraction`
- `queue_default_visible_qty`
- `queue_depth_mode`

These values are attached to routed child orders so split routing can simulate venue-specific queue, fee, latency, and executable-price behavior.

Routing uses the latest order book/quote when available and falls back to the last trade price.

### Split Routing

Split routing fans a parent order into multiple child orders using the venue weights.

Config keys:

- `execution.routing.split.enabled` enables splitting.
- `execution.routing.split.mode`: `parallel` or `sequential`.
- `execution.routing.split.min_child_qty` minimum quantity per child.
- `execution.routing.split.max_children` cap on the number of child orders.
- `execution.routing.split.parent_aggregation`: `none`, `final`, or `partial`.

The parent order acts as a routing container; child orders carry the per-venue quantities.

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
    type: maker_taker
    maker_rebate_bps: 1
    taker_fee_bps: 2
  market_impact:
    type: order_book
    max_bps: 50
  latency:
    ms: 5
  simulation:
    tick_mode: real_ticks
    synthetic_tick_profile: ohlc_4tick
    bar_mode: intrabar_ohlc
  policy:
    fill: return
    max_deviation_bps: 25
    price_drift_action: requote
  queue:
    enabled: true
    progress_fraction: 0.5
    default_visible_qty: 10
    depth_mode: full_depth
  routing:
    enabled: true
    mode: smart
    max_spread_bps: 8
    passive_offset_bps: 0.0
    default_venue: lit_primary
    venues:
      - name: lit_primary
        weight: 0.6
      - name: lit_mid
        weight: 0.4
```
