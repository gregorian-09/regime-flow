# Live Order Reconciliation (Detailed)

This section describes how RegimeFlow keeps internal order state aligned with broker reality.

## Reconciliation Sequence

```mermaid
sequenceDiagram
  participant LiveEngine
  participant BrokerAdapter
  participant LiveOrderManager
  participant Portfolio

  LiveEngine->>BrokerAdapter: get_open_orders()
  BrokerAdapter-->>LiveEngine: open orders list
  LiveEngine->>LiveOrderManager: reconcile_with_broker()
  LiveOrderManager->>LiveOrderManager: compare + update
  LiveOrderManager-->>Portfolio: apply fills/updates
```


## What It Means

- The engine periodically asks the broker for open orders.
- It compares broker state with internal state.
- Differences are resolved so the system always reflects reality.


## Interpretation

Interpretation: reconciliation compares broker truth to internal state and resolves mismatches.

