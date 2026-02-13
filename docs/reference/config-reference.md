# Configuration Reference

This page is the **complete, field-by-field configuration reference** for RegimeFlow. It covers both configuration
styles used in the project.

1. **C++ engine config** (nested `engine`, `data`, `strategy`, `risk`, `execution`, `regime`, `plugins`, `live`).
2. **Python `BacktestConfig` YAML** (flat keys used by the Python CLI and bindings).

Each section below includes field types, defaults, valid values, and how the field is consumed by the system.

## C++ Engine Config Layout

The C++ engine uses a nested YAML config loaded by `YamlConfigLoader`. A minimal example:

```yaml
engine:
  initial_capital: 100000
  currency: "USD"
  audit_log_path: "logs/backtest_audit.json"

plugins:
  search_paths:
    - examples/plugins/custom_regime/build
  load:
    - libcustom_regime_detector.so

data:
  type: csv
  data_directory: examples/backtest_basic/data
  file_pattern: "{symbol}.csv"
  has_header: true

symbols:
  - AAPL
  - MSFT

strategy:
  name: moving_average_cross
  params:
    fast_period: 10
    slow_period: 30
    quantity: 5

regime:
  detector: hmm
  hmm:
    states: 4
    window: 20
    normalize_features: true
    normalization: zscore

execution:
  model: basic
  slippage:
    type: fixed_bps
    bps: 1.0
  commission:
    type: fixed
    amount: 0.005

risk:
  limits:
    max_drawdown: 0.2
    max_position_pct: 0.1
  stop_loss:
    enable: true
    pct: 0.05
```

## C++ Engine Config Fields

### Engine

| Key | Type | Default | Description | Used By |
| --- | --- | --- | --- | --- |
| `engine.initial_capital` | number | `0.0` | Starting portfolio value for backtests. | `EngineFactory::create` |
| `engine.currency` | string | `"USD"` | Base currency for reports and portfolio accounting. | `EngineFactory::create` |
| `engine.audit_log_path` | string | empty | If set, writes audit log events to this path. | `BacktestEngine::set_audit_log_path` |

### Plugins

| Key | Type | Default | Description | Used By |
| --- | --- | --- | --- | --- |
| `plugins.search_paths` | array of strings | empty | Directories to scan for plugins. | `EngineFactory::create` |
| `plugins.load` | array of strings | empty | Explicit plugin filenames or paths. Relative entries are resolved against `plugins.search_paths`. | `EngineFactory::create` |

### Strategy (C++ registry and plugins)

| Key | Type | Default | Description | Used By |
| --- | --- | --- | --- | --- |
| `strategy.name` | string | empty | Built-in or registered strategy name. Uses registry if matched; otherwise plugin lookup. | `StrategyFactory::create` |
| `strategy.type` | string | empty | Alias for `strategy.name`. | `StrategyFactory::create` |
| `strategy.params` | object | empty | Arbitrary strategy parameters passed into strategy config. | `StrategyFactory::create` |

Built-in strategy names: `buy_and_hold`, `moving_average_cross`, `pairs_trading`, `harmonic_pattern`.

#### Strategy Params: `buy_and_hold`

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `symbol` | string | empty | Optional fixed symbol to buy. If empty, first bar symbol is used. |
| `quantity` | number | `1.0` | Quantity to buy. If `<= 0`, defaults to 1 when no position exists. |

#### Strategy Params: `moving_average_cross`

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `fast_period` | integer | `10` | Window for fast SMA. |
| `slow_period` | integer | `30` | Window for slow SMA. Must be greater than `fast_period`. |
| `quantity` | number | `1.0` | Order size when crossing. |

#### Strategy Params: `pairs_trading`

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `symbol_a` | string | empty | First symbol in the pair. Required for live/backtest. |
| `symbol_b` | string | empty | Second symbol in the pair. Required for live/backtest. |
| `lookback` | integer | `120` | Window for spread and z-score computation. |
| `entry_z` | number | `2.0` | Z-score threshold to enter a spread trade. |
| `exit_z` | number | `0.5` | Z-score threshold to exit. |
| `max_z` | number | `4.0` | Hard cap to avoid extreme sizing. |
| `allow_short` | boolean | `true` | Allow short legs in the spread. |
| `base_qty` | integer | `10` | Base order quantity per leg. |
| `min_qty_scale` | number | `0.5` | Minimum scaling factor for `base_qty`. |
| `max_qty_scale` | number | `2.0` | Maximum scaling factor for `base_qty`. |
| `cooldown_bars` | integer | `5` | Cooldown bars after a signal before another trade. |

