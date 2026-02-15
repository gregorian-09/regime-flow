# Package: `regimeflow::execution`

## Summary

Execution models and cost components. This package encapsulates slippage, commissions, latency, market impact, and order book simulation to turn strategy intent into realistic fills.

Related diagrams:
- [Execution Models](../explanation/execution-models.md)
- [Execution Costs](../explanation/execution-costs.md)
- [Slippage Math](../explanation/slippage-math.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/execution/basic_execution_model.h` | Simple fill model. |
| `regimeflow/execution/commission.h` | Commission models and fees. |
| `regimeflow/execution/execution_factory.h` | Factory for execution models. |
| `regimeflow/execution/execution_model.h` | Base execution model interface. |
| `regimeflow/execution/fill_simulator.h` | Fill simulation logic. |
| `regimeflow/execution/latency_model.h` | Latency model for order execution. |
| `regimeflow/execution/market_impact.h` | Market impact cost model. |
| `regimeflow/execution/order_book_execution_model.h` | Order book simulation model. |
| `regimeflow/execution/slippage.h` | Slippage model. |
| `regimeflow/execution/transaction_cost.h` | Aggregate transaction cost model. |

## Type Index

| Type | Description |
| --- | --- |
| `ExecutionModel` | Strategy-order to fill translation interface. |
| `BasicExecutionModel` | Straightforward fill logic for backtests. |
| `OrderBookExecutionModel` | Uses order book state for fills. |
| `FillSimulator` | Computes fills and partials. |
| `LatencyModel` | Simulates execution delays. |
| `SlippageModel` | Computes slippage against mid/quote. |
| `MarketImpactModel` | Price impact from order size. |
| `TransactionCostModel` | Composition of cost components. |

## Lifecycle & Usage Notes

- `ExecutionFactory` is used by `EngineFactory` to wire the chosen model.
- `TransactionCostModel` integrates with `FillSimulator` to compute realistic net prices.

## Type Details

### `ExecutionModel`

Base interface for converting orders into fills. Implementations may use simplistic or order-book-aware models.

Methods:

| Method | Description |
| --- | --- |
| `execute(order, reference_price, timestamp)` | Execute order and return fills. |

Method Details:

Type Hints:

- `order` → `engine::Order`
- `fill` → `engine::Fill`
- `reference_price` → `Price`
- `timestamp` → `Timestamp`
- `book` → `data::OrderBook*`
- `config` → `Config`

#### `execute(order, reference_price, timestamp)`
Parameters: `order` order to execute; `reference_price` price for modeling; `timestamp` execution time.
Returns: Vector of fills.
Throws: None.

### `BasicExecutionModel`

Simple fill logic with configurable slippage and commissions.

Methods:

| Method | Description |
| --- | --- |
| `BasicExecutionModel(slippage_model)` | Construct with slippage model. |
| `execute(order, reference_price, timestamp)` | Execute with simulated slippage. |

Method Details:

#### `BasicExecutionModel(slippage_model)`
Parameters: `slippage_model` shared slippage model.
Returns: Instance.
Throws: None.

#### `execute(order, reference_price, timestamp)`
Parameters: order, reference price, timestamp.
Returns: Vector of fills.
Throws: None.

### `OrderBookExecutionModel`

Simulates fills against an order book snapshot for more realistic execution.

Methods:

| Method | Description |
| --- | --- |
| `OrderBookExecutionModel(book)` | Construct with order book snapshot. |
| `execute(order, reference_price, timestamp)` | Execute using order book depth. |

Method Details:

#### `OrderBookExecutionModel(book)`
Parameters: `book` shared order book snapshot.
Returns: Instance.
Throws: None.

#### `execute(order, reference_price, timestamp)`
Parameters: order, reference price, timestamp.
Returns: Vector of fills.
Throws: None.

### `FillSimulator`

Applies execution logic to compute fills and partials.

Methods:

| Method | Description |
| --- | --- |
| `FillSimulator(slippage_model)` | Construct with slippage model. |
| `simulate(order, reference_price, timestamp, is_maker)` | Simulate a fill. |

Method Details:

#### `FillSimulator(slippage_model)`
Parameters: `slippage_model` shared slippage model.
Returns: Instance.
Throws: None.

#### `simulate(order, reference_price, timestamp, is_maker)`
Parameters: order, reference price, timestamp, maker flag.
Returns: Simulated `Fill`.
Throws: None.

### `LatencyModel`

Introduces execution delays to emulate broker latency.

Methods:

| Method | Description |
| --- | --- |
| `latency()` | Return latency duration. |

Method Details:

#### `latency()`
Parameters: None.
Returns: `Duration`.
Throws: None.

### `SlippageModel`

Computes slippage against mid/quote prices and spreads.

Methods:

| Method | Description |
| --- | --- |
| `execution_price(order, reference_price)` | Compute execution price. |

Method Details:

#### `execution_price(order, reference_price)`
Parameters: order, reference price.
Returns: Execution price.
Throws: None.

### `MarketImpactModel`

Estimates impact based on order size and liquidity.

Methods:

| Method | Description |
| --- | --- |
| `impact_bps(order, book)` | Compute impact in basis points. |

Method Details:

#### `impact_bps(order, book)`
Parameters: order, optional order book.
Returns: Impact in basis points.
Throws: None.

### `TransactionCostModel`

Composes slippage, commission, and impact into total transaction costs.

Methods:

| Method | Description |
| --- | --- |
| `cost(order, fill)` | Compute transaction cost in currency. |

Method Details:

#### `cost(order, fill)`
Parameters: order, fill.
Returns: Cost in currency.
Throws: None.

### `CommissionModel`

Commission calculator for fills.

Methods:

| Method | Description |
| --- | --- |
| `commission(order, fill)` | Compute commission for a fill. |

Method Details:

#### `commission(order, fill)`
Parameters: order, fill.
Returns: Commission amount.
Throws: None.

### `ZeroCommissionModel` / `FixedPerFillCommissionModel`

Commission model implementations.

### `FixedPerFillCommissionModel`

Fixed commission per fill.

### `ZeroSlippageModel` / `FixedBpsSlippageModel` / `RegimeBpsSlippageModel`

Slippage model implementations.

### `FixedBpsSlippageModel`

Fixed basis-point slippage.

### `RegimeBpsSlippageModel`

Regime-dependent slippage.

### `FixedLatencyModel`

Constant-latency implementation.

### `ZeroMarketImpactModel` / `FixedMarketImpactModel` / `OrderBookImpactModel`

Market impact model implementations.

### `FixedMarketImpactModel`

Fixed impact in basis points.

### `OrderBookImpactModel`

Impact model using order book depth.

### `ZeroTransactionCostModel` / `FixedBpsTransactionCostModel` / `PerShareTransactionCostModel` / `PerOrderTransactionCostModel` / `TieredBpsTransactionCostModel`

Transaction cost model implementations.

### `FixedBpsTransactionCostModel`

Fixed basis-point transaction costs.

### `PerShareTransactionCostModel`

Per-share transaction costs.

### `PerOrderTransactionCostModel`

Per-order transaction costs.

### `TieredBpsTransactionCostModel`

Tiered basis-point transaction costs.

### `TieredTransactionCostTier`

Tier definition for tiered bps transaction costs.

### `ExecutionFactory`

Factory helpers for execution-related models.

Methods:

| Method | Description |
| --- | --- |
| `create_execution_model(config)` | Build execution model from config. |
| `create_slippage_model(config)` | Build slippage model from config. |
| `create_commission_model(config)` | Build commission model from config. |
| `create_transaction_cost_model(config)` | Build transaction cost model from config. |
| `create_market_impact_model(config)` | Build market impact model from config. |
| `create_latency_model(config)` | Build latency model from config. |

Method Details:

#### `create_execution_model(config)`
Parameters: execution config.
Returns: Execution model.
Throws: `Error::ConfigError` on invalid config.

#### `create_slippage_model(config)`
Parameters: execution config.
Returns: Slippage model.
Throws: `Error::ConfigError` on invalid config.

#### `create_commission_model(config)`
Parameters: execution config.
Returns: Commission model.
Throws: `Error::ConfigError` on invalid config.

#### `create_transaction_cost_model(config)`
Parameters: execution config.
Returns: Transaction cost model.
Throws: `Error::ConfigError` on invalid config.

#### `create_market_impact_model(config)`
Parameters: execution config.
Returns: Market impact model.
Throws: `Error::ConfigError` on invalid config.

#### `create_latency_model(config)`
Parameters: execution config.
Returns: Latency model.
Throws: `Error::ConfigError` on invalid config.

## Usage Examples

```cpp
#include "regimeflow/execution/basic_execution_model.h"
#include "regimeflow/execution/slippage.h"

auto slippage = std::make_shared<regimeflow::execution::FixedBpsSlippageModel>(5.0);
regimeflow::execution::BasicExecutionModel model(slippage);
```

## See Also

- [Execution Models](../explanation/execution-models.md)
- [Execution Costs](../explanation/execution-costs.md)
- [Slippage Math](../explanation/slippage-math.md)
