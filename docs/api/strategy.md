# Package: `regimeflow::strategy`

## Summary

Strategy framework and strategy registry. Provides strategy interfaces, context access, strategy manager, and built-in example strategies.

Related diagrams:
- [Strategy Selection](../strategy-selection.md)
- [Execution Flow](../execution-flow.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/strategy/context.h` | Strategy context (market data, portfolio, regime). |
| `regimeflow/strategy/strategy.h` | Base strategy interface. |
| `regimeflow/strategy/strategy_factory.h` | Factory for strategy construction. |
| `regimeflow/strategy/strategy_manager.h` | Strategy lifecycle orchestration. |
| `regimeflow/strategy/strategies/buy_and_hold.h` | Built-in buy-and-hold example. |
| `regimeflow/strategy/strategies/moving_average_cross.h` | Built-in MA cross example. |

## Type Index

| Type | Description |
| --- | --- |
| `Strategy` | Base strategy interface with `on_bar`/`on_tick`. |
| `StrategyContext` | Read-only view of engine state. |
| `StrategyManager` | Coordinates strategy callbacks and order submission. |
| `StrategyFactory` | Maps registry names to strategy constructors. |
| `BuyAndHoldStrategy` | Example baseline strategy. |
| `MovingAverageCrossStrategy` | Simple signal strategy. |
| `HarmonicPatternStrategy` | Multi-pattern harmonic strategy with smart routing. |
| `PairsTradingStrategy` | Mean-reversion pairs strategy with z-score spread. |

## Lifecycle & Usage Notes

- Strategies should be deterministic and avoid side effects outside the context.
- Regime-aware strategies can query `StrategyContext::regime_state()` for decisions.

## Type Details

### `Strategy`

Base strategy interface. Strategies implement lifecycle hooks and signal generation.

Key APIs:
- `on_start()` / `on_stop()`
- `on_tick()` / `on_bar()`

Methods:

| Method | Description |
| --- | --- |
| `set_context(ctx)` | Attach a strategy context. |
| `context()` | Access the current context. |
| `initialize(ctx)` | Initialize with context (required). |
| `on_start()` | Lifecycle start hook. |
| `on_stop()` | Lifecycle stop hook. |
| `on_bar(bar)` | Handle bar event. |
| `on_tick(tick)` | Handle tick event. |
| `on_quote(quote)` | Handle quote event. |
| `on_order_book(book)` | Handle order book event. |
| `on_order_update(order)` | Handle order updates. |
| `on_fill(fill)` | Handle fill updates. |
| `on_regime_change(transition)` | Handle regime changes. |
| `on_end_of_day(date)` | Handle end-of-day marker. |
| `on_timer(timer_id)` | Handle timer callback. |

### `StrategyContext`

Read-only access to portfolio, market data, and regime state. Exposes helper queries for signals.

Methods:

| Method | Description |
| --- | --- |
| `StrategyContext(...)` | Construct strategy context. |
| `submit_order(order)` | Submit an order. |
| `cancel_order(id)` | Cancel an order by ID. |
| `config()` | Access strategy config. |
| `get_as<T>(key)` | Fetch typed config value. |
| `portfolio()` | Access portfolio (mutable). |
| `portfolio() const` | Access portfolio (const). |
| `latest_bar(symbol)` | Latest bar for symbol. |
| `latest_tick(symbol)` | Latest tick for symbol. |
| `latest_quote(symbol)` | Latest quote for symbol. |
| `recent_bars(symbol, count)` | Recent bars for symbol. |
| `latest_order_book(symbol)` | Latest order book for symbol. |
| `current_regime()` | Current regime state. |
| `schedule_timer(id, interval)` | Schedule recurring timer. |
| `cancel_timer(id)` | Cancel timer. |
| `current_time()` | Current simulated time. |
### `StrategyManager`

Coordinates strategy initialization and dispatches market events to strategies.

Methods:

| Method | Description |
| --- | --- |
| `add_strategy(strategy)` | Add a strategy instance. |
| `clear()` | Remove all strategies. |
| `initialize(ctx)` | Initialize all strategies. |
| `start()` | Start all strategies. |
| `stop()` | Stop all strategies. |
| `on_bar(bar)` | Dispatch bar event. |
| `on_tick(tick)` | Dispatch tick event. |
| `on_quote(quote)` | Dispatch quote event. |
| `on_order_book(book)` | Dispatch order book event. |
| `on_order_update(order)` | Dispatch order update. |
| `on_fill(fill)` | Dispatch fill update. |
| `on_regime_change(transition)` | Dispatch regime change. |
| `on_timer(timer_id)` | Dispatch timer event. |
### `StrategyFactory`

Resolves strategy names to constructors, including built-in registry names.

Methods:

| Method | Description |
| --- | --- |
| `instance()` | Access singleton factory. |
| `register_creator(name, creator)` | Register a strategy creator. |
| `create(config)` | Create a strategy from config. |
| `register_builtin_strategies()` | Register built-in strategies. |
### `BuyAndHoldStrategy`

Baseline buy-and-hold strategy for reference and smoke tests.

Methods:

| Method | Description |
| --- | --- |
| `initialize(ctx)` | Initialize with context. |
| `on_bar(bar)` | Enter long position on first bar. |
### `MovingAverageCrossStrategy`

Simple moving-average crossover signal strategy.

Methods:

| Method | Description |
| --- | --- |
| `initialize(ctx)` | Initialize with context. |
| `on_bar(bar)` | Generate MA-cross signals. |

### `HarmonicPatternStrategy`

Harmonic pattern strategy supporting Gartley, Bat, Butterfly, Crab, and Cypher patterns.
Includes a simple smart routing layer that selects market vs. limit orders based on volatility.

Key params:
- `symbol` (string)
- `pivot_threshold_pct` (double)
- `tolerance` (double)
- `min_bars` (int)
- `cooldown_bars` (int)
- `use_limit` (bool)
- `limit_offset_bps` (double)
- `vol_threshold_pct` (double)
- `min_confidence` (double)
- `min_qty_scale` (double)
- `max_qty_scale` (double)
- `aggressive_confidence_threshold` (double)
- `venue_switch_confidence` (double)
- `passive_venue_weight` (double)
- `aggressive_venue_weight` (double)
- `allow_short` (bool)
- `order_qty` (int)

Methods:
| Method | Description |
| --- | --- |
| `initialize(ctx)` | Load config and initialize symbol. |
| `on_bar(bar)` | Detect patterns and route orders. |

### `PairsTradingStrategy`

Pairs trading strategy based on z-score of the spread between two symbols.

Key params:
- `symbol_a` (string)
- `symbol_b` (string)
- `lookback` (int)
- `entry_z` (double)
- `exit_z` (double)
- `max_z` (double)
- `allow_short` (bool)
- `base_qty` (int)
- `min_qty_scale` (double)
- `max_qty_scale` (double)
- `cooldown_bars` (int)

Methods:
| Method | Description |
| --- | --- |
| `initialize(ctx)` | Load symbols and parameters. |
| `on_bar(bar)` | Compute z-score and place spread trades. |

## Method Details

Type Hints:

- `bar` → `data::Bar`
- `tick` → `data::Tick`
- `quote` → `data::Quote`
- `book` → `data::OrderBook`
- `order` → `engine::Order`
- `fill` → `engine::Fill`

### `Strategy`

#### `initialize(ctx)`
Parameters: strategy context.
Returns: `void`.
Throws: None.

#### `on_bar/on_tick/on_quote/on_order_book`
Parameters: market data.
Returns: `void`.
Throws: None.

#### `on_order_update/on_fill/on_regime_change/on_timer`
Parameters: updates and events.
Returns: `void`.
Throws: None.

### `StrategyContext`

#### `submit_order(order)` / `cancel_order(id)`
Parameters: order or ID.
Returns: `Result<OrderId>` / `Result<void>`.
Throws: `Error::InvalidState` when the context lacks an order manager, `Error::InvalidArgument` for invalid order fields, `Error::NotFound` when canceling a missing order, `Error::InvalidState` when canceling a non-open order. Pre-submit hooks may return additional error codes.

#### `latest_bar/tick/quote/order_book`
Parameters: symbol.
Returns: Optional data.
Throws: None.

#### `current_regime()` / `current_time()`
Parameters: None.
Returns: regime state / timestamp.
Throws: None.

### `StrategyManager`

#### `initialize(ctx)` / `start()` / `stop()`
Parameters: context.
Returns: `void`.
Throws: None.

#### `on_bar/on_tick/on_quote/on_order_book/on_order_update/on_fill/on_regime_change/on_timer`
Parameters: events.
Returns: `void`.
Throws: None.

### `StrategyFactory`

#### `register_creator(name, creator)` / `create(config)`
Parameters: name, creator/config.
Returns: strategy instance.
Throws: None. `create()` returns `nullptr` when the name is unregistered.

## Usage Examples

```cpp
#include "regimeflow/strategy/strategy_factory.h"

regimeflow::Config cfg;
cfg.set("name", "moving_average_cross");
auto strat = regimeflow::strategy::StrategyFactory::instance().create(cfg);
```

## See Also

- [Strategy Selection](../strategy-selection.md)
- [Execution Flow](../execution-flow.md)
