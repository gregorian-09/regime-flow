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

## Reconciliation

`LiveConfig` supports:

- `order_reconcile_interval`.
- `position_reconcile_interval`.
- `account_refresh_interval`.

These intervals control how frequently the live engine reconciles broker state.
