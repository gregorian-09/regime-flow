# Live Paper Trading: Interactive Brokers

Paper trading example using IB. This is **env-gated** and should never be run in CI.

## Required env vars

- `IB_HOST` (example: 127.0.0.1)
- `IB_PORT` (example: 7497 for paper)
- `IB_CLIENT_ID`

## Run

```bash
export IB_HOST=127.0.0.1
export IB_PORT=7497
export IB_CLIENT_ID=1
./build/bin/regimeflow_live --config examples/live_paper_ib/config.yaml
```
