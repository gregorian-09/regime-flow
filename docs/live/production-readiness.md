# Live Production Readiness

Use this page as the operational checklist before promoting a live strategy from local validation to paper trading and then to production.

## What RegimeFlow Verifies In-Repo

- Broker capability tests validate the current time-in-force gates for Alpaca, Binance, and IB.
- Adapter failure tests verify credential and connection guardrails for Alpaca and Binance.
- Live-engine integration tests validate reconciliation, throttling, dashboard updates, and risk shutdowns with a broker-compatible mock.
- Env-gated startup smoke tests are registered in `ctest` for Alpaca, Binance, and IB. They skip cleanly when credentials are absent.
- `regimeflow_live_validate` provides manual broker lifecycle validation in:
  - `submit_cancel_reconcile`
  - `fill_reconcile`

## What You Still Need To Verify Per Broker Account

- Credentials are loaded from environment or `.env`, never from committed config.
- Mounted secret files through `*_FILE` are preferred over plain env when the runtime supports them.
- Secret-manager references should be exercised on the target host so `vault`, `aws`, `gcloud`, or `az` resolution is verified before the first session.
- Paper or demo account permissions are active for every symbol you intend to trade.
- Market-data subscriptions cover the feeds required by your symbols and strategy cadence.
- Time-in-force, order type, and asset-class combinations match the broker account you are actually using.
- Rate limits, reconnect behavior, and heartbeat thresholds are sized for the venue and network path you operate.
- Audit logging and live metrics sinks write to persistent storage, not just stdout.
- Startup logs, runtime error logs, and audit logs should be checked for secret redaction before promotion.

## Recommended Promotion Path

1. Run deterministic backtests and inspect the dashboard/report outputs.
2. Run native unit tests and the broker capability tests.
3. Run the env-gated startup smoke test for your broker with `ctest -R live_paper_`.
4. Run `./build/bin/regimeflow_live_validate --config ... --mode submit_cancel_reconcile` with a safe quantity and, when needed, an explicit passive `--limit-price`.
5. Run `./build/bin/regimeflow_live_validate --config ... --mode fill_reconcile` only on paper/demo accounts after you are comfortable with the venue behavior.
6. Run the broker-specific manual `workflow_dispatch` workflow when you want a GitHub-hosted paper-validation record.
7. Only after paper reconciliation is stable should you consider production capital.

## Validation Modes

### `submit_cancel_reconcile`

Use this first. It is intended to validate:

- auth
- market-data reachability when price discovery is needed
- order submit acknowledgement
- open-order reconciliation
- cancel flow
- post-cancel reconciliation

This mode is the default because it minimizes account drift.

### `fill_reconcile`

Use this only on paper/demo accounts. It attempts a small round-trip:

- submit a small market entry
- observe the position/account reconciliation change
- submit a matching exit
- verify flattening through reconciliation

This mode is intentionally manual because it mutates the paper account state.

## Broker Notes

### Alpaca

- Alpaca documents `day`, `gtc`, `ioc`, and `fok` for supported equity order combinations, while crypto only supports `gtc` and `ioc`.
- Alpaca also documents Trading API WebSocket authentication through the
  `APCA-API-KEY-ID` and `APCA-API-SECRET-KEY` headers. RegimeFlow now uses that
  handshake-layer auth path for Alpaca market-data streaming.
- Alpaca also documents a market-data test stream at `wss://stream.data.alpaca.markets/v2/test`, which is useful for connectivity checks.

### Binance

- Binance Spot documents `GTC`, `IOC`, and `FOK` as the main time-in-force values for spot orders.
- Binance also provides Spot Testnet and Demo Mode environments. Current official examples include `https://api.binance.com/api`, `wss://stream.binance.com:9443/ws`, `https://demo-api.binance.com/api`, `wss://demo-stream.binance.com/ws`, and `wss://stream.testnet.binance.vision/ws`.
- Use environment variables instead of hardcoding those URLs in committed config.

### Interactive Brokers

- IB accepts multiple TIF values, but actual acceptance still depends on order type, exchange, and contract rules.
- The adapter now supports per-symbol contract metadata for `STK`, `CASH`, `FUT`, and `OPT` contracts. Non-US equities usually need `primary_exchange`, and many futures/options need `local_symbol`, `trading_class`, or `multiplier`.
- Build-time support depends on `ENABLE_IBAPI`, so production packaging should validate that option explicitly.
- Interactive Brokers can still deny login or API access based on account policy, jurisdiction, IP geolocation, or other compliance controls. That access boundary is external to RegimeFlow.
- When IB is blocked for a user region or deployment environment, the correct fallback is another supported broker adapter. If an environment-specific IB fix is needed, users should fork the project and open a pull request for review.

## Related Pages

- `live/config.md`
- `live/brokers.md`
- `live/resilience.md`
