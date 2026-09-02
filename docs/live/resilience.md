# Live Resilience

The live engine includes health checks, reconnect logic, and order throttling. These behaviors are controlled by `LiveConfig` and the CLI mappings.

## Reconnect Backoff

Config keys:

- `live.reconnect.enabled`.
- `live.reconnect.initial_ms`.
- `live.reconnect.max_ms`.

## Heartbeat Monitoring

Config keys:

- `live.heartbeat.enabled`.
- `live.heartbeat.interval_ms`.
- `live.heartbeat.disable_trading_on_timeout`.
- `live.heartbeat.cancel_orders_on_timeout`.

The engine reports heartbeat status in the live CLI loop and logs stale conditions.
By default, stale market data is fail-closed: trading is disabled and open live
orders are cancelled when the heartbeat timeout expires.

## Order Rate Limits

`LiveConfig` supports:

- `max_orders_per_minute`.
- `max_orders_per_second`.

If `max_orders_per_second` is `0`, broker limits are used when available.

## Duplicate Order Guard

Set `live.duplicate_order_window_ms` to reject identical live orders emitted inside a short window before they reach the broker adapter. The fingerprint includes symbol, side, order type, time-in-force, quantity, limit/stop prices, strategy ID, and optional `client_order_id` / `idempotency_key` metadata.

This guard is disabled by default to avoid surprising research strategies, but production live configs should usually set a small window such as `250` to `1000` milliseconds.

## Callback And Shutdown Safety

Live broker adapters may deliver market data and execution reports on transport-owned threads.
`LiveTradingEngine` therefore completes account recovery, position reconciliation, and strategy
initialization before it registers callbacks or starts its worker threads. Portfolio mutation is
serialized, and regime-model inference and retraining share a dedicated model lock.

`LiveOrderManager` serializes its order maps, duplicate-order guard, execution-quality tracker,
and callback registration. Broker calls and user callbacks run after that lock is released, so
application callbacks must not assume they execute under an engine lock.

`EventBus::publish()` remains fire-and-forget for compatibility. New code that needs explicit
shutdown behavior should use `EventBus::try_publish()`: it returns `false` after `stop()` begins.
`stop()` closes publisher admission, waits for admitted publishers, then drains the internally
synchronized FIFO queue before joining the dispatcher. Lifecycle transitions are serialized, so a
concurrent `start()` cannot reopen admission while shutdown is in progress. RegimeFlow deliberately
uses mutex-backed queues here: safe reclamation is more important than an unsafe lock-free linked
list on a live trading control path.
The owner must still stop broker/adaptor threads before destroying the bus; no member function can
make calls through an object after its lifetime safe.

## Interactive Brokers Transport

The IB TWS/Gateway API uses a native plaintext socket. `IBAdapter` is therefore loopback-only by
default and requires an explicit `allow_plaintext_remote: true` opt-in for another host. That flag
is not encryption; deploy a tunnel, VPN, or TLS-terminating proxy around the network path before
enabling it. This follows the upstream connection model while preventing accidental exposure of a
broker control socket on a LAN or public network.

## Market-Data Validation

Order-book consumers must not treat an empty or crossed top of book as a price. Use
`OrderBook::best_bid()`, `OrderBook::best_ask()`, and `OrderBook::has_usable_top_of_book()`.
The live engine only marks the portfolio from a usable, non-crossed book. The feature extractor
returns zero-valued features without changing rolling history for an invalid book snapshot, and
the HMM preserves its previous probability distribution.

## Dry-Run Order Mode

Set `live.dry_run: true` to run strategy, routing, risk, broker normalization,
rate-limit, and audit paths without submitting orders to the broker. Dry-run
orders are logged as `DryRunOrder` audit events and then cancelled internally so
dashboards do not show them as open broker orders.

## Reconciliation

`LiveConfig` supports:

- `order_reconcile_interval`.
- `position_reconcile_interval`.
- `account_refresh_interval`.
- `live.reconciliation.disable_trading_on_error`.

These intervals control how frequently the live engine reconciles broker state. By default, an order-reconciliation error is fail-closed: trading is disabled, open orders are cancelled through the live order manager, and the audit log records `trading_disabled=true`.
