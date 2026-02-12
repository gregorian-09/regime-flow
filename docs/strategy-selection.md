# Strategy Selection

Strategies can be built‑in, plugin‑based, or Python‑defined.

## Strategy Registry Flow

```mermaid
flowchart LR
  A[Config] --> B[StrategyFactory]
  B --> C{Builtin?}
  C -- Yes --> D[Instantiate Builtin]
  C -- No --> E{Plugin?}
  E -- Yes --> F[Load Plugin]
  E -- No --> G[Python Strategy]
  D --> H[Strategy Manager]
  F --> H
  G --> H
```


## What It Means

- The system reads the config and chooses a strategy.
- You can plug in your own strategies without changing the core.


## Interpretation

Interpretation: strategies can be built‑in, plugin‑based, or provided in Python.

