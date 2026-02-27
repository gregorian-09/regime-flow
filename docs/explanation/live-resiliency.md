# Live Resiliency

The live engine monitors connectivity, heartbeat health, and order flow to maintain stable trading sessions.

## Resiliency Diagram

```mermaid
flowchart LR
  A[Broker Adapter] --> B[Live Engine]
  B --> C[Heartbeat Monitor]
  B --> D[Reconnect Backoff]
  B --> E[Order Rate Limits]
  B --> F[Reconciliation]
```

## Key Mechanisms

- **Reconnect backoff** using `live.reconnect.*`.
- **Heartbeat timeout** using `live.heartbeat.*`.
- **Rate limiting** using `max_orders_per_minute` and `max_orders_per_second`.
- **Periodic reconciliation** of orders, positions, and account state.

## Operational Signals

The live CLI prints reconnect attempts and heartbeat status, which helps detect data feed degradation or broker instability.
