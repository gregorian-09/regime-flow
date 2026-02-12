# Dashboard Flow

The dashboard aggregates system, portfolio, and regime state.

## Dashboard Pipeline

```mermaid
flowchart LR
  A[Live Engine] --> B[Snapshot Builder]
  B --> C[Dashboard Data]
  C --> D[Dash UI]
  A --> E[System Health]
  E --> B
  A --> F[Alerts]
  F --> B
```


## What It Means

- The dashboard is built from periodic snapshots.
- Health metrics and alerts are included alongside PnL and positions.


## Interpretation

Interpretation: dashboard snapshots merge portfolio state with health and alert signals.

