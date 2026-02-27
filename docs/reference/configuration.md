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
