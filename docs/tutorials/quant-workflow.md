# Quantitative Developer Workflow

This workflow is a practical, end-to-end path for quantitative developers. It covers data, regime modeling, strategy design, backtesting, and validation. Use it as the canonical flow for research and production readiness.

## 1. Choose the Data Source

Pick the data source based on scale and latency needs.

Options:
- `mmap` for large historical datasets with fast playback
- `csv` for small experiments
- `alpaca` for live REST pull (bars/trades)
- `database` for shared, centralized storage

Example (CSV backtest):
```
data_source: csv
data:
  root_dir: examples/backtest_basic/data
  symbol_metadata_csv: examples/backtest_basic/data/metadata.csv
```

Example (Alpaca REST):
```
data_source: alpaca
data:
  api_key: ${ALPACA_API_KEY}
  secret_key: ${ALPACA_API_SECRET}
  trading_base_url: https://paper-api.alpaca.markets
  data_base_url: https://data.alpaca.markets
  timeout_seconds: 10
  symbols: AAPL,MSFT
```

Related:
- `reference/config-reference.md`
- `how-to/data-validation.md`
- `how-to/mmap-storage.md`
- `how-to/symbol-metadata.md`

## 2. Configure the Regime Model

Regime detection is pluggable and supports HMM and custom detectors.

Baseline HMM configuration:
```
hmm:
  kalman_enabled: true
  kalman_process_noise: 1e-5
  kalman_measurement_noise: 1e-4
```

Custom regime model (plugin):
```
regime:
  detector: my_custom_detector
  params:
    feature_set: volatility_trend
    window: 60
```

If you implement a custom detector, register it via the plugin system.

Related:
- `explanation/regime-detection.md`
- `explanation/regime-features.md`
- `explanation/regime-transitions.md`
- `explanation/hmm-math.md`
- `reference/plugin-api.md`

## 3. Build the Strategy

Strategies consume regime state and market data. You can use built-in strategies or provide custom logic.

Built-in strategies:
- `moving_average_cross`
- `pairs_trading`
- `harmonic_pattern`

Example backtest config:
```
strategy: moving_average_cross
strategy_params:
  fast_window: 20
  slow_window: 50
```

Custom strategy via plugin:
```
strategy: my_custom_strategy
strategy_params:
  entry_threshold: 1.5
  exit_threshold: 0.8
```

Related:
- `explanation/strategy-selection.md`
- `reference/plugin-api.md`
- `api/strategy.md`

## 4. Set Execution and Risk Controls

Execution models impact fills and costs. Risk limits protect capital and portfolio constraints.

Execution:
```
execution_model: basic
slippage:
  model: volume
  impact_bps: 5
commission:
  per_share: 0.005
```

Risk controls:
```
risk:
  max_gross_exposure: 2.0
  max_symbol_exposure: 0.2
  max_drawdown: 0.15
```

Related:
- `explanation/execution-models.md`
- `explanation/execution-costs.md`
- `explanation/slippage-math.md`
- `explanation/risk-controls.md`
- `explanation/risk-limits-deep.md`

## 5. Run a Backtest

```
./build/bin/regimeflow_backtest --config examples/backtest_basic/config.yaml
```

Review the report and dashboard outputs.

Related:
- `explanation/backtest-methodology.md`
- `explanation/performance-metrics.md`
- `how-to/dashboard-flow.md`

## 6. Validate Results

Use the metrics validation helpers to verify correctness and regime attribution.

Python validation:
```
import regimeflow as rf
from regimeflow.metrics.validation import validate_regime_attribution

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")
validate_regime_attribution(results)
```

Related:
- `api/metrics.md`
- `api/python-metrics-validation.md`

## 7. Iterate and Promote

Once metrics pass, iterate on:
- Feature sets (regime inputs)
- Strategy parameters
- Execution models
- Risk limits

When ready, move to live paper trading with the live CLI.

Related:
- `how-to/live-trading.md`
- `explanation/live-resiliency.md`
- `explanation/live-order-reconciliation.md`