#### Strategy Params: `harmonic_pattern`

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `symbol` | string | empty | Symbol to trade. If empty, first bar symbol is used. |
| `pivot_threshold_pct` | number | `0.03` | Minimum swing threshold for pivot detection. |
| `tolerance` | number | `0.1` | Pattern ratio tolerance band. |
| `min_bars` | integer | `120` | Minimum bars before pattern evaluation. |
| `cooldown_bars` | integer | `5` | Cooldown bars after a signal. |
| `use_limit` | boolean | `true` | Allow limit orders when volatility is low. |
| `limit_offset_bps` | number | `2.0` | Limit price offset in basis points. |
| `vol_threshold_pct` | number | `0.01` | Volatility threshold for choosing limit orders. |
| `min_confidence` | number | `0.45` | Minimum pattern confidence to trade. |
| `min_qty_scale` | number | `0.5` | Minimum quantity scaling. |
| `max_qty_scale` | number | `1.5` | Maximum quantity scaling. |
| `aggressive_confidence_threshold` | number | `0.7` | Confidence level to use aggressive routing. |
| `venue_switch_confidence` | number | `0.6` | Confidence threshold for venue selection. |
| `passive_venue_weight` | number | `0.7` | Weight for passive venue routing. |
| `aggressive_venue_weight` | number | `0.3` | Weight for aggressive venue routing. |
| `allow_short` | boolean | `false` | Allow short positions. |
| `order_qty` | integer | `10` | Base order quantity. |

### Regime Detection

| Key | Type | Default | Description | Used By |
| --- | --- | --- | --- | --- |
| `regime.detector` | string | `"constant"` | Detector type. Supported: `constant`, `hmm`, `ensemble`, or plugin name. | `RegimeFactory::create_detector` |
| `regime.type` | string | `"constant"` | Alias for `regime.detector`. | `RegimeFactory::create_detector` |
| `regime.regime` | string | `"neutral"` | For `constant` detector only. Supported: `bull`, `neutral`, `bear`, `crisis`. | `RegimeFactory::create_detector` |
| `regime.params` | object | empty | Plugin parameters for custom detectors. | `RegimeFactory::create_detector` |

#### HMM Configuration (`regime.detector: hmm`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `regime.hmm.states` | integer | `4` | Number of hidden states. |
| `regime.hmm.window` | integer | `20` | Feature window length. |
| `regime.hmm.features` | array of strings | empty | Feature list. Supported: `return`, `volatility`, `volume`, `log_return`, `volume_zscore`, `range`, `range_zscore`, `volume_ratio`, `volatility_ratio`, `obv`, `up_down_volume_ratio`, `bid_ask_spread`, `spread_zscore`, `order_imbalance`. |
| `regime.hmm.normalize_features` | boolean | `false` | Normalize features before HMM. |
| `regime.hmm.normalization` | string | `"none"` | Normalization mode: `none`, `zscore`, `minmax`, `robust`. |
| `regime.hmm.kalman_enabled` | boolean | `false` | Enable Kalman smoothing of regime probabilities. |
| `regime.hmm.kalman_process_noise` | number | `1e-3` | Kalman process noise. |
| `regime.hmm.kalman_measurement_noise` | number | `1e-2` | Kalman measurement noise. |
| `regime.hmm.state{i}.mean_return` | number | `0.0` | Initial mean return for state `i`. |
| `regime.hmm.state{i}.mean_vol` | number | `0.01` | Initial mean volatility for state `i`. |
| `regime.hmm.state{i}.var_return` | number | `1e-6` | Initial return variance for state `i`. |
| `regime.hmm.state{i}.var_vol` | number | `1e-4` | Initial volatility variance for state `i`. |

