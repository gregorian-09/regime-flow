# Backtesting

This guide covers how backtests are constructed, how the engine consumes data, and how results are produced.

## Backtest Pipeline

1. **Load config** and build data source, execution, risk, and regime modules.
2. **Create iterators** over bars, ticks, and order books.
3. **Load data** into the engine.
4. **Initialize strategy** with `StrategyContext`.
5. **Run** the event loop and produce fills and portfolio updates.
6. **Emit results** including equity curve and performance metrics.

## Backtest Engine Lifecycle

- `engine::EngineFactory::create` builds a `BacktestEngine` from a config object.
- `BacktestEngine::configure_execution` wires slippage, commission, market impact, and latency.
- `BacktestEngine::configure_risk` sets limits and stop-loss rules.
- `BacktestEngine::configure_regime` constructs the detector from config.
- `BacktestRunner::run` binds a strategy, loads data iterators, and executes the loop.

## Execution Realism

Backtests now support resting-order simulation inside the execution pipeline:

- limit, stop, and stop-limit orders can remain active after submission
- these orders are re-evaluated on later bar, tick, quote, and order-book events
- fills are generated only when later market data crosses the order conditions
- marketable orders still fill immediately when the current market supports execution
- bar-only backtests can now choose `close_only`, `open_only`, or synthetic `intrabar_ohlc` replay

This is now a materially stronger exchange-style replay foundation. The simulator can vary execution timing and executable prices by venue, react to dynamic halt/resume events, and enforce simple calendar closures. It is still not a full exchange matching engine, but execution outcomes are no longer only post-trade analytics layered onto a single venue-neutral path.

### Bar Replay Modes

When the dataset is primarily OHLC bars, `execution.simulation.bar_mode` controls how a bar is exposed to the execution pipeline:

- `close_only` (default): only the bar close is used. This matches the previous behavior.
- `open_only`: only the bar open is used. This is useful for strategies that trade on the next bar open.
- `intrabar_ohlc`: the bar is replayed as a deterministic synthetic path. Bullish and flat bars use `open -> low -> high -> close`; bearish bars use `open -> high -> low -> close`.
- `execution.simulation.tick_mode` chooses whether execution follows synthetic bar replay (`synthetic_ticks`) or prefers real tick/quote/book events (`real_ticks`).
- `execution.simulation.synthetic_tick_profile` chooses the synthetic execution-tick path when bars are driving execution: `bar_close`, `bar_open`, or `ohlc_4tick`.

`ohlc_4tick` is still an approximation, but it lets resting stop and limit orders react to the full bar range instead of only the close. In `real_ticks` mode, once a symbol has seen real tick-like market data, later bars stop driving execution for that symbol and are used only for analytics and mark-to-market.

### Fill Policies And Requotes

Backtests can now apply broker-style execution policy controls through `execution.policy`:

- `fill` sets the default behavior for Day/GTC orders when the order itself does not already request `IOC` or `FOK`.
- `max_deviation_bps` sets the allowed adverse move between the requested price and the eventual executable price.
- `price_drift_action` chooses whether an out-of-band move is ignored, rejected, or treated as a requote.

This is especially useful for stop-triggered orders, where a bar or tick can cross the stop and then gap through the expected entry price.

Latency is now modeled as an actual activation window for execution:

- an order submitted at `t0` with `execution.latency.ms > 0` does not become executable until `t0 + latency`
- price-drift rules compare the requested price at submission with the executable price when the activation window opens

This makes requotes and rejects reflect delayed execution instead of only changing the emitted fill timestamp.

For resting limit orders, `execution.queue.*` can model queue ahead at the touch. That lets the simulator distinguish between:

- maker fills after the order has waited through visible queue
- taker fills when price moves through the limit before the queue clears

`execution.queue.depth_mode` also lets the queue model use only the top visible level, the exact resting price level, or the full visible same-side depth ahead of the order.

The queue model now also supports deeper persistent behavior:

- `execution.queue.aging_fraction` reduces queue ahead when the market moves away from the order, which approximates cancellations ahead while your order keeps resting
- `execution.queue.replenishment_fraction` adds queue back ahead when visible same-price liquidity builds after the order is already resting

This makes the queue state path-dependent instead of only decreasing monotonically on touch events.

### Session-Aware Execution

The execution simulator can now enforce a configurable session window:

- `execution.session.enabled`
- `execution.session.start_hhmm`
- `execution.session.end_hhmm`
- `execution.session.open_auction_minutes`
- `execution.session.close_auction_minutes`
- `execution.session.weekdays`
- `execution.session.closed_dates`
- `execution.session.halt`
- `execution.session.halted_symbols`

When session gating is enabled:

- fills are deferred until the configured session is open
- `MarketOnOpen` orders only fill during the opening auction window
- `MarketOnClose` orders only fill during the closing auction window
- configured holidays and disallowed weekdays block execution while preserving resting orders
- halted symbols remain non-executable until the halt is removed from config or a runtime resume event arrives

Market data, strategy logic, and valuation still continue to run. The gate affects execution eligibility, not data replay.

Venue routing can also push venue-level execution state into the simulator:

- `execution.routing.venues[].price_adjustment_bps` shifts the executable price per venue
- `execution.routing.venues[].latency_ms` overrides the global latency for routed child orders on that venue

