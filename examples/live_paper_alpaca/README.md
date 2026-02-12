# Live Paper Trading: Alpaca

Paper trading example using Alpaca. This is **env-gated** and should never be run in CI.

## Required env vars

- `ALPACA_API_KEY`
- `ALPACA_API_SECRET`
- `ALPACA_PAPER_BASE_URL` (example: https://paper-api.alpaca.markets)

## Run

```bash
export ALPACA_API_KEY=...
export ALPACA_API_SECRET=...
export ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
./build/bin/regimeflow_live --config examples/live_paper_alpaca/config.yaml

Alternatively, place these values in a `.env` file at the repository root;
the live CLI will load it automatically if present.

The live CLI logs connection status and heartbeat health to stdout while running.
```
