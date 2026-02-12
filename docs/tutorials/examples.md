# Examples

This section documents runnable examples intended for local testing and validation. They are designed to be deterministic where possible and safe by default.

## Safety Rules

- No real money keys or production broker credentials should ever be committed.
- Live examples are paper-trading only and require environment variables.
- CI must not execute any live broker example.

## Example Index

- **Backtest Basic**: deterministic backtest on local CSV data.
- **Data Ingest**: CSV normalization and validation pipeline.
- **Live Paper Alpaca**: paper-trading example for Alpaca (env-gated).
- **Live Paper IB**: paper-trading example for Interactive Brokers (env-gated).

## Backtest Basic

Path: `examples/backtest_basic/`

```bash
./build/bin/regimeflow_backtest --config examples/backtest_basic/config.yaml
```

## Data Ingest

Path: `examples/data_ingest/`

```bash
./build/bin/regimeflow_ingest --config examples/data_ingest/config.yaml
```

## Live Paper Alpaca

Path: `examples/live_paper_alpaca/`

Env vars:
- `ALPACA_API_KEY`
- `ALPACA_API_SECRET`
- `ALPACA_PAPER_BASE_URL`

```bash
export ALPACA_API_KEY=...
export ALPACA_API_SECRET=...
export ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
./build/bin/regimeflow_live --config examples/live_paper_alpaca/config.yaml

You can also store these values in a `.env` file in the project root; the live CLI
will load it automatically if present.

When the live CLI runs, it prints connection status and heartbeat health to stdout.

## Alpaca Data REST (Python)

```python
import regimeflow as rf

client = rf.data.AlpacaDataClient({
    "api_key": "...",
    "secret_key": "...",
    "trading_base_url": "https://paper-api.alpaca.markets",
    "data_base_url": "https://data.alpaca.markets",
})

assets = client.list_assets()
bars = client.get_bars(["AAPL", "MSFT"], "1Day", "2024-01-01", "2024-06-30")
trades = client.get_trades(["AAPL"], "2024-01-01", "2024-01-02")
snapshot = client.get_snapshot("AAPL")
```

## Alpaca Data REST (C++)

Fetch assets, bars, trades, and snapshots using the built-in helper (bars/trades paginate and merge all pages):

```bash
export ALPACA_API_KEY=...
export ALPACA_API_SECRET=...
./build/bin/regimeflow_alpaca_fetch --symbols=AAPL,MSFT --start=2024-01-01 --end=2024-01-05 --timeframe=1Day
```

## Alpaca Data Source (Backtest)

Use the Alpaca REST bars as a `data_source`:

```yaml
data_source: alpaca
data:
  api_key: ${ALPACA_API_KEY}
  secret_key: ${ALPACA_API_SECRET}
  trading_base_url: https://paper-api.alpaca.markets
  data_base_url: https://data.alpaca.markets
  timeout_seconds: 10
  symbols: AAPL,MSFT
```
```

## Live Paper IB

Path: `examples/live_paper_ib/`

Env vars:
- `IB_HOST`
- `IB_PORT`
- `IB_CLIENT_ID`

```bash
export IB_HOST=127.0.0.1
export IB_PORT=7497
export IB_CLIENT_ID=1
./build/bin/regimeflow_live --config examples/live_paper_ib/config.yaml
```
