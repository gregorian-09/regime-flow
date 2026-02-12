# Database

This diagram reflects the Postgres schema used by RegimeFlow.

```mermaid
erDiagram
  SYMBOLS {
    BIGINT id PK
    TEXT symbol
    TEXT exchange
    TEXT asset_class
    TEXT currency
    TIMESTAMPTZ created_at
    TIMESTAMPTZ updated_at
  }

  BARS {
    BIGINT id PK
    BIGINT symbol_id FK
    TIMESTAMPTZ ts
    DOUBLE open
    DOUBLE high
    DOUBLE low
    DOUBLE close
    BIGINT volume
  }

  TICKS {
    BIGINT id PK
    BIGINT symbol_id FK
    TIMESTAMPTZ ts
    DOUBLE price
    DOUBLE quantity
    INTEGER flags
  }

  ORDER_BOOKS {
    BIGINT id PK
    BIGINT symbol_id FK
    TIMESTAMPTZ ts
    DOUBLE bid_price_1
    DOUBLE bid_qty_1
    DOUBLE ask_price_1
    DOUBLE ask_qty_1
  }

  SYMBOLS ||--o{ BARS : has
  SYMBOLS ||--o{ TICKS : has
  SYMBOLS ||--o{ ORDER_BOOKS : has
```



## Interpretation

Interpretation: symbols are the primary entity, and bars/ticks/order books reference them by symbol_id.

