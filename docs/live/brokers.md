# Brokers

Live brokers are configured through `live.broker` and `live.broker_config`. The broker adapter is selected in `live_engine.cpp`.

## Supported Adapters

- `alpaca`
- `binance`
- `ib` when built with `ENABLE_IBAPI`

## Alpaca Broker Config

Keys in `live.broker_config`:

- `api_key`
- `secret_key`
- `base_url`
- `data_url`
- `stream_url`
- `stream_auth_template`
- `stream_subscribe_template`
- `stream_unsubscribe_template`
- `stream_ca_bundle_path`
- `stream_expected_hostname`
- `enable_streaming` (`true` or `false`)
- `paper` (`true` or `false`)
- `timeout_seconds`

Notes:

- Alpaca market-data streaming now authenticates at the WebSocket handshake layer with
  `APCA-API-KEY-ID` and `APCA-API-SECRET-KEY`, matching Alpaca's documented Trading API
  WebSocket header auth flow.
- `stream_auth_template` is retained as a compatibility fallback, but the normal Alpaca
  paper/demo path no longer depends on a post-connect auth message.

## Binance Broker Config

Keys in `live.broker_config`:

- `api_key`
- `secret_key`
- `base_url`
- `stream_url`
- `stream_subscribe_template`
- `stream_unsubscribe_template`
- `stream_ca_bundle_path`
- `stream_expected_hostname`
- `enable_streaming` (`true` or `false`)
- `timeout_seconds`
- `recv_window_ms`

## Interactive Brokers Config

Keys in `live.broker_config`:

- `host`
- `port`
- `client_id`
- `defaults.*` for the adapter-wide contract defaults
- `contracts.<symbol>.*` for per-symbol overrides

Supported Interactive Brokers contract fields:

- `symbol_override`
- `security_type`
- `exchange`
- `currency`
- `primary_exchange`
- `local_symbol`
- `trading_class`
- `last_trade_date_or_contract_month`
- `right`
- `multiplier`
- `strike`
- `con_id`
- `include_expired`

Example:

```yaml
live:
  broker: ib
  broker_config:
    host: 127.0.0.1
    port: 7497
    client_id: 7
    defaults:
      security_type: STK
      exchange: SMART
      currency: USD
    contracts:
      EURUSD:
        security_type: CASH
        exchange: IDEALPRO
      FGBL:
        security_type: FUT
        exchange: EUREX
        currency: EUR
        local_symbol: FGBL MAR 27
        include_expired: true
      BMW_C72_DEC25:
        security_type: OPT
        symbol_override: BMW
        exchange: EUREX
        currency: EUR
        last_trade_date_or_contract_month: 20251219
        strike: 72
        right: C
        multiplier: "100"
```

Notes:

- `contracts.EURUSD` automatically maps `EURUSD` to IB `CASH` fields `symbol=EUR` and `currency=USD` unless you override `symbol_override` or `currency`.
- Use `primary_exchange` for smart-routed non-US equities when the contract would otherwise be ambiguous.
- Use `local_symbol`, `trading_class`, `multiplier`, or `con_id` for futures and options that need extra disambiguation.

## Notes

- The CLI fills Alpaca keys from `ALPACA_API_KEY`, `ALPACA_API_SECRET`, `ALPACA_PAPER_BASE_URL`, and `ALPACA_STREAM_URL` when missing.
- The CLI fills Binance config from `BINANCE_API_KEY`, `BINANCE_SECRET_KEY`, `BINANCE_BASE_URL`, `BINANCE_STREAM_URL`, and `BINANCE_RECV_WINDOW_MS` when missing.
- The CLI fills Interactive Brokers config from `IB_HOST`, `IB_PORT`, and `IB_CLIENT_ID` when missing.
- Secret values in `live.broker_config` can also be expressed as `vault://`, `aws-sm://`, `gcp-sm://`, or `azure-kv://` references and are resolved before the live engine starts.
- Brokers are constructed at engine startup. Invalid config fields typically fail at connect time.
- Capability tests now validate the supported time-in-force matrix for Alpaca, Binance, and IB.
- Env-gated paper smoke tests are available through `ctest` for Alpaca, Binance, and IB.

## Verification Status

- `alpaca`: integrated into the live engine, capability-tested, env-gated paper smoke test available.
- `binance`: integrated into the live engine, capability-tested, env-gated paper/demo smoke test available.
- `ib`: integrated into the live engine, capability-tested when built with `ENABLE_IBAPI`, env-gated paper smoke test available.

## Interactive Brokers Access Boundary

Interactive Brokers availability is not only a code or packaging question. Access can also be blocked by
Interactive Brokers account policy, login policy, jurisdiction, IP geolocation, or compliance restrictions.

Practical implication:

- A user may have a correct RegimeFlow build and correct `IB_HOST` / `IB_PORT` / `IB_CLIENT_ID` values,
  but still be unable to run TWS or connect a paper session because IB blocks login or system access
  for that region or IP origin.
- In that case, the limitation is external to RegimeFlow.

For users:

- If Interactive Brokers is unavailable in your region or from your current IP origin, use another supported
  broker adapter instead of assuming the RegimeFlow IB adapter is broken.
- If you need IB behavior adjusted for a different deployment environment, fork the project, apply the change,
  and open a pull request for review.

## Next Step

- `live/production-readiness.md`
