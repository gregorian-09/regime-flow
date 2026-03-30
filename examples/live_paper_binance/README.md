# Live Paper Trading: Binance

Paper or demo trading example using Binance Spot. This is env-gated and should never be run in CI.

## Required env vars

- `BINANCE_API_KEY`
- `BINANCE_SECRET_KEY`
- `BINANCE_BASE_URL`
- `BINANCE_STREAM_URL`

## Run

```bash
export BINANCE_API_KEY=...
export BINANCE_SECRET_KEY=...
export BINANCE_BASE_URL=https://demo-api.binance.com
export BINANCE_STREAM_URL=wss://demo-stream.binance.com/ws
./build/bin/regimeflow_live --config examples/live_paper_binance/config.yaml
```

Current official Binance Spot endpoints you can use:

- Live REST: `https://api.binance.com`
- Live market stream: `wss://stream.binance.com:9443/ws`
- Demo Mode REST: `https://demo-api.binance.com`
- Demo Mode market stream: `wss://demo-stream.binance.com/ws`
- Spot Testnet market stream: `wss://stream.testnet.binance.vision/ws`

RegimeFlow keeps the config file environment-driven so you can choose the venue that
matches your account and current Binance documentation.

The live CLI logs connection status and heartbeat health to stdout while running.

Lifecycle validation before a longer demo session:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_binance/config.yaml \
  --mode submit_cancel_reconcile --quantity 0.001
```

Optional round-trip fill validation on demo or testnet only:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_binance/config.yaml \
  --mode fill_reconcile --quantity 0.001
```
