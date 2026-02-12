# Symbol Metadata Precedence

RegimeFlow resolves symbol metadata from multiple sources with a strict order.

## Precedence Flow

```mermaid
flowchart LR
  A[Config Map] --> B{Found?}
  B -- Yes --> F[Use Config]
  B -- No --> C[Database]
  C --> D{Found?}
  D -- Yes --> F
  D -- No --> E[CSV Metadata]
  E --> F
```


## What It Means

- If metadata exists in config, it wins.
- Otherwise the DB is used.
- If neither are available, CSV metadata is used.


## Interpretation

Interpretation: metadata resolution is deterministic using config, then DB, then CSV.

