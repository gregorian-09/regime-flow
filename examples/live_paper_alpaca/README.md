# Live Paper Trading: Alpaca

Paper trading example using Alpaca. This is **env-gated** and should never be run in CI.

## Required env vars

- `ALPACA_API_KEY`
- `ALPACA_API_SECRET`
- `ALPACA_PAPER_BASE_URL` (example: https://paper-api.alpaca.markets)
- `ALPACA_STREAM_URL` (example: `wss://stream.data.alpaca.markets/v2/iex`)

## Run

```bash
export ALPACA_API_KEY=...
export ALPACA_API_SECRET=...
export ALPACA_PAPER_BASE_URL=https://paper-api.alpaca.markets
export ALPACA_STREAM_URL=wss://stream.data.alpaca.markets/v2/iex
./build/bin/regimeflow_live --config examples/live_paper_alpaca/config.yaml
```

Alternatively, place these values in a `.env` file at the repository root;
the live CLI will load it automatically if present.

RegimeFlow now authenticates the Alpaca market-data stream during the WebSocket
handshake using the Trading API headers `APCA-API-KEY-ID` and
`APCA-API-SECRET-KEY`. You do not need to configure a separate post-connect
auth message for the normal paper-trading path.

For a deterministic connectivity check, Alpaca also documents a test market-data stream
at `wss://stream.data.alpaca.markets/v2/test` and the symbol `FAKEPACA`.
That test endpoint is useful for connectivity checks, but it is not the normal paper-trading
market-data feed.

For actual paper-trading market data, Alpaca documents feed URLs under
`wss://stream.data.alpaca.markets/{version}/{feed}`. For equities on the free plan,
`wss://stream.data.alpaca.markets/v2/iex` is the standard real-time feed.

The live CLI logs connection status and heartbeat health to stdout while running.

Lifecycle validation before a longer paper session:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_alpaca/config.yaml \
  --mode submit_cancel_reconcile --quantity 1
```

Optional round-trip fill validation on paper only:

```bash
./build/bin/regimeflow_live_validate \
  --config examples/live_paper_alpaca/config.yaml \
  --mode fill_reconcile --quantity 1
```