#### Ensemble Configuration (`regime.detector: ensemble`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `regime.ensemble.voting_method` | string | `"weighted_average"` | Voting mode: `weighted_average`, `majority`, `confidence_weighted`, `bayesian`. |
| `regime.ensemble.detectors` | array of objects | empty | List of detector configs (each entry can be `constant`, `hmm`, or plugin). |
| `regime.ensemble.detectors[].weight` | number | `1.0` | Weight for that detector in the ensemble. |

### Execution Models

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `execution.model` | string | `"basic"` | Execution model type. `basic` or plugin name. |
| `execution.type` | string | empty | Alias for `execution.model`. |
| `execution.params` | object | empty | Plugin parameters for custom execution models. |

#### Slippage

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `execution.slippage.type` | string | `"zero"` | `zero`, `fixed_bps`, `regime_bps`, or plugin. |
| `execution.slippage.bps` | number | `0.0` | Fixed slippage in bps for `fixed_bps`. |
| `execution.slippage.default_bps` | number | `0.0` | Default bps for `regime_bps`. |
| `execution.slippage.regime_bps.bull` | number | `default_bps` | Regime-specific bps. |
| `execution.slippage.regime_bps.neutral` | number | `default_bps` | Regime-specific bps. |
| `execution.slippage.regime_bps.bear` | number | `default_bps` | Regime-specific bps. |
| `execution.slippage.regime_bps.crisis` | number | `default_bps` | Regime-specific bps. |

#### Commission

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `execution.commission.type` | string | `"zero"` | `zero`, `fixed`, or plugin. |
| `execution.commission.amount` | number | `0.0` | Fixed commission per fill. |

#### Transaction Cost

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `execution.transaction_cost.type` | string | `"zero"` | `zero`, `fixed_bps`, `per_share`, `per_order`, `tiered`. |
| `execution.transaction_cost.bps` | number | `0.0` | Fixed bps for `fixed_bps`. |
| `execution.transaction_cost.per_share` | number | `0.0` | Per-share cost for `per_share`. |
| `execution.transaction_cost.per_order` | number | `0.0` | Per-order cost for `per_order`. |
| `execution.transaction_cost.tiers` | array of objects | empty | Tier list for `tiered` (each entry has `bps` and optional `max_notional`). |

#### Market Impact

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `execution.market_impact.type` | string | `"zero"` | `zero`, `fixed_bps`, `order_book`. |
| `execution.market_impact.bps` | number | `0.0` | Fixed impact bps for `fixed_bps`. |
| `execution.market_impact.max_bps` | number | `50.0` | Cap for `order_book` impact. |

#### Latency

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `execution.latency.ms` | integer | `0` | Fixed execution latency in milliseconds. |

### Risk Manager

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `risk.type` | string | empty | Plugin risk manager name. If omitted, built-in limits are applied. |
| `risk.params` | object | empty | Plugin-specific config if `risk.type` is set. |

#### Risk Limits

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `risk.limits.max_notional` | number | none | Max total notional exposure. |
| `risk.limits.max_position` | number | none | Max position size per symbol. |
| `risk.limits.max_position_pct` | number | none | Max position as fraction of equity. |
| `risk.limits.max_drawdown` | number | none | Max allowed drawdown (fraction). |
| `risk.limits.max_gross_exposure` | number | none | Gross exposure limit. |
| `risk.limits.max_net_exposure` | number | none | Net exposure limit. |
| `risk.limits.max_leverage` | number | none | Max leverage. |
| `risk.limits.sector_limits` | object | empty | Map: sector name -> max exposure. |
| `risk.limits.sector_map` | object | empty | Map: symbol -> sector. |
| `risk.limits.industry_limits` | object | empty | Map: industry name -> max exposure. |
| `risk.limits.industry_map` | object | empty | Map: symbol -> industry. |
| `risk.limits.correlation.window` | integer | `0` | Rolling window size for correlation. |
| `risk.limits.correlation.max_corr` | number | `0.0` | Maximum absolute correlation allowed. |
| `risk.limits.correlation.max_pair_exposure_pct` | number | `0.0` | Max exposure per correlated pair. |

#### Regime-Specific Limits

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `risk.limits_by_regime` | object | empty | Map of regime name -> nested `risk` config. Each entry supports the same limit keys as `risk.limits.*`. |

