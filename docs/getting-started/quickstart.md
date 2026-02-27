# Quickstart (Backtest)

This quickstart runs a backtest using the Python CLI, which is the fastest path for quant research and iteration.

## 1. Build And Activate

```bash
cmake -S . -B build
cmake --build build --target all

python3 -m venv .venv
. .venv/bin/activate
pip install -e python
export PYTHONPATH=python:build/lib
```

## 2. Create A Minimal Backtest Config

Save this to `quickstart.yaml`:

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
    normalize_features: true
    normalization: zscore

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
```

## 3. Run The Backtest

Use a built-in strategy name:

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy moving_average_cross \
  --print-summary
```

To run a custom Python strategy, pass `module:Class`:

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy my_strategies:MyStrategy
```

## 4. Inspect Outputs

You can export reports and curves:

```bash
.venv/bin/python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy moving_average_cross \
  --output-json out/report.json \
  --output-csv out/report.csv \
  --output-equity out/equity.csv \
  --output-trades out/trades.csv
```

## 5. Run A Parity Check Before Live

Compare your backtest configuration to a live config to catch mismatches early:

```bash
regimeflow_parity_check \
  --backtest-config quickstart.yaml \
  --live-config examples/live_paper_alpaca/config.yaml
```

## Next Steps

- `guide/backtesting.md`
- `guide/data-sources.md`
- `guide/strategies.md`
- `guide/risk-management.md`
