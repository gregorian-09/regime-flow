# Quickstart (Backtest)

This quickstart assumes you already have a source checkout and want a first backtest run from that checkout. If you want the fastest install path instead, use [Quick Install](quick-install.md).

## Assumptions

- You have a local clone of the repository.
- You can build the project with CMake.
- You want to run the Python CLI against a local source build.

## 1. Build The Project

### Unix-like shells

```bash
cmake -S . -B build
cmake --build build --target all
```

### Windows PowerShell

```powershell
cmake -S . -B build
cmake --build build --config Release --target ALL_BUILD
```

## 2. Prepare The Python Environment

### Unix-like shells

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
export PYTHONPATH=python:build/lib
```

### Windows PowerShell

```powershell
py -3 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -e .
$env:PYTHONPATH = "python;build\lib"
```

## 3. Create A Minimal Backtest Config

Save this as `quickstart.yaml`:

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

## 4. Run The Backtest

Primary CLI style:

```bash
regimeflow-backtest \
  --config quickstart.yaml \
  --strategy moving_average_cross \
  --print-summary
```

Alternative module style:

```bash
python -m regimeflow.cli \
  --config quickstart.yaml \
  --strategy moving_average_cross \
  --print-summary
```

## 5. Export Outputs

```bash
regimeflow-backtest \
  --config quickstart.yaml \
  --strategy moving_average_cross \
  --output-json out/report.json \
  --output-csv out/report.csv \
  --output-equity out/equity.csv \
  --output-trades out/trades.csv
```

## 6. Optional: Compare Backtest And Live Configs

```bash
./build/bin/regimeflow_parity_check \
  --backtest-config quickstart.yaml \
  --live-config examples/live_paper_alpaca/config.yaml
```

On Windows, use `build\bin\regimeflow_parity_check.exe`.

## What To Read Next

- [Backtesting](../guide/backtesting.md)
- [Strategies](../guide/strategies.md)
- [Regime Detection](../guide/regime-detection.md)
- [Configuration](../reference/configuration.md)
- [Troubleshooting](troubleshooting.md)