#### Stop Loss Configuration

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `risk.stop_loss.enable` | boolean | `false` | Enable fixed percentage stop-loss. |
| `risk.stop_loss.pct` | number | `0.05` | Stop-loss percent. |
| `risk.trailing_stop.enable` | boolean | `false` | Enable trailing stop-loss. |
| `risk.trailing_stop.pct` | number | `0.03` | Trailing stop percent. |
| `risk.atr_stop.enable` | boolean | `false` | Enable ATR-based stop-loss. |
| `risk.atr_stop.window` | integer | `14` | ATR lookback window. |
| `risk.atr_stop.multiplier` | number | `2.0` | ATR multiplier. |
| `risk.time_stop.enable` | boolean | `false` | Enable time-based stop. |
| `risk.time_stop.max_holding_seconds` | integer | `0` | Max holding time in seconds. `0` disables. |

### Data Sources

`data.type` selects the data source. All types also support symbol metadata overlays via
`symbol_metadata_csv`, `symbol_metadata_delimiter`, `symbol_metadata_has_header`, and
`symbol_metadata`.

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.type` | string | `"csv"` | One of `csv`, `tick_csv`, `memory`, `mmap`, `mmap_ticks`, `mmap_books`, `api`, `alpaca`, `database` (or plugin name). |
| `data.params` | object | empty | Plugin parameters for custom data sources. |

#### CSV Bars (`data.type: csv`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.data_directory` | string | empty | Root directory for CSV data. |
| `data.file_pattern` | string | `"{symbol}.csv"` | Filename pattern. `{symbol}` is substituted. |
| `data.actions_directory` | string | empty | Corporate actions directory. |
| `data.actions_file_pattern` | string | `"{symbol}_actions.csv"` | Corporate actions filename pattern. |
| `data.date_format` | string | `"%Y-%m-%d"` | Date format for daily bars. |
| `data.datetime_format` | string | `"%Y-%m-%d %H:%M:%S"` | Date-time format for intraday bars. |
| `data.actions_date_format` | string | `"%Y-%m-%d"` | Date format for corporate actions. |
| `data.actions_datetime_format` | string | `"%Y-%m-%d %H:%M:%S"` | Date-time format for corporate actions. |
| `data.delimiter` | string | `","` | CSV delimiter. Only first char is used. |
| `data.has_header` | boolean | `true` | CSV header row present. |
| `data.collect_validation_report` | boolean | `false` | Collects validation report per load. |
| `data.allow_symbol_column` | boolean | `false` | Allow a symbol column in each row. |
| `data.symbol_column` | string | `"symbol"` | Column name if `allow_symbol_column` is true. |
| `data.utc_offset_seconds` | integer | `0` | Offset applied to parsed timestamps. |
| `data.fill_missing_bars` | boolean | `false` | Fill missing bars when possible. |

#### CSV Ticks (`data.type: tick_csv`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.data_directory` | string | empty | Root directory for tick CSVs. |
| `data.file_pattern` | string | `"{symbol}_ticks.csv"` | Filename pattern. |
| `data.datetime_format` | string | `"%Y-%m-%d %H:%M:%S"` | Timestamp format. |
| `data.delimiter` | string | `","` | CSV delimiter. Only first char is used. |
| `data.has_header` | boolean | `true` | CSV header row present. |
| `data.collect_validation_report` | boolean | `false` | Collects validation report per load. |
| `data.utc_offset_seconds` | integer | `0` | Offset applied to parsed timestamps. |

#### Memory/MMap Sources

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.data_directory` | string | empty | Directory of memory-mapped files. |
| `data.preload_index` | boolean | `false` | Preload index on startup (mmap). |
| `data.max_cached_files` | integer | `0` | Max files cached (0 = unlimited). |
| `data.max_cached_ranges` | integer | `0` | Max ranges cached (0 = unlimited). |

#### API Data Source (`data.type: api`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.base_url` | string | empty | Base API URL. |
| `data.bars_endpoint` | string | empty | Bars endpoint path. |
| `data.ticks_endpoint` | string | empty | Ticks endpoint path. |
| `data.api_key` | string | empty | API key. |
| `data.api_key_header` | string | empty | Header name for API key. |
| `data.format` | string | empty | Response format if required by API. |
| `data.time_format` | string | empty | Timestamp format. |
| `data.timeout_seconds` | integer | `0` | Request timeout seconds. |
| `data.collect_validation_report` | boolean | `false` | Collect validation report. |
| `data.fill_missing_bars` | boolean | `false` | Fill missing bars. |
| `data.symbols` | string | empty | Comma-separated symbols to fetch. |

