# Configuration Reference

This is the authoritative configuration reference mapped to the current codebase. It covers both config formats:

1. **C++ Engine Config** used by `YamlConfigLoader` and `EngineFactory`.
2. **Python BacktestConfig** used by `python/bindings.cpp` and `regimeflow.cli`.

## C++ Engine Config

### Engine

- `engine.initial_capital` double.
- `engine.currency` string.
- `engine.audit_log_path` string.

### Plugins

- `plugins.search_paths` array of plugin directories.
- `plugins.load` array of plugin filenames or paths.

### Data

- `data.type` selects the data source.
- All other keys are passed to `DataSourceFactory`.

See `guide/data-sources.md` for per-type fields.

### Symbols And Range

- `symbols` array of symbol strings.
- `start_date` and `end_date` as `YYYY-MM-DD`.
- `bar_type` such as `1d`, `1h`, `5m`.

### Strategy

- `strategy.name` or `strategy.type` built-in or plugin name.
- `strategy.params` map of strategy parameters.

### Regime

- `regime.detector` or `regime.type`.
- `regime.params` for plugin detectors.
- `regime.hmm.*` for HMM configuration.
- `regime.ensemble.*` for ensemble configuration.

See `guide/regime-detection.md`.

### Execution

- `execution.model` or `execution.type`.
- `execution.slippage.*`.
- `execution.commission.*`.
- `execution.transaction_cost.*`.
- `execution.market_impact.*`.
- `execution.latency.ms`.
- `execution.simulation.bar_mode` for `close_only`, `open_only`, or `intrabar_ohlc`.
- `execution.simulation.tick_mode` for `synthetic_ticks` or `real_ticks`.
- `execution.simulation.synthetic_tick_profile` for `bar_close`, `bar_open`, or `ohlc_4tick`.
- `execution.session.enabled`, `execution.session.start_hhmm`, `execution.session.end_hhmm`.
- `execution.session.open_auction_minutes`, `execution.session.close_auction_minutes`.
- `execution.session.weekdays` to restrict execution to specific UTC weekdays (`sun`..`sat` or `0`..`6`).
- `execution.session.closed_dates` for `YYYY-MM-DD` market closures.
- `execution.session.halt` for a global execution halt.
- `execution.session.halted_symbols` array for symbol-specific halts.
- `execution.policy.fill`, `execution.policy.max_deviation_bps`, `execution.policy.price_drift_action`.
- `execution.queue.enabled`, `execution.queue.progress_fraction`, `execution.queue.default_visible_qty`, `execution.queue.depth_mode`.
- `execution.queue.aging_fraction` to model cancellations ahead while the market moves away.
- `execution.queue.replenishment_fraction` to model new queue rebuilding ahead when visible size increases.
- `execution.routing.*` for smart routing controls.
- `execution.routing.venues[]` may also include `maker_rebate_bps`, `taker_fee_bps`, `price_adjustment_bps`, `latency_ms`, `queue_enabled`, `queue_progress_fraction`, `queue_default_visible_qty`, and `queue_depth_mode` for venue-specific routed child behavior.
- `execution.account.margin.initial_margin_ratio`, `execution.account.margin.maintenance_margin_ratio`, `execution.account.margin.stop_out_margin_level`.
- `execution.margin.*` is also accepted as a shorthand when you are constructing the execution config block directly.
- `execution.account.enforcement.enabled`.
- `execution.account.enforcement.margin_call_action` for `ignore`, `cancel_open_orders`, or `halt_trading`.
- `execution.account.enforcement.stop_out_action` for `none`, `liquidate_all`, or `liquidate_worst_first`.
- `execution.account.enforcement.halt_after_stop_out`.
- `execution.account.financing.enabled`.
- `execution.account.financing.long_rate_bps_per_day`.
- `execution.account.financing.short_borrow_bps_per_day`.
- `execution.enforcement.*` and `execution.financing.*` are also accepted as shorthand when you are constructing the execution config block directly.

See `guide/execution-models.md`.

