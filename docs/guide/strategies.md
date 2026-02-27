# Strategies

Strategies implement decision logic and are called by the engine through `StrategyContext`. Built-in strategies are registered in `src/strategy/strategies/register_builtin.cpp` and are created through `strategy::StrategyFactory`.

## Strategy Lifecycle

- `initialize(ctx)` is called once at startup.
- `on_start()` and `on_stop()` are called when the engine starts or stops.
- Market callbacks: `on_bar`, `on_tick`, `on_order_book`, `on_quote`.
- Execution callbacks: `on_order_update`, `on_fill`.
- Regime callback: `on_regime_change`.
- Utility callbacks: `on_end_of_day`, `on_timer`.

## Built-In Strategies

### `buy_and_hold`

Parameters:

- `symbol` optional fixed symbol. If empty, the first bar’s symbol is used.
- `quantity` order size. If `<= 0`, defaults to 1 when no position exists.

### `moving_average_cross`

Parameters:

- `fast_period` integer, default 10.
- `slow_period` integer, default 30. If `fast_period >= slow_period`, slow is set to `fast_period + 1`.
- `quantity` order size. If `<= 0`, defaults to 1.

### `pairs_trading`

Parameters:

- `symbol_a` and `symbol_b` required symbols.
- `lookback` integer, minimum 30.
- `entry_z` double, minimum 0.5.
- `exit_z` double, minimum 0.1.
- `max_z` double, minimum `entry_z`.
- `allow_short` boolean.
- `base_qty` integer, minimum 1.
- `min_qty_scale` double, minimum 0.1.
- `max_qty_scale` double, minimum `min_qty_scale`.
- `cooldown_bars` integer, minimum 0.

### `harmonic_pattern`

Parameters:

- `symbol` required symbol.
- `pivot_threshold_pct` minimum 0.001.
- `tolerance` range 0.0 to 0.5.
- `min_bars` integer, minimum 30.
- `cooldown_bars` integer, minimum 0.
- `use_limit` boolean.
- `limit_offset_bps` double, minimum 0.0.
- `vol_threshold_pct` double, minimum 0.0.
- `min_confidence` range 0.0 to 1.0.
- `min_qty_scale` double, minimum 0.1.
- `max_qty_scale` double, minimum `min_qty_scale`.
- `aggressive_confidence_threshold` range 0.0 to 1.0.
- `venue_switch_confidence` range 0.0 to 1.0.
- `passive_venue_weight` range 0.0 to 1.0.
- `aggressive_venue_weight` range 0.0 to 1.0.
- `allow_short` boolean.
- `order_qty` integer, minimum 1.

## Python Strategies

Python strategies must subclass `regimeflow.Strategy` and implement the same callbacks. You can pass the strategy class via `module:Class` in the CLI.

Example:

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy my_strategies:MyStrategy
```

## Next Steps

- `guide/regime-detection.md`
- `guide/risk-management.md`
- `reference/configuration.md`
