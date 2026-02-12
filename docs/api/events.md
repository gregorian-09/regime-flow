# Package: `regimeflow::events`

## Summary

Event model for the engine and live runtime. Defines event types, queues, and dispatch infrastructure used by the event loop and live adapters.

Related diagrams:
- [Event Model](../event-model.md)
- [Order State Machine](../order-state-machine.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/events/dispatcher.h` | Event dispatch interface and helpers. |
| `regimeflow/events/event.h` | Base event type and metadata. |
| `regimeflow/events/event_queue.h` | Event queue implementation. |
| `regimeflow/events/market_event.h` | Market data event types. |
| `regimeflow/events/order_event.h` | Order lifecycle event types. |
| `regimeflow/events/system_event.h` | System lifecycle and control events. |

## Type Index

| Type | Description |
| --- | --- |
| `Event` | Base event interface (timestamp, source, type). |
| `EventQueue` | Lock-free event queue used in the loop. |
| `MarketEvent` | Tick/bar/order book events. |
| `OrderEvent` | Submit, fill, cancel, reject events. |
| `SystemEvent` | Start/stop/heartbeat and control events. |

## Lifecycle & Usage Notes

- `EventQueue` is the backbone for `EventLoop` and must respect producer/consumer constraints.
- `Dispatcher` implementations should avoid blocking; long tasks should be delegated.

## Type Details

### `Event`

Base event interface with timestamp, event type, and origin metadata.

Fields and Helpers:

| Member | Description |
| --- | --- |
| `timestamp` | Event time. |
| `type` | Event category. |
| `priority` | Scheduling priority (lower first). |
| `sequence` | Monotonic sequence for tie-breaks. |
| `symbol` | Associated symbol ID. |
| `payload` | Variant payload for market/order/system data. |
| `default_priority(type)` | Map event type to default priority. |
| `make_market_event(bar/tick/quote/book)` | Build market event from data type. |
| `make_system_event(kind, timestamp, code, id)` | Build system event. |
| `make_order_event(kind, timestamp, order_id, ...)` | Build order event. |

Method Details:

Type Hints:

- `type` → `EventType`
- `kind` → `MarketEventKind` / `OrderEventKind` / `SystemEventKind`
- `timestamp` → `Timestamp`
- `event` → `Event`
- `order_id` → `OrderId`
- `fill_id` → `FillId`
- `symbol` → `SymbolId`

#### `default_priority(type)`
Parameters: `type` event type.
Returns: Default priority.
Throws: None.

#### `make_market_event(bar/tick/quote/book)`
Parameters: Market data object.
Returns: `Event`.
Throws: None.

#### `make_system_event(kind, timestamp, code, id)`
Parameters: `kind` system event kind; `timestamp` event time; `code` optional; `id` optional identifier.
Returns: `Event`.
Throws: None.

#### `make_order_event(kind, timestamp, order_id, fill_id, quantity, price, symbol, commission)`
Parameters: Event kind, time, identifiers, fill details.
Returns: `Event`.
Throws: None.

### `EventType` / `MarketEventKind` / `OrderEventKind` / `SystemEventKind`

Event categories and subtypes.

### `MarketEventKind`

Market event subtype enum.

### `OrderEventKind`

Order event subtype enum.

### `SystemEventKind`

System event subtype enum.

### `MarketEventPayload` / `OrderEventPayload` / `SystemEventPayload`

Payload structures for event data.

### `OrderEventPayload`

Order event payload fields.

### `SystemEventPayload`

System event payload fields.

### `EventComparator`

Priority comparator for deterministic event ordering.

### `MarketEvent`

Represents ticks, bars, and order book updates. Feeds into strategies and indicators.

Alias:

| Alias | Description |
| --- | --- |
| `MarketEvent` | Alias to `MarketEventPayload`. |

### `OrderEvent`

Represents order lifecycle transitions: submit, accept, fill, reject, cancel.

Alias:

| Alias | Description |
| --- | --- |
| `OrderEvent` | Alias to `OrderEventPayload`. |

### `SystemEvent`

Control-plane events such as start, stop, heartbeat, and timer signals.

Alias:

| Alias | Description |
| --- | --- |
| `SystemEvent` | Alias to `SystemEventPayload`. |

### `EventQueue`

Lock-free queue for `Event` objects. Designed for high-throughput dispatch.

Methods:

| Method | Description |
| --- | --- |
| `push(event)` | Enqueue an event. |
| `pop()` | Pop next event by priority. |
| `peek()` | Peek next event without removing. |
| `empty()` | Check if queue is empty. |
| `size()` | Number of queued events. |
| `clear()` | Clear all queued events. |
| `~EventQueue()` | Destructor, clears and releases pool. |

Method Details:

#### `push(event)`
Parameters: `event` to enqueue.
Returns: `void`.
Throws: None.

#### `pop()`
Parameters: None.
Returns: Optional `Event`.
Throws: None.

#### `peek()`
Parameters: None.
Returns: Optional `Event`.
Throws: None.

#### `empty()`
Parameters: None.
Returns: `bool`.
Throws: None.

#### `size()`
Parameters: None.
Returns: `size_t`.
Throws: None.

#### `clear()`
Parameters: None.
Returns: `void`.
Throws: None.

### `Dispatcher`

Dispatches `Event` objects to interested handlers.

Methods:

| Method | Description |
| --- | --- |
| `set_market_handler(handler)` | Set handler for market events. |
| `set_order_handler(handler)` | Set handler for order events. |
| `set_system_handler(handler)` | Set handler for system events. |
| `set_user_handler(handler)` | Set handler for user events. |
| `dispatch(event)` | Dispatch to appropriate handler. |

Method Details:

#### `set_market_handler(handler)`
Parameters: `handler` callback.
Returns: `void`.
Throws: None.

#### `set_order_handler(handler)`
Parameters: `handler` callback.
Returns: `void`.
Throws: None.

#### `set_system_handler(handler)`
Parameters: `handler` callback.
Returns: `void`.
Throws: None.

#### `set_user_handler(handler)`
Parameters: `handler` callback.
Returns: `void`.
Throws: None.

#### `dispatch(event)`
Parameters: `event` to dispatch.
Returns: `void`.
Throws: None.

### `EventDispatcher`

Alias for the dispatcher used by the engine.

## Usage Examples

```cpp
#include "regimeflow/events/event_queue.h"
#include "regimeflow/events/event.h"

regimeflow::events::EventQueue queue;
queue.push(regimeflow::events::make_system_event(
    regimeflow::events::SystemEventKind::DayStart,
    regimeflow::Timestamp::now()));
auto next = queue.pop();
```

## See Also

- [Event Model](../event-model.md)
- [Order State Machine](../order-state-machine.md)