#### Alpaca Data Source (`data.type: alpaca`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.api_key` | string | empty | Alpaca API key. |
| `data.secret_key` | string | empty | Alpaca API secret. |
| `data.trading_base_url` | string | empty | Alpaca trading base URL. |
| `data.data_base_url` | string | empty | Alpaca market data base URL. |
| `data.timeout_seconds` | integer | `0` | Request timeout seconds. |
| `data.symbols` | string | empty | Comma-separated symbols to fetch. |

#### Database Data Source (`data.type: database` or `db`)

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.connection_string` | string | empty | Database connection string. |
| `data.bars_table` | string | empty | Bars table name. |
| `data.ticks_table` | string | empty | Ticks table name. |
| `data.actions_table` | string | empty | Corporate actions table name. |
| `data.order_books_table` | string | empty | Order book table name. |
| `data.symbols_table` | string | empty | Symbols table name. |
| `data.connection_pool_size` | integer | `0` | DB pool size. |
| `data.bars_has_bar_type` | boolean | `false` | If true, bars table includes bar type column. |
| `data.collect_validation_report` | boolean | `false` | Collect validation report. |
| `data.fill_missing_bars` | boolean | `false` | Fill missing bars. |

### Data Validation

Validation fields apply to CSV, API, and DB sources that parse or validate bar/tick data.

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data.validation.require_monotonic_timestamps` | boolean | `true` | Enforce non-decreasing timestamps. |
| `data.validation.check_price_bounds` | boolean | `true` | Check that prices are within bounds. |
| `data.validation.check_gap` | boolean | `false` | Enable gap checks. |
| `data.validation.max_gap_seconds` | integer | `172800` | Max gap size in seconds (default 2 days). |
| `data.validation.check_price_jump` | boolean | `false` | Enable price jump checks. |
| `data.validation.max_jump_pct` | number | `0.2` | Max jump size (fraction). |
| `data.validation.check_future_timestamps` | boolean | `false` | Reject timestamps in the future. |
| `data.validation.max_future_skew_seconds` | integer | `0` | Allowed skew in seconds. |
| `data.validation.check_trading_hours` | boolean | `false` | Enforce trading hours. |
| `data.validation.trading_start_seconds` | integer | `0` | Trading day start (sec since midnight). |
| `data.validation.trading_end_seconds` | integer | `86400` | Trading day end (sec since midnight). |
| `data.validation.check_volume_bounds` | boolean | `false` | Enforce volume bounds. |
| `data.validation.max_volume` | integer | `0` | Max volume. `0` disables. |
| `data.validation.max_price` | number | `0.0` | Max price. `0` disables. |
| `data.validation.check_outliers` | boolean | `false` | Enable outlier checks. |
| `data.validation.outlier_zscore` | number | `5.0` | Z-score threshold. |
| `data.validation.outlier_warmup` | integer | `30` | Warmup rows before outlier checks. |
| `data.validation.on_error` | string | `"fail"` | Action on error: `fail`, `skip`, `fill`, `continue`. |
| `data.validation.on_gap` | string | `"fill"` | Action on gap. |
| `data.validation.on_warning` | string | `"continue"` | Action on warning. |

### Symbol Metadata

Symbol metadata can be loaded from CSV and/or inline config. The overlay order is:
source defaults -> CSV -> config (config overrides CSV).

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `symbol_metadata_csv` | string | empty | CSV file path with symbol metadata. |
| `symbol_metadata_delimiter` | string | `","` | CSV delimiter. |
| `symbol_metadata_has_header` | boolean | `true` | CSV header row present. |
| `symbol_metadata` | object | empty | Inline metadata map. Keys are tickers. |

