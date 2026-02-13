# Configuration Cheat Sheet

This page is a **quick, scan-friendly summary** of the most commonly used configuration keys. For full
field-by-field documentation, see [Configuration Reference](config-reference.md).

## Quick Start (C++ Engine Config)

```yaml
engine:
  initial_capital: 100000
  currency: "USD"

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

execution:
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
```

## Quick Start (Python BacktestConfig YAML)

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

## Common Keys (C++ Engine Config)

- `engine.initial_capital`
- `engine.currency`
- `engine.audit_log_path`
- `plugins.search_paths`
- `plugins.load`
- `symbols`
- `data.type`
- `data.data_directory`
- `data.file_pattern`
- `data.has_header`
- `strategy.name`
- `strategy.params`
- `regime.detector`
- `execution.slippage.type`
- `execution.commission.type`
- `risk.limits.*`

## Common Keys (Python BacktestConfig)

- `data_source`
- `data` (object)
- `symbols`
- `start_date`, `end_date`
- `bar_type`
- `initial_capital`, `currency`
- `regime_detector`, `regime_params`
- `plugins_search_paths`, `plugins_load`
- `slippage_model`, `slippage_params`
- `commission_model`, `commission_params`
- `risk_params`
- `strategy_params`

## Live Trading (C++ live config)

```yaml
live:
  broker: alpaca
  symbols: [AAPL, MSFT]
  paper: true
  broker_config:
    api_key: "${ALPACA_API_KEY}"
    secret_key: "${ALPACA_API_SECRET}"
    base_url: "${ALPACA_PAPER_BASE_URL}"
  reconnect:
    enabled: true
    initial_ms: 1000
    max_ms: 30000
  heartbeat:
    enabled: true
    interval_ms: 30000

strategy:
  name: moving_average_cross
  params:
    fast_period: 10
    slow_period: 30

live:
  risk:
    limits:
      max_drawdown: 0.1
```

## Data Validation (any data source that supports validation)

```yaml
data:
  validation:
    check_gap: true
    max_gap_seconds: 3600
    check_price_jump: true
    max_jump_pct: 0.05
    on_gap: fill
    on_error: fail
```

## Symbol Metadata Overlay

```yaml
symbol_metadata_csv: "configs/symbols.csv"

symbol_metadata:
  AAPL:
    exchange: "NASDAQ"
    sector: "Technology"
    industry: "Consumer Electronics"
```

## Related

- Full reference: [Configuration Reference](config-reference.md)
- Python interfaces: [Python Interfaces](python-interfaces.md)
