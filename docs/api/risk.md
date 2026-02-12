# Package: `regimeflow::risk`

## Summary

Risk controls and position sizing. Provides centralized risk limits, stop-loss models, and policy factories used by both backtest and live engines.

Related diagrams:
- [Risk Controls](../risk-controls.md)
- [Risk Limits](../risk-limits-deep.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/risk/position_sizer.h` | Position sizing strategies. |
| `regimeflow/risk/risk_factory.h` | Factory for risk policies. |
| `regimeflow/risk/risk_limits.h` | Risk limits and configuration. |
| `regimeflow/risk/stop_loss.h` | Stop-loss policy. |

## Type Index

| Type | Description |
| --- | --- |
| `RiskLimit` | Account-level and symbol-level constraints. |
| `PositionSizer` | Computes position sizes from config and regime. |
| `StopLoss` | Stop-loss policies and state. |
| `RiskFactory` | Creates risk policies by name. |

## Lifecycle & Usage Notes

- `RiskLimits` are enforced at order creation and pre-trade checks.
- `StopLoss` can be evaluated per-tick or per-bar depending on configuration.

## Type Details

### `RiskLimit`

Configurable account, symbol, and portfolio-level constraints enforced before order submission.

Methods:

| Method | Description |
| --- | --- |
| `validate(order, portfolio)` | Validate an order. |
| `validate_portfolio(portfolio)` | Validate portfolio state. |

### `PositionSizer`

Computes position sizes from config, portfolio state, and regime context.

Methods:

| Method | Description |
| --- | --- |
| `size(ctx)` | Compute position size from context. |

### `StopLoss`

Implements stop-loss policies and tracks their state for open positions.

Methods:

| Method | Description |
| --- | --- |
| `configure(config)` | Configure stop-loss settings. |
| `on_position_update(position)` | Update state for position. |
| `on_bar(bar, order_manager)` | Process bar for stops. |
| `on_tick(tick, order_manager)` | Process tick for stops. |

### `StopLossManager`

Manager for stop-loss policies (implementation class).

### `StopState`

Internal per-symbol stop-loss state.

### `RiskFactory`

Factory for constructing risk policies based on configuration names.

Methods:

| Method | Description |
| --- | --- |
| `create_risk_manager(config)` | Build a risk manager from config. |

### `RiskManager`

Aggregates multiple risk limits and validates orders/portfolio.

Methods:

| Method | Description |
| --- | --- |
| `add_limit(limit)` | Add a risk limit. |
| `validate(order, portfolio)` | Validate order against limits. |
| `validate_portfolio(portfolio)` | Validate portfolio state. |
| `set_regime_limits(limits)` | Set regime-specific limit sets. |

### `Limit Classes`

Concrete limit classes implement `validate()` and `validate_portfolio()` from `RiskLimit`. See [Risk Limits](../risk-limits-deep.md) for details on each limit type.

Classes:

| Class | Description |
| --- | --- |
| `MaxNotionalLimit` | Max notional per order. |
| `MaxPositionLimit` | Max absolute position size. |
| `MaxPositionPctLimit` | Max position as % equity. |
| `MaxDrawdownLimit` | Max drawdown limit. |
| `MaxLeverageLimit` | Max leverage limit. |
| `MaxGrossExposureLimit` | Max gross exposure. |
| `MaxNetExposureLimit` | Max net exposure. |
| `MaxSectorExposureLimit` | Max sector exposure. |
| `MaxIndustryExposureLimit` | Max industry exposure. |
| `MaxCorrelationExposureLimit` | Max correlation exposure. |

### `MaxNotionalLimit`

Maximum notional per order.

### `MaxPositionLimit`

Maximum absolute position size.

### `MaxPositionPctLimit`

Maximum position as percent of equity.

### `MaxDrawdownLimit`

Maximum drawdown limit.

### `MaxLeverageLimit`

Maximum leverage limit.

### `MaxGrossExposureLimit`

Maximum gross exposure limit.

### `MaxNetExposureLimit`

Maximum net exposure limit.

### `MaxSectorExposureLimit`

Maximum sector exposure limit.

### `MaxIndustryExposureLimit`

Maximum industry exposure limit.

### `MaxCorrelationExposureLimit`

Maximum correlation exposure limit.

### `StopLossConfig`

Stop-loss configuration settings.

### `PositionSizingContext`

Inputs used for position sizing.

### `Position Sizer Classes`

Concrete sizers implement `size(ctx)`:

| Class | Description |
| --- | --- |
| `FixedFractionalSizer` | Fixed-fractional sizing. |
| `VolatilityTargetSizer` | Volatility-target sizing. |
| `KellySizer` | Kelly criterion sizing. |
| `RegimeScaledSizer` | Regime-scaled wrapper. |

### `FixedFractionalSizer` / `VolatilityTargetSizer` / `KellySizer` / `RegimeScaledSizer`

Concrete position sizing models.

### `VolatilityTargetSizer`

Volatility targeting position sizing model.

### `KellySizer`

Kelly criterion position sizing model.

### `RegimeScaledSizer`

Regime-scaled position sizing wrapper.

## Method Details

Type Hints:

- `order` → `engine::Order`
- `portfolio` → `engine::Portfolio`
- `bar` → `data::Bar`
- `tick` → `data::Tick`
- `ctx` → `PositionSizingContext`

### `RiskLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange`/`Error::InvalidArgument`.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange`.

### `MaxNotionalLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when order notional exceeds the configured limit or portfolio equity.

### `MaxPositionLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when projected absolute quantity exceeds the limit.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when any existing position exceeds the limit.

### `MaxPositionPctLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when projected notional exceeds the configured equity percentage.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when any existing position exceeds the configured equity percentage.

### `MaxDrawdownLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when drawdown exceeds the configured maximum.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when drawdown exceeds the configured maximum.

### `MaxGrossExposureLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when projected gross exposure exceeds the limit.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when gross exposure exceeds the limit.

### `MaxLeverageLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when projected leverage exceeds the limit.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when leverage exceeds the limit.

### `MaxNetExposureLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when projected net exposure exceeds the limit.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when net exposure exceeds the limit.

### `MaxSectorExposureLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when projected sector exposure exceeds the limit.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when sector exposure exceeds the limit.

### `MaxIndustryExposureLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` when a limit order is missing a price, `Error::OutOfRange` when projected industry exposure exceeds the limit.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when industry exposure exceeds the limit.

### `MaxCorrelationExposureLimit`

#### `validate(order, portfolio)`
Parameters: order, portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when pair exposure exceeds correlation limits.

#### `validate_portfolio(portfolio)`
Parameters: portfolio.
Returns: `Result<void>`.
Throws: `Error::OutOfRange` when pair exposure exceeds correlation limits.

### `RiskManager`

#### `add_limit(limit)`
Parameters: risk limit.
Returns: `void`.
Throws: None.

#### `validate(order, portfolio)` / `validate_portfolio(portfolio)`
Parameters: order/portfolio.
Returns: `Result<void>`.
Throws: limit-specific errors.

### `StopLossManager`

#### `configure(config)`
Parameters: stop-loss config.
Returns: `void`.
Throws: None.

#### `on_bar(bar, order_manager)` / `on_tick(tick, order_manager)`
Parameters: market data and order manager.
Returns: `void`.
Throws: None.

### `PositionSizer`

#### `size(ctx)`
Parameters: sizing context.
Returns: `Quantity`.
Throws: None.

## Usage Examples

```cpp
#include "regimeflow/risk/risk_limits.h"

regimeflow::risk::RiskManager manager;
manager.add_limit(std::make_unique<regimeflow::risk::MaxNotionalLimit>(1'000'000.0));
```
## See Also

- [Risk Controls](../risk-controls.md)
- [Risk Limits](../risk-limits-deep.md)