Inline `symbol_metadata` fields support: `exchange`, `asset_class`, `currency`, `tick_size`, `lot_size`,
`multiplier`, `sector`, `industry`.

### Live Trading Config

Live configs are read by `regimeflow-live` (C++). Values are under `live.*` and `strategy.*`.

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `live.broker` | string | required | Broker adapter name, for example `alpaca`. |
| `live.symbols` | array of strings | empty | Symbols to trade. |
| `live.paper` | boolean | `true` | Paper trading mode. |
| `live.risk` | object | empty | Risk config for live engine. |
| `live.reconnect.enabled` | boolean | `true` | Enable auto reconnect. |
| `live.reconnect.initial_ms` | integer | `1000` | Initial backoff in ms. |
| `live.reconnect.max_ms` | integer | `30000` | Max backoff in ms. |
| `live.heartbeat.enabled` | boolean | `false` | Enable heartbeat monitoring. |
| `live.heartbeat.interval_ms` | integer | `0` | Heartbeat timeout in ms. |
| `live.broker_config` | object | empty | Broker-specific string map. |

Alpaca environment variables are also supported when fields are missing:
`ALPACA_API_KEY`, `ALPACA_API_SECRET`, `ALPACA_PAPER_BASE_URL`.

## Python BacktestConfig YAML

The Python CLI (`regimeflow-backtest`) uses `BacktestConfig.from_yaml`. This YAML is **flat** and maps
directly to the fields below.

If you want a machine-readable schema, see [BacktestConfig JSON Schema](config-schema.json).
You can validate configs with any JSON Schema validator. Example (using `ajv`):

```bash
npx -y ajv-cli validate -s docs/reference/config-schema.json -d your_config.json
```

Python example (using `jsonschema`):

```bash
.venv/bin/python - <<'PY'
import json
from jsonschema import validate

with open("docs/reference/config-schema.json", "r", encoding="utf-8") as f:
    schema = json.load(f)

with open("your_config.json", "r", encoding="utf-8") as f:
    config = json.load(f)

validate(instance=config, schema=schema)
print("Config is valid.")
PY
```

### BacktestConfig Fields

| Key | Type | Default | Description |
| --- | --- | --- | --- |
| `data_source` | string | `"csv"` | Data source type (`csv`, `tick_csv`, `alpaca`, `api`, `database`, or plugin). |
| `data` | object | empty | Data source configuration (same keys as `data.*` in C++). |
| `symbols` | array of strings | empty | Symbols to trade. |
| `start_date` | string | empty | Backtest start date (`YYYY-MM-DD`). |
| `end_date` | string | empty | Backtest end date (`YYYY-MM-DD`). |
| `bar_type` | string | `"1d"` | Bar type (e.g., `1d`, `1h`, `1m`). |
| `initial_capital` | number | `1000000.0` | Starting capital. |
| `currency` | string | `"USD"` | Base currency. |
| `regime_detector` | string | `"hmm"` | Regime detector (`hmm`, `ensemble`, `constant`, or plugin). |
| `regime_params` | object | empty | Regime detector parameters. |
| `plugins_search_paths` | array of strings | empty | Plugin search paths. |
| `plugins_load` | array of strings | empty | Explicit plugin filenames or paths. |
| `slippage_model` | string | `"zero"` | Slippage model (`zero`, `fixed_bps`, `regime_bps`, or plugin). |
| `slippage_params` | object | empty | Slippage params (same keys as `execution.slippage.*`). |
| `commission_model` | string | `"zero"` | Commission model (`zero`, `fixed`, or plugin). |
| `commission_params` | object | empty | Commission params (same keys as `execution.commission.*`). |
| `risk_params` | object | empty | Risk params (same keys as `risk.limits.*` and `risk.stop_loss.*`). |
| `strategy_params` | object | empty | Strategy params for built-in or Python strategies. |

### BacktestConfig Example

```yaml
data_source: csv
data:
  data_directory: examples/backtest_basic/data
  file_pattern: "{symbol}.csv"
  has_header: true

symbols:
  - AAPL
  - MSFT

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
  max_position_pct: 0.1

strategy_params:
  fast_period: 10
  slow_period: 30
  quantity: 5
```