### Risk

- `risk.limits.*` for standard limits.
- `risk.limits_by_regime.*` for regime-specific limits.
- `risk.stop_loss.*`, `risk.trailing_stop.*`, `risk.atr_stop.*`, `risk.time_stop.*`.

See `guide/risk-management.md`.

### Live

- `live.broker`, `live.paper`, `live.symbols`.
- `live.reconnect.*`, `live.heartbeat.*`.
- `live.risk` block.
- `live.broker_config` map.
- `live.log_dir` for output paths.
- `live.broker_asset_class` for broker TIF support (`equity`, `crypto`).
- `live.routing.*` for live smart routing controls.
- `live.execution.routing.*` is also accepted so live configs can mirror the backtest `execution.routing.*` layout.
- `live.account.margin.*` for internal portfolio margin tracking in the live dashboard snapshot.
- `account.margin.*` can be shared across backtest and live, then overridden by `live.account.margin.*`.

### Live Metrics

- `metrics.live.enable` boolean.
- `metrics.live.baseline_report` path to backtest report JSON.
- `metrics.live.output_dir` directory for `live_drift.csv` and `live_performance.json`.
- `metrics.live.sinks` array, supports `file` and `postgres`.
- `metrics.live.postgres.connection_string`.
- `metrics.live.postgres.table`.
- `metrics.live.postgres.pool_size`.

See `live/config.md`.

## Python BacktestConfig

Python config keys map to `BacktestConfig` in `python/bindings.cpp`:

- `data_source` string.
- `data` object (data source config).
- `symbols` list.
- `start_date`, `end_date`, `bar_type`.
- `initial_capital`, `currency`.
- `regime_detector`, `regime_params`.
- `plugins_search_paths`, `plugins_load` or `plugins.search_paths`, `plugins.load`.
- `execution_model`, `execution_params`.
- `slippage_model`, `slippage_params`.
- `commission_model`, `commission_params`.
- `risk_params`.
- `strategy_params`.

### Python Example

```yaml
data_source: csv
data:
  data_directory: examples/backtest_basic/data
  file_pattern: "{symbol}.csv"
  has_header: true

symbols: ["AAPL", "MSFT"]
start_date: "2019-01-01"
end_date: "2020-12-31"
bar_type: "1d"
initial_capital: 100000.0
currency: "USD"

regime_detector: hmm
regime_params:
  hmm:
    states: 4
    window: 20

slippage_model: fixed_bps
slippage_params:
  bps: 1.0

commission_model: fixed
commission_params:
  amount: 0.005

execution_params:
  simulation:
    tick_mode: real_ticks
    synthetic_tick_profile: ohlc_4tick
    bar_mode: intrabar_ohlc
  session:
    enabled: true
    start_hhmm: "09:30"
    end_hhmm: "16:00"
    open_auction_minutes: 1
    close_auction_minutes: 1
    halted_symbols: ["AAPL"]
  transaction_cost:
    type: maker_taker
    maker_rebate_bps: 1
    taker_fee_bps: 2
  policy:
    fill: return
    max_deviation_bps: 25
    price_drift_action: requote
  queue:
    enabled: true
    progress_fraction: 0.5
    default_visible_qty: 10
    depth_mode: full_depth
    aging_fraction: 0.25
    replenishment_fraction: 0.5
  account:
    margin:
      initial_margin_ratio: 0.5
      maintenance_margin_ratio: 0.25
      stop_out_margin_level: 0.4
    enforcement:
      enabled: true
      margin_call_action: halt_trading
      stop_out_action: liquidate_worst_first
      halt_after_stop_out: true
    financing:
      enabled: true
      long_rate_bps_per_day: 5
      short_borrow_bps_per_day: 12

risk_params:
  limits:
    max_notional: 100000
    max_position_pct: 0.2

strategy_params:
  fast_period: 10
  slow_period: 30
  quantity: 5
```

## Validation Configuration

Validation keys are under `validation.*` for any data source config. See `reference/data-validation.md` for details.
