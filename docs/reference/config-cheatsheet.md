# Configuration Cheat Sheet

This page provides quick, scan-friendly config examples. For full details, see `reference/configuration.md`.

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
  limits:
    max_position_pct: 0.1

strategy_params:
  fast_period: 10
  slow_period: 30
  quantity: 5
```

## Common Keys (C++ Engine Config)

- `engine.initial_capital`
- `engine.currency`
- `plugins.search_paths`
- `plugins.load`
- `data.type`
- `data.data_directory`
- `data.file_pattern`
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
- `start_date`, `end_date`, `bar_type`
- `initial_capital`, `currency`
- `regime_detector`, `regime_params`
- `slippage_model`, `slippage_params`
- `commission_model`, `commission_params`
- `risk_params.limits.*`
- `strategy_params`
