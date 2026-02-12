# Execution Flow

This document describes how orders are created, validated, executed, and tracked.

## Order Lifecycle (Backtest)

```mermaid
sequenceDiagram
  participant Strategy
  participant OrderManager
  participant ExecutionPipeline
  participant ExecutionModel
  participant Portfolio

  Strategy->>OrderManager: submit_order(order)
  OrderManager->>ExecutionPipeline: enqueue(order)
  ExecutionPipeline->>ExecutionModel: simulate_fill(order)
  ExecutionModel-->>ExecutionPipeline: fills
  ExecutionPipeline->>Portfolio: apply_fills
  Portfolio-->>OrderManager: update status
```


## Order Lifecycle (Live)

```mermaid
sequenceDiagram
  participant Strategy
  participant LiveOrderManager
  participant BrokerAdapter
  participant ExecutionPipeline
  participant Portfolio

  Strategy->>LiveOrderManager: submit_order(order)
  LiveOrderManager->>BrokerAdapter: send order
  BrokerAdapter-->>LiveOrderManager: broker_order_id
  BrokerAdapter-->>LiveOrderManager: execution report
  LiveOrderManager->>ExecutionPipeline: fill update
  ExecutionPipeline->>Portfolio: apply_fills
```


## Execution Model Injection

Execution models are selected via `execution::ExecutionFactory` and can be extended
without changing strategy or engine logic.


## Interpretation

Interpretation: orders move from strategy into the execution pipeline, producing fills that update the portfolio.

