# Package: `regimeflow::live`

## Summary

Live trading layer. Adapters for broker APIs, live event bus, audit logging, order manager, and broker-specific codecs. Designed for resiliency, schema validation, and reconnection strategies.

Related diagrams:
- [Live Trading](../how-to/live-trading.md)
- [Live Resiliency](../explanation/live-resiliency.md)
- [Broker Streaming](../explanation/broker-streaming.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/live/alpaca_adapter.h` | Alpaca broker adapter. |
| `regimeflow/live/audit_log.h` | Live audit log interface. |
| `regimeflow/live/binance_adapter.h` | Binance adapter. |
| `regimeflow/live/broker_adapter.h` | Broker adapter base interface. |
| `regimeflow/live/event_bus.h` | Live event bus and topic routing. |
| `regimeflow/live/ib_adapter.h` | Interactive Brokers adapter. |
| `regimeflow/live/live_engine.h` | Live engine coordinator. |
| `regimeflow/live/live_order_manager.h` | Live order lifecycle management. |
| `regimeflow/live/mq_adapter.h` | Message bus adapter (Kafka/Redis Streams). |
| `regimeflow/live/mq_codec.h` | Serialization/codec for MQ payloads. |
| `regimeflow/live/types.h` | Shared live types and enums. |

## Type Index

| Type | Description |
| --- | --- |
| `BrokerAdapter` | Abstract broker integration interface. |
| `LiveEngine` | Live runtime orchestrator. |
| `LiveOrderManager` | Tracks broker order state. |
| `EventBus` | Pub/sub for live events. |
| `MQAdapter` | Message bus integration. |
| `AlpacaAdapter`, `IbAdapter`, `BinanceAdapter` | Broker-specific adapters. |

## Lifecycle & Usage Notes

- `LiveEngine` uses `BrokerAdapter` and `LiveOrderManager` to ensure consistent lifecycle.
- The `MQAdapter` is designed for external durability and downstream analytics.
- Adapter implementations must honor the reconnection/backoff policies in `live-resiliency` docs.

## Type Details

### `LiveEngine`

Coordinates live data ingestion, order routing, risk checks, and audit logging.

### `LiveTradingEngine`

Primary live engine class (alias of `LiveEngine` in docs).

Methods:

| Method | Description |
| --- | --- |
| `LiveTradingEngine(config)` | Construct live engine. |
| `LiveTradingEngine(config, broker)` | Construct with injected broker. |
| `~LiveTradingEngine()` | Stop engine and join threads. |
| `start()` | Start engine. |
| `stop()` | Stop engine. |
| `is_running()` | Check running status. |
| `get_status()` | Get engine status snapshot. |
| `get_dashboard_snapshot()` | Get dashboard snapshot. |
| `get_system_health()` | Get system health snapshot. |
| `enable_trading()` | Enable live trading. |
| `disable_trading()` | Disable live trading. |
| `close_all_positions()` | Close all open positions. |
| `on_trade(cb)` | Register trade callback. |
| `on_regime_change(cb)` | Register regime change callback. |
| `on_error(cb)` | Register error callback. |
| `on_dashboard_update(cb)` | Register dashboard callback. |

### `LiveConfig`

Configuration for live trading engine.

### `EngineStatus` / `LiveOrderSummary` / `DashboardSnapshot` / `SystemHealth`

Runtime snapshots for monitoring and dashboards.

### `BrokerAdapter`

Abstract interface for broker connectivity, order placement, and streaming updates.

Methods:

| Method | Description |
| --- | --- |
| `connect()` | Connect to broker. |
| `disconnect()` | Disconnect from broker. |
| `is_connected()` | Connection status. |
| `subscribe_market_data(symbols)` | Subscribe to market data. |
| `unsubscribe_market_data(symbols)` | Unsubscribe from market data. |
| `submit_order(order)` | Submit an order. |
| `cancel_order(broker_order_id)` | Cancel a broker order. |
| `modify_order(broker_order_id, mod)` | Modify an order. |
| `get_account_info()` | Retrieve account info. |
| `get_positions()` | Retrieve positions. |
| `get_open_orders()` | Retrieve open orders. |
| `on_market_data(cb)` | Register market data callback. |
| `on_execution_report(cb)` | Register execution report callback. |
| `on_position_update(cb)` | Register position update callback. |
| `max_orders_per_second()` | Broker order rate limit. |
| `max_messages_per_second()` | Broker message rate limit. |
| `poll()` | Poll for updates if required. |

### `ExecutionReport` / `LiveOrderStatus`

Broker execution report payloads and status enums.

### `LiveOrderStatus`

Live order status enum as reported by brokers.

### `LiveOrderManager`

Tracks broker order state and reconciles against local state.

Methods:

| Method | Description |
| --- | --- |
| `LiveOrderManager(broker)` | Construct with broker adapter. |
| `submit_order(order)` | Submit live order. |
| `cancel_order(id)` | Cancel live order by internal ID. |
| `cancel_all_orders()` | Cancel all open orders. |
| `cancel_orders(symbol)` | Cancel all orders for symbol. |
| `modify_order(id, mod)` | Modify live order. |
| `get_order(id)` | Get live order by internal ID. |
| `get_open_orders()` | Get all open live orders. |
| `get_orders_by_status(status)` | Get orders filtered by status. |
| `on_execution_report(cb)` | Register execution report callback. |
| `on_order_update(cb)` | Register order update callback. |
| `handle_execution_report(report)` | Apply broker execution report. |
| `reconcile_with_broker()` | Reconcile internal state with broker. |
| `validate_order(order)` | Validate live order parameters. |
| `find_order_id_by_broker_id(id)` | Lookup internal ID by broker ID. |

### `LiveOrder`

Internal tracking representation for live orders.

### `EventBus`

Pub/sub routing for live events and system control messages.

Methods:

| Method | Description |
| --- | --- |
| `EventBus()` | Construct event bus. |
| `~EventBus()` | Stop bus on destruction. |
| `start()` | Start dispatch loop. |
| `stop()` | Stop dispatch loop. |
| `subscribe(topic, cb)` | Subscribe to topic. |
| `unsubscribe(id)` | Unsubscribe by ID. |
| `publish(message)` | Publish a message. |

### `LiveTopic` / `LiveMessage`

Live event bus topics and message envelope.

### `LiveMessage`

Event bus message wrapper.

### `MQAdapter`

Integration layer for Kafka/Redis Streams durability and downstream analytics.

### `MessageQueueConfig` / `MessageQueueAdapter` / `LiveMessageCodec`

Message queue configuration, adapter, and codec for streaming.

### `MessageQueueAdapter`

MQ adapter interface for Kafka/Redis Streams.

### `LiveMessageCodec`

Codec for live messages on the MQ.

### `AccountInfo` / `MarketDataUpdate` / `Trade`

Core live types for account state and data updates.

### `MarketDataUpdate`

Market data update payload.

### `Trade`

Trade execution payload.

### `AuditEvent`

Live audit log event payload for compliance logging.

### `AlpacaAdapter` / `IBAdapter` / `BinanceAdapter`

Broker-specific implementations of `BrokerAdapter` with streaming and order lifecycle handling.

### `IBAdapter`

Interactive Brokers adapter.

### `BinanceAdapter`

Binance adapter.

### `DashboardSnapshot` / `LiveOrderSummary` / `SystemHealth`

Dashboard and health snapshot payloads.

### `LiveOrderSummary`

Summary of open live orders for dashboards.

### `SystemHealth`

System health telemetry snapshot.

## Method Details

Type Hints:

- `order` → `engine::Order`
- `mod` → `engine::OrderModification`
- `topic` → `LiveTopic`
- `message` → `LiveMessage`
- `config` → `LiveConfig`

### `LiveTradingEngine`

#### `start()`
Parameters: None.
Returns: `Result<void>`.
Throws: `Error::BrokerError`/`Error::NetworkError` on start failure.

#### `stop()`
Parameters: None.
Returns: `void`.
Throws: None.

#### `enable_trading()` / `disable_trading()`
Parameters: None.
Returns: `void`.
Throws: None.

#### `close_all_positions()`
Parameters: None.
Returns: `void`.
Throws: None.

### `BrokerAdapter`

#### `connect()` / `disconnect()` / `is_connected()`
Parameters: None.
Returns: `Result<void>` or `bool`.
Throws: `Error::NetworkError` on failure.

#### `submit_order(order)`
Parameters: order.
Returns: Broker order ID.
Throws: `Error::BrokerError`.

#### `cancel_order(broker_order_id)` / `modify_order(broker_order_id, mod)`
Parameters: broker order ID, modification.
Returns: `Result<void>`.
Throws: `Error::BrokerError`.

### `LiveOrderManager`

#### `submit_order(order)`
Parameters: order.
Returns: `Result<OrderId>`.
Throws: `Error::InvalidState` when broker adapter is missing, `Error::InvalidArgument` on invalid params, broker-specific errors from `BrokerAdapter::submit_order()`.

#### `cancel_order(id)` / `cancel_all_orders()` / `cancel_orders(symbol)`
Parameters: internal ID or symbol.
Returns: `Result<void>`.
Throws: `Error::InvalidState` when broker adapter is missing, `Error::NotFound` for missing orders, `Error::BrokerError` from the broker adapter.

#### `reconcile_with_broker()`
Parameters: None.
Returns: `Result<void>`.
Throws: `Error::InvalidState` when broker adapter is missing, `Error::BrokerError` from the broker adapter.

### `EventBus`

#### `start()` / `stop()`
Parameters: None.
Returns: `void`.
Throws: None.

#### `subscribe(topic, cb)` / `unsubscribe(id)`
Parameters: topic/callback or subscription ID.
Returns: subscription ID / `void`.
Throws: None.

#### `publish(message)`
Parameters: message.
Returns: `void`.
Throws: None.

## Usage Examples

```cpp
#include "regimeflow/live/live_engine.h"

regimeflow::live::LiveConfig cfg;
cfg.broker_type = "ib";
cfg.symbols = {"AAPL"};
regimeflow::live::LiveTradingEngine engine(cfg);
engine.start();
```

The `regimeflow_live` CLI prints connect/disconnect events and heartbeat status to stdout.

## See Also

- [Live Trading](../how-to/live-trading.md)
- [Live Resiliency](../explanation/live-resiliency.md)
- [Broker Streaming](../explanation/broker-streaming.md)