This means split child orders can now differ by venue in both timing and execution price, not only in post-fill attribution.

### Account Margin Scaffolding

The backtest portfolio now derives account-level margin state from `execution.account.margin.*` (or `execution.margin.*` when you are already passing the execution block directly):

- `initial_margin_ratio`
- `maintenance_margin_ratio`
- `stop_out_margin_level`

These settings do not force liquidation yet. They currently provide deterministic account state for:

- `Portfolio::margin_snapshot()`
- `PortfolioSnapshot` history
- `engine.portfolio.equity_curve()` in Python, which now includes margin columns such as `buying_power` and `margin_excess`

### Margin Enforcement

The backtest engine now supports configurable enforcement under `execution.account.enforcement.*`:

- `enabled`
- `margin_call_action`: `ignore`, `cancel_open_orders`, or `halt_trading`
- `stop_out_action`: `none`, `liquidate_all`, or `liquidate_worst_first`
- `halt_after_stop_out`

When maintenance margin is breached, the engine can cancel resting orders or halt new order entry. When the stop-out threshold is breached, the engine can:

- flatten every open position immediately, or
- liquidate the worst unrealized PnL positions first and stop once the account is back above the stop-out threshold

This liquidation is deterministic and uses the portfolio’s latest mark price. It is designed for backtest realism and auditability, not exchange-level liquidation-book simulation.

### Financing And Borrow

Daily carry is now applied on trading-day transitions under `execution.account.financing.*`:

- `enabled`
- `long_rate_bps_per_day`
- `short_borrow_bps_per_day`

At the first event of a new trading day, the engine debits or credits cash based on the prior day’s held positions and their latest marked prices. Long financing rates can be positive (cost) or negative (credit). Short borrow rates are typically positive costs.

### Venue Diagnostics

Backtest results now include venue-aware execution diagnostics derived from fill metadata:

- per-venue fill counts
- per-venue split-parent attribution
- commission and transaction-cost decomposition
- maker-fill ratio
- weighted average slippage in basis points

In Python, use:

- `results.venue_fill_summary()`
- `results.account_curve()`
- `results.account_state()`

This makes multi-venue routing analysis available directly from the result object instead of requiring custom post-processing.

For a single runnable example that combines these controls in one place, see `examples/python_execution_realism/`.
For a compiled C++ example focused on calendar closures and runtime halt/resume control, run `./build/bin/regimeflow_backtest_controls_demo` and see `examples/backtest_controls_demo/README.md`.

## Configuration Modes

RegimeFlow supports two config formats:

1. **C++ Engine Config**: nested `engine`, `data`, `strategy`, `risk`, `execution`, `regime`, `plugins`.
2. **Python BacktestConfig**: flat keys used by the Python bindings and CLI.

For both, see `reference/configuration.md`.

## Selecting The Strategy

- Built-in strategies are registered in `strategy::StrategyFactory`.
- Plugins can provide additional strategies.
- Python backtests can use a `module:Class` strategy via `regimeflow.cli`.

Example (built-in):

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy moving_average_cross
```

Example (custom Python):

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy my_strategies:MyStrategy
```

## Backtest Results

`BacktestResults` exposes:

- Equity curve and portfolio snapshots.
- Trades and fills.
- Performance metrics and report exports.

### Performance And Regime Metrics

The report pipeline now uses a consistent metric definition across top-level and regime-aware summaries:

- portfolio Sharpe, Sortino, Calmar, max drawdown, win rate, and profit factor follow the same definitions in both `performance` and `performance_summary`
- intraday annualization is inferred from snapshot timestamps instead of assuming daily bars
- regime return is compounded inside each regime segment
- regime time share is duration-weighted
- transition return is compounded across each transition window before averaging by transition pair

This avoids the older mismatch where intraday runs could annualize differently across report sections, and it prevents regime exposure from being distorted by uneven event spacing.

In Python, you can export these directly:

```python
results = engine.run("moving_average_cross")
results.report_json()
results.report_csv()
results.equity_curve()
results.trades()
```

For native C++ tester surfaces, `BacktestEngine` now also exposes:

- `dashboard_snapshot()`
- `dashboard_snapshot_json()`
- `dashboard_terminal()`

These power terminal dashboards and direct snapshot export without waiting for final `BacktestResults`.

The `regimeflow_strategy_tester` example tool layers on top of these APIs and supports:

- live terminal refresh,
- a split-pane TUI layout,
- tab-specific rendering,
- non-blocking keyboard-driven tab switching,
- file-based JSON snapshot export for external pollers.

The interactive browser dashboard is provided by the Python visualization package. The C++ toolchain provides terminal dashboards, tester-style TUI output, and snapshot export, but not the web dashboard itself.

## Parallel Backtests

`BacktestEngine::run_parallel` can run multiple parameter sets. Use this for grid searches or random sweeps, and combine with walk-forward optimization for robust selection.

See `guide/walkforward.md` for the optimizer flow.

The Python strategy tester dashboard now detects walk-forward results and adds an `Optimization` view automatically. That view includes:

- window-by-window in-sample vs out-of-sample fitness,
- parameter evolution across windows,
- parameter stability scores,
- overfitting diagnostics from the optimizer.
