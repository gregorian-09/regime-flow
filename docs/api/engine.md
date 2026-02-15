# Package: `regimeflow::engine`

## Summary

Backtest and live engine core. Orchestrates event generation, event loop processing, execution pipelines, order management, portfolio state, and regime tracking. This package is the central runtime for both historical and live trading modes.

Related diagrams:
- [Execution Flow](../explanation/execution-flow.md)
- [Backtest Sequence](../diagrams/sequence-backtest.md)
- [Live Sequence](../diagrams/sequence-live.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/engine/audit_log.h` | Audit log interface for backtest and live runs. |
| `regimeflow/engine/backtest_engine.h` | Backtest engine coordinator. |
| `regimeflow/engine/backtest_results.h` | Backtest result container and summaries. |
| `regimeflow/engine/backtest_runner.h` | High-level runner for repeated backtests. |
| `regimeflow/engine/engine_factory.h` | Engine factory and dependency wiring. |
| `regimeflow/engine/event_generator.h` | Generates events from data sources. |
| `regimeflow/engine/event_loop.h` | Main event loop and dispatch. |
| `regimeflow/engine/execution_pipeline.h` | Execution pipeline orchestration. |
| `regimeflow/engine/market_data_cache.h` | Cache for recent market data. |
| `regimeflow/engine/order.h` | Order model and helpers. |
| `regimeflow/engine/order_book_cache.h` | In-memory order book cache. |
| `regimeflow/engine/order_manager.h` | Order lifecycle management. |
| `regimeflow/engine/portfolio.h` | Portfolio state and accounting. |
| `regimeflow/engine/regime_tracker.h` | Regime state tracking and transitions. |
| `regimeflow/engine/timer_service.h` | Scheduled callbacks and timers. |

## Type Index

| Type | Description |
| --- | --- |
| `BacktestEngine` | Orchestrates backtest data ingestion and strategy execution. |
| `BacktestRunner` | Repeats backtests for parameter sweeps. |
| `EventLoop` | Single-threaded event processing pipeline. |
| `ExecutionPipeline` | Bridges strategies to execution models. |
| `OrderManager` | Validates, routes, and tracks orders. |
| `Portfolio` | PnL, positions, and cash accounting. |
| `RegimeTracker` | Tracks regime signals and transition stats. |
| `TimerService` | Timers for heartbeat, sampling, and periodic tasks. |

## Lifecycle & Usage Notes

- `EngineFactory` should be the only constructor of `BacktestEngine` or `LiveEngine` to ensure consistent wiring.
- `EventGenerator` drives the `EventLoop` with market data events.
- The `ExecutionPipeline` is the hand-off between strategy decisions and execution models.
- `AuditLog` is used for both live and backtest runs to guarantee traceability.

## Type Details

### `BacktestEngine`

Coordinates historical data playback, strategy execution, and metric collection for backtests.

Key APIs:
- `run()` to execute a full backtest
- accessors for `Portfolio`, `MetricsTracker`, and `AuditLog`

Methods:

| Method | Description |
| --- | --- |
| `BacktestEngine(initial_capital, currency)` | Construct the engine with starting capital and base currency. |
| `event_queue()` | Access the internal event queue. |
| `dispatcher()` | Access the event dispatcher. |
| `event_loop()` | Access the event loop. |
| `order_manager()` | Access the order manager. |
| `portfolio()` | Access the portfolio. |
| `market_data()` | Access the market data cache. |
| `enqueue(event)` | Enqueue a raw event. |
| `load_data(iterator)` | Load a single data iterator. |
| `load_data(bar, tick, book)` | Load bar, tick, and order book iterators. |
| `set_strategy(strategy, config)` | Set primary strategy with config. |
| `add_strategy(strategy)` | Add an additional strategy. |
| `set_execution_model(model)` | Set execution model. |
| `set_commission_model(model)` | Set commission model. |
| `set_transaction_cost_model(model)` | Set transaction cost model. |
| `set_market_impact_model(model)` | Set market impact model. |
| `set_latency_model(model)` | Set latency model. |
| `set_regime_detector(detector)` | Set regime detector implementation. |
| `risk_manager()` | Access risk manager. |
| `metrics()` | Access metrics tracker. |
| `current_regime()` | Access current regime state. |
| `current_time()` | Access current simulated time. |
| `hooks()` | Access hook system. |
| `hook_manager()` | Access hook manager. |
| `configure_execution(config)` | Configure execution models from config. |
| `configure_risk(config)` | Configure risk controls from config. |
| `configure_regime(config)` | Configure regime detection from config. |
| `set_parallel_context(context)` | Configure parallel run context. |
| `run_parallel(param_sets, factory, num_threads)` | Run parameter sweeps in parallel. |
| `register_hook(type, hook, priority)` | Register hook callback with priority. |
| `on_progress(callback)` | Register progress callback. |
| `set_audit_log_path(path)` | Enable audit logging to file. |
| `results()` | Return results after run completion. |
| `step()` | Advance event loop by one event. |
| `run_until(end_time)` | Run until a stop time. |
| `run()` | Run until exhaustion. |
| `stop()` | Stop the engine. |

Method Details:

Type Hints:

- `event` → `events::Event`
- `iterator` → `std::unique_ptr<data::DataIterator>`
- `bar_iterator` → `std::unique_ptr<data::DataIterator>`
- `tick_iterator` → `std::unique_ptr<data::TickIterator>`
- `book_iterator` → `std::unique_ptr<data::OrderBookIterator>`
- `strategy` → `std::unique_ptr<strategy::Strategy>`
- `model` → `std::unique_ptr<execution::ExecutionModel>` (or related model type)
- `detector` → `std::unique_ptr<regime::RegimeDetector>`
- `config` → `Config`
- `context` → `ParallelContext`
- `end_time` → `Timestamp`

#### `BacktestEngine(initial_capital, currency)`
Parameters: `initial_capital` starting cash; `currency` base currency code.
Returns: Instance.
Throws: None.

#### `event_queue()`
Parameters: None.
Returns: Reference to internal event queue.
Throws: None.

#### `dispatcher()`
Parameters: None.
Returns: Reference to event dispatcher.
Throws: None.

#### `event_loop()`
Parameters: None.
Returns: Reference to event loop.
Throws: None.

#### `order_manager()`
Parameters: None.
Returns: Reference to order manager.
Throws: None.

#### `portfolio()`
Parameters: None.
Returns: Reference to portfolio.
Throws: None.

#### `market_data()`
Parameters: None.
Returns: Reference to market data cache.
Throws: None.

#### `enqueue(event)`
Parameters: `event` raw event to enqueue.
Returns: `void`.
Throws: None.

#### `load_data(iterator)`
Parameters: `iterator` bar or tick iterator.
Returns: `void`.
Throws: None.

#### `load_data(bar_iterator, tick_iterator, book_iterator)`
Parameters: `bar_iterator` bar iterator; `tick_iterator` tick iterator; `book_iterator` order book iterator.
Returns: `void`.
Throws: None.

#### `set_strategy(strategy, config)`
Parameters: `strategy` primary strategy instance; `config` strategy config.
Returns: `void`.
Throws: None.

#### `add_strategy(strategy)`
Parameters: `strategy` additional strategy instance.
Returns: `void`.
Throws: None.

#### `set_execution_model(model)`
Parameters: `model` execution model.
Returns: `void`.
Throws: None.

#### `set_commission_model(model)`
Parameters: `model` commission model.
Returns: `void`.
Throws: None.

#### `set_transaction_cost_model(model)`
Parameters: `model` transaction cost model.
Returns: `void`.
Throws: None.

#### `set_market_impact_model(model)`
Parameters: `model` market impact model.
Returns: `void`.
Throws: None.

#### `set_latency_model(model)`
Parameters: `model` latency model.
Returns: `void`.
Throws: None.

#### `set_regime_detector(detector)`
Parameters: `detector` regime detector implementation.
Returns: `void`.
Throws: None.

#### `risk_manager()`
Parameters: None.
Returns: Reference to risk manager.
Throws: None.

#### `metrics()`
Parameters: None.
Returns: Reference to metrics tracker.
Throws: None.

#### `current_regime()`
Parameters: None.
Returns: Current regime state.
Throws: None.

#### `current_time()`
Parameters: None.
Returns: Current simulated time.
Throws: None.

#### `hooks()`
Parameters: None.
Returns: Reference to hook system.
Throws: None.

#### `hook_manager()`
Parameters: None.
Returns: Reference to hook manager.
Throws: None.

#### `configure_execution(config)`
Parameters: `config` execution configuration.
Returns: `void`.
Throws: `Error::ConfigError` on invalid config.

#### `configure_risk(config)`
Parameters: `config` risk configuration.
Returns: `void`.
Throws: `Error::ConfigError` on invalid config.

#### `configure_regime(config)`
Parameters: `config` regime configuration.
Returns: `void`.
Throws: `Error::ConfigError` on invalid config.

#### `set_parallel_context(context)`
Parameters: `context` parallel run context.
Returns: `void`.
Throws: None.

#### `run_parallel(param_sets, factory, num_threads)`
Parameters: `param_sets` parameter maps; `factory` strategy factory; `num_threads` worker count.
Returns: Vector of `BacktestResults`.
Throws: None.

#### `register_hook(type, hook, priority)`
Parameters: `type` hook type; `hook` callback; `priority` ordering priority.
Returns: `void`.
Throws: None.

#### `on_progress(callback)`
Parameters: `callback` progress callback.
Returns: `void`.
Throws: None.

#### `set_audit_log_path(path)`
Parameters: `path` output file path.
Returns: `void`.
Throws: `Error::IoError` on open failure (deferred).

#### `results()`
Parameters: None.
Returns: Snapshot `BacktestResults`.
Throws: None.

#### `step()`
Parameters: None.
Returns: `bool` indicating more events remain.
Throws: None.

#### `run_until(end_time)`
Parameters: `end_time` stop timestamp.
Returns: `void`.
Throws: None.

#### `run()`
Parameters: None.
Returns: `void`.
Throws: None.

#### `stop()`
Parameters: None.
Returns: `void`.
Throws: None.

### `BacktestRunner`

Executes multiple backtests (parameter sweeps, walk-forward trials) and aggregates results.

Methods:

| Method | Description |
| --- | --- |
| `BacktestRunner(engine, data_source)` | Construct a runner bound to engine and data source. |
| `run(strategy, range, symbols, bar_type)` | Execute a single backtest. |
| `run_parallel(runs, num_threads)` | Execute multiple runs in parallel. |

Method Details:

#### `BacktestRunner(engine, data_source)`
Parameters: `engine` backtest engine; `data_source` data source.
Returns: Instance.
Throws: None.

#### `run(strategy, range, symbols, bar_type)`
Parameters: `strategy` strategy instance; `range` time range; `symbols` to trade; `bar_type` aggregation.
Returns: `BacktestResults`.
Throws: None.

#### `run_parallel(runs, num_threads)`
Parameters: `runs` run specs; `num_threads` worker count.
Returns: Vector of `BacktestResults`.
Throws: None.

### `EventLoop`

Single-threaded event processing core. Dispatches market, order, and system events.

Methods:

| Method | Description |
| --- | --- |
| `EventLoop(queue)` | Construct with an event queue. |
| `set_dispatcher(dispatcher)` | Set the event dispatcher. |
| `add_pre_hook(hook)` | Register pre-dispatch hook. |
| `add_post_hook(hook)` | Register post-dispatch hook. |
| `set_progress_callback(callback)` | Register progress callback. |
| `run()` | Run until queue exhaustion or stop. |
| `run_until(end_time)` | Run until a target time. |
| `step()` | Process a single event. |
| `stop()` | Request loop stop. |
| `current_time()` | Get time of last processed event. |

Method Details:

#### `EventLoop(queue)`
Parameters: `queue` event queue.
Returns: Instance.
Throws: None.

#### `set_dispatcher(dispatcher)`
Parameters: `dispatcher` event dispatcher.
Returns: `void`.
Throws: None.

#### `add_pre_hook(hook)`
Parameters: `hook` callback.
Returns: `void`.
Throws: None.

#### `add_post_hook(hook)`
Parameters: `hook` callback.
Returns: `void`.
Throws: None.

#### `set_progress_callback(callback)`
Parameters: `callback` progress callback.
Returns: `void`.
Throws: None.

#### `run()`
Parameters: None.
Returns: `void`.
Throws: None.

#### `run_until(end_time)`
Parameters: `end_time` stop time.
Returns: `void`.
Throws: None.

#### `step()`
Parameters: None.
Returns: `bool` indicating whether an event was processed.
Throws: None.

#### `stop()`
Parameters: None.
Returns: `void`.
Throws: None.

#### `current_time()`
Parameters: None.
Returns: `Timestamp` of last processed event.
Throws: None.

### `EventGenerator`

Turns data iterators into engine events.

Methods:

| Method | Description |
| --- | --- |
| `EventGenerator(iterator, queue[, config])` | Construct from a single data iterator. |
| `EventGenerator(bar, tick, book, queue[, config])` | Construct from multiple iterators. |
| `enqueue_all()` | Enqueue all events from iterators. |

Method Details:

#### `EventGenerator(iterator, queue[, config])`
Parameters: `iterator` data iterator; `queue` destination queue; `config` generator config (optional).
Returns: Instance.
Throws: None.

#### `EventGenerator(bar, tick, book, queue[, config])`
Parameters: `bar` bar iterator; `tick` tick iterator; `book` order book iterator; `queue` destination queue; `config` optional config.
Returns: Instance.
Throws: None.

#### `enqueue_all()`
Parameters: None.
Returns: `void`.
Throws: None.

### `ExecutionPipeline`

Routes orders through the execution model, applying latency, slippage, and commissions.

Methods:

| Method | Description |
| --- | --- |
| `ExecutionPipeline(market_data, order_books, event_queue)` | Construct pipeline. |
| `set_execution_model(model)` | Set execution model. |
| `set_commission_model(model)` | Set commission model. |
| `set_transaction_cost_model(model)` | Set transaction cost model. |
| `set_market_impact_model(model)` | Set market impact model. |
| `set_latency_model(model)` | Set latency model. |
| `on_order_submitted(order)` | Handle order submission and emit fills. |

Method Details:

#### `ExecutionPipeline(market_data, order_books, event_queue)`
Parameters: `market_data` cache; `order_books` cache; `event_queue` event queue.
Returns: Instance.
Throws: None.

#### `set_execution_model(model)`
Parameters: `model` execution model.
Returns: `void`.
Throws: None.

#### `set_commission_model(model)`
Parameters: `model` commission model.
Returns: `void`.
Throws: None.

#### `set_transaction_cost_model(model)`
Parameters: `model` transaction cost model.
Returns: `void`.
Throws: None.

#### `set_market_impact_model(model)`
Parameters: `model` market impact model.
Returns: `void`.
Throws: None.

#### `set_latency_model(model)`
Parameters: `model` latency model.
Returns: `void`.
Throws: None.

#### `on_order_submitted(order)`
Parameters: `order` submitted order.
Returns: `void`.
Throws: None.

### `OrderManager`

Validates, submits, and tracks orders and their lifecycle transitions.

Methods:

| Method | Description |
| --- | --- |
| `submit_order(order)` | Submit a new order. |
| `cancel_order(id)` | Cancel an order by ID. |
| `modify_order(id, mod)` | Modify an existing order. |
| `get_order(id)` | Fetch an order by ID. |
| `get_open_orders()` | Fetch all open orders. |
| `get_open_orders(symbol)` | Fetch open orders for a symbol. |
| `get_orders_by_strategy(strategy_id)` | Fetch orders by strategy. |
| `get_fills(order_id)` | Fetch fills for an order. |
| `get_fills(symbol, range)` | Fetch fills for a symbol in a range. |
| `on_order_update(callback)` | Register order update callback. |
| `on_fill(callback)` | Register fill callback. |
| `on_pre_submit(callback)` | Register pre-submit validation callback. |
| `process_fill(fill)` | Apply a fill to an order. |
| `update_order_status(id, status)` | Update order status. |

Method Details:

#### `submit_order(order)`
Parameters: `order` order to submit.
Returns: `Result<OrderId>`.
Throws: `Error::InvalidArgument` on invalid order, plus any error returned by pre-submit hooks.

#### `cancel_order(id)`
Parameters: `id` order ID.
Returns: `Result<void>`.
Throws: `Error::NotFound` if ID unknown, `Error::InvalidState` if the order is not open.

#### `modify_order(id, mod)`
Parameters: `id` order ID; `mod` modification fields.
Returns: `Result<void>`.
Throws: `Error::NotFound` if ID unknown, `Error::InvalidState` if the order is not open, `Error::InvalidArgument` if the modified order is invalid.

#### `get_order(id)`
Parameters: `id` order ID.
Returns: Optional `Order`.
Throws: None.

#### `get_open_orders()`
Parameters: None.
Returns: Vector of open orders.
Throws: None.

#### `get_open_orders(symbol)`
Parameters: `symbol` symbol ID.
Returns: Vector of open orders.
Throws: None.

#### `get_orders_by_strategy(strategy_id)`
Parameters: `strategy_id` strategy identifier.
Returns: Vector of orders.
Throws: None.

#### `get_fills(order_id)`
Parameters: `order_id` order ID.
Returns: Vector of fills.
Throws: None.

#### `get_fills(symbol, range)`
Parameters: `symbol` symbol ID; `range` time range.
Returns: Vector of fills.
Throws: None.

#### `on_order_update(callback)`
Parameters: `callback` order update callback.
Returns: `void`.
Throws: None.

#### `on_fill(callback)`
Parameters: `callback` fill callback.
Returns: `void`.
Throws: None.

#### `on_pre_submit(callback)`
Parameters: `callback` pre-submit validator.
Returns: `void`.
Throws: None.

#### `process_fill(fill)`
Parameters: `fill` fill record.
Returns: `void`.
Throws: None.

#### `update_order_status(id, status)`
Parameters: `id` order ID; `status` new status.
Returns: `Result<void>`.
Throws: `Error::NotFound` if ID unknown.

### `Portfolio`

Tracks positions, cash, PnL, and exposure. Used by strategies and risk checks.

Methods:

| Method | Description |
| --- | --- |
| `Portfolio(initial_capital, currency)` | Construct a portfolio. |
| `update_position(fill)` | Apply a fill to positions. |
| `mark_to_market(symbol, price, timestamp)` | Mark a symbol to market. |
| `mark_to_market(prices, timestamp)` | Mark multiple symbols. |
| `set_cash(cash, timestamp)` | Set cash balance. |
| `set_position(symbol, quantity, avg_cost, price, timestamp)` | Set a position explicitly. |
| `replace_positions(positions, timestamp)` | Replace all positions. |
| `get_position(symbol)` | Fetch a position for a symbol. |
| `get_all_positions()` | Fetch all positions. |
| `get_held_symbols()` | Get symbols currently held. |
| `cash()` | Current cash balance. |
| `initial_capital()` | Initial capital. |
| `currency()` | Base currency code. |
| `equity()` | Total equity. |
| `gross_exposure()` | Gross exposure. |
| `net_exposure()` | Net exposure. |
| `leverage()` | Portfolio leverage. |
| `total_unrealized_pnl()` | Total unrealized PnL. |
| `total_realized_pnl()` | Total realized PnL. |
| `snapshot()` | Snapshot current state. |
| `equity_curve()` | Get equity curve history. |
| `record_snapshot(timestamp)` | Record a snapshot. |
| `get_fills()` | Get all fills. |
| `get_fills(symbol)` | Get fills for a symbol. |
| `get_fills(range)` | Get fills in a time range. |
| `on_position_change(callback)` | Register position update callback. |
| `on_equity_change(callback)` | Register equity change callback. |

Method Details:

#### `Portfolio(initial_capital, currency)`
Parameters: `initial_capital` starting cash; `currency` base currency.
Returns: Instance.
Throws: None.

#### `update_position(fill)`
Parameters: `fill` execution fill.
Returns: `void`.
Throws: None.

#### `mark_to_market(symbol, price, timestamp)`
Parameters: `symbol` symbol ID; `price` latest price; `timestamp` valuation time.
Returns: `void`.
Throws: None.

#### `mark_to_market(prices, timestamp)`
Parameters: `prices` symbol-price map; `timestamp` valuation time.
Returns: `void`.
Throws: None.

#### `set_cash(cash, timestamp)`
Parameters: `cash` new balance; `timestamp` update time.
Returns: `void`.
Throws: None.

#### `set_position(symbol, quantity, avg_cost, price, timestamp)`
Parameters: `symbol` symbol ID; `quantity` position; `avg_cost` cost basis; `price` price; `timestamp` update time.
Returns: `void`.
Throws: None.

#### `replace_positions(positions, timestamp)`
Parameters: `positions` position map; `timestamp` update time.
Returns: `void`.
Throws: None.

#### `get_position(symbol)`
Parameters: `symbol` symbol ID.
Returns: Optional `Position`.
Throws: None.

#### `get_all_positions()`
Parameters: None.
Returns: Vector of positions.
Throws: None.

#### `get_held_symbols()`
Parameters: None.
Returns: Vector of symbol IDs.
Throws: None.

#### `cash()` / `initial_capital()` / `currency()`
Parameters: None.
Returns: Cash, initial capital, or currency.
Throws: None.

#### `equity()` / `gross_exposure()` / `net_exposure()` / `leverage()`
Parameters: None.
Returns: Equity/exposure/leverage values.
Throws: None.

#### `total_unrealized_pnl()` / `total_realized_pnl()`
Parameters: None.
Returns: PnL values.
Throws: None.

#### `snapshot()`
Parameters: None.
Returns: `PortfolioSnapshot`.
Throws: None.

#### `equity_curve()`
Parameters: None.
Returns: Vector of snapshots.
Throws: None.

#### `record_snapshot(timestamp)`
Parameters: `timestamp` snapshot time.
Returns: `void`.
Throws: None.

#### `get_fills()` / `get_fills(symbol)` / `get_fills(range)`
Parameters: Optional `symbol` or `range`.
Returns: Vector of fills.
Throws: None.

#### `on_position_change(callback)` / `on_equity_change(callback)`
Parameters: callback.
Returns: `void`.
Throws: None.

### `MarketDataCache`

In-memory cache of latest market data and short history.

Methods:

| Method | Description |
| --- | --- |
| `update(bar)` | Update cache with a bar. |
| `update(tick)` | Update cache with a tick. |
| `update(quote)` | Update cache with a quote. |
| `latest_bar(symbol)` | Get latest bar for symbol. |
| `latest_tick(symbol)` | Get latest tick for symbol. |
| `latest_quote(symbol)` | Get latest quote for symbol. |
| `recent_bars(symbol, count)` | Get recent bars for symbol. |

Method Details:

#### `update(bar/tick/quote)`
Parameters: data object.
Returns: `void`.
Throws: None.

#### `latest_bar(symbol)` / `latest_tick(symbol)` / `latest_quote(symbol)`
Parameters: `symbol` symbol ID.
Returns: Optional data.
Throws: None.

#### `recent_bars(symbol, count)`
Parameters: `symbol` symbol ID; `count` number of bars.
Returns: Vector of bars.
Throws: None.

### `OrderBookCache`

In-memory cache of latest order book snapshots.

Methods:

| Method | Description |
| --- | --- |
| `update(book)` | Update cache with a snapshot. |
| `latest(symbol)` | Get latest order book for symbol. |

Method Details:

#### `update(book)`
Parameters: `book` order book snapshot.
Returns: `void`.
Throws: None.

#### `latest(symbol)`
Parameters: `symbol` symbol ID.
Returns: Optional `OrderBook`.
Throws: None.

### `RegimeTracker`

Maintains current regime state and transition statistics used by regime-aware strategies.

Methods:

| Method | Description |
| --- | --- |
| `RegimeTracker(detector)` | Construct with a detector. |
| `on_bar(bar)` | Process bar and emit transition. |
| `on_tick(tick)` | Process tick and emit transition. |
| `current_state()` | Access current regime state. |
| `history()` | Access regime history. |
| `set_history_size(size)` | Set history size limit. |
| `register_transition_callback(callback)` | Register transition callback. |

Method Details:

#### `RegimeTracker(detector)`
Parameters: `detector` regime detector.
Returns: Instance.
Throws: None.

#### `on_bar(bar)` / `on_tick(tick)`
Parameters: `bar` or `tick`.
Returns: Optional `RegimeTransition`.
Throws: None.

#### `current_state()` / `history()`
Parameters: None.
Returns: Current state or history deque.
Throws: None.

#### `set_history_size(size)`
Parameters: `size` max history size.
Returns: `void`.
Throws: None.

#### `register_transition_callback(callback)`
Parameters: callback.
Returns: `void`.
Throws: None.

### `TimerService`

Scheduling utility for periodic tasks such as heartbeat, metrics snapshots, and alerts.

Methods:

| Method | Description |
| --- | --- |
| `TimerService(queue)` | Construct with an event queue. |
| `schedule(id, interval, start)` | Schedule recurring timer. |
| `cancel(id)` | Cancel a timer. |
| `on_time_advance(now)` | Advance timer state. |

Method Details:

#### `TimerService(queue)`
Parameters: `queue` event queue.
Returns: Instance.
Throws: None.

#### `schedule(id, interval, start)`
Parameters: `id` timer ID; `interval` duration; `start` first fire time.
Returns: `void`.
Throws: None.

#### `cancel(id)`
Parameters: `id` timer ID.
Returns: `void`.
Throws: None.

#### `on_time_advance(now)`
Parameters: `now` current time.
Returns: `void`.
Throws: None.

### `AuditLogger`

Structured audit logger for backtest and live runs.

Methods:

| Method | Description |
| --- | --- |
| `AuditLogger(path)` | Construct logger with output path. |
| `log(event)` | Write an audit event. |
| `log_error(error)` | Write an error event. |
| `log_regime_change(transition)` | Write a regime transition event. |

Method Details:

#### `AuditLogger(path)`
Parameters: `path` output file path.
Returns: Instance.
Throws: `Error::IoError` on open failure.

#### `log(event)`
Parameters: `event` audit event.
Returns: `Result<void>`.
Throws: `Error::IoError` on write failure.

#### `log_error(error)`
Parameters: `error` error message.
Returns: `Result<void>`.
Throws: `Error::IoError` on write failure.

#### `log_regime_change(transition)`
Parameters: `transition` regime transition.
Returns: `Result<void>`.
Throws: `Error::IoError` on write failure.

### `BacktestResults`

Aggregated outputs from a backtest run.

Fields:

| Field | Description |
| --- | --- |
| `total_return` | Total return as a fraction. |
| `max_drawdown` | Maximum drawdown as a fraction. |
| `metrics` | Detailed metrics tracker output. |
| `fills` | Fills captured during the run. |
| `regime_history` | Regime states observed during the run (full timeline). |

### `Order` / `Fill`

Order and fill structures used across execution and portfolio.

Methods and Fields:

| Member | Description |
| --- | --- |
| `Order::market(symbol, side, qty)` | Factory for market order. |
| `Order::limit(symbol, side, qty, price)` | Factory for limit order. |
| `Order::stop(symbol, side, qty, stop)` | Factory for stop order. |
| `Order` fields | Order identifiers, sizing, prices, status, timestamps. |
| `Fill` fields | Fill identifiers, price, quantity, timestamps, costs. |

### `OrderSide` / `OrderType` / `TimeInForce` / `OrderStatus`

Order enums used across engine and execution.

### `OrderType`

Order type enum.

### `TimeInForce`

Time-in-force enum.

### `OrderStatus`

Order status enum.

### `Fill`

Fill structure used for execution updates.

### `OrderModification`

Mutable fields for order modification.

Fields:

| Member | Description |
| --- | --- |
| `quantity` | New quantity (optional). |
| `limit_price` | New limit price (optional). |
| `stop_price` | New stop price (optional). |
| `tif` | New time-in-force (optional). |

### `PortfolioSnapshot`

Snapshot of portfolio state.

Fields:

| Member | Description |
| --- | --- |
| `timestamp` | Snapshot time. |
| `cash` / `equity` | Cash and total equity. |
| `gross_exposure` / `net_exposure` | Exposure measures. |
| `leverage` | Portfolio leverage. |
| `positions` | Position map. |

### `BacktestRunSpec`

Specification for a single backtest run.

Fields:

| Member | Description |
| --- | --- |
| `engine_config` | Engine configuration. |
| `data_config` | Data source configuration. |
| `strategy_config` | Strategy configuration. |
| `range` | Backtest range. |
| `symbols` | Symbols to trade. |
| `bar_type` | Bar aggregation type. |

### `ParallelContext`

Context parameters for parallel backtest execution.

Fields:

| Member | Description |
| --- | --- |
| `data_config` | Data source configuration. |
| `range` | Backtest range. |
| `symbols` | Symbols to trade. |
| `bar_type` | Bar aggregation type. |

### `AuditEvent`

Structured audit log event.

Fields:

| Member | Description |
| --- | --- |
| `timestamp` | Event timestamp. |
| `type` | Audit event type. |
| `details` | Event details. |
| `metadata` | Key-value metadata. |

### `AuditEvent::Type`

Audit event categories (order, regime, system, error).

### `EngineFactory`

Builds engines with consistent dependencies (data sources, execution models, risk policies).

Methods:

| Method | Description |
| --- | --- |
| `create(config)` | Create a configured `BacktestEngine`. |

Method Details:

#### `create(config)`
Parameters: `config` root configuration.
Returns: `unique_ptr<BacktestEngine>`.
Throws: `Error::ConfigError` on invalid configuration.

## Usage Examples

```cpp
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/strategy/strategies/buy_and_hold.h"

regimeflow::engine::BacktestEngine engine(1'000'000.0, "USD");
engine.set_strategy(std::make_unique<regimeflow::strategy::BuyAndHoldStrategy>());
engine.run();
auto results = engine.results();
```

## See Also

- [Execution Flow](../explanation/execution-flow.md)
- [Order State Machine](../explanation/order-state-machine.md)
- [Backtest Methodology](../explanation/backtest-methodology.md)
