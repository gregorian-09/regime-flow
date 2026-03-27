# Live Configuration

Live config is loaded by `src/tools/live_main.cpp` and mapped into `live::LiveConfig`.

## Required Keys

- `live.broker` broker adapter name. Current adapters include `alpaca`, `binance`, and `ib`.
- `live.symbols` list of symbols to trade.

## Core Keys

- `live.paper` boolean for paper trading.
- `live.reconnect.enabled` boolean.
- `live.reconnect.initial_ms` and `live.reconnect.max_ms`.
- `live.heartbeat.enabled` boolean.
- `live.heartbeat.interval_ms` heartbeat timeout in milliseconds.
- `live.risk` risk configuration block. This is passed to the risk factory.
- `live.broker_config` key/value map of broker-specific settings.
- `live.log_dir` output directory for logs and metrics.
- `live.broker_asset_class` default `equity`, used for TIF support.

## Strategy

- `strategy.name` built-in strategy name or registered plugin.
- `strategy.params` strategy configuration.

## Smart Routing

Live routing mirrors the execution routing controls:

- `live.routing.enabled` enables smart routing.
- `live.execution.routing.*` is also accepted and has priority over `live.routing.*`, so a live config can keep the same routing shape as a backtest config.
- `live.routing.mode`: `smart` or `none`.
- `live.routing.max_spread_bps` spread threshold for auto limit routing.
- `live.routing.passive_offset_bps` and `live.routing.aggressive_offset_bps`.
- `live.routing.default_venue` and `live.routing.venues`.
- `live.routing.split.enabled`, `live.routing.split.mode`.
- `live.routing.split.min_child_qty`, `live.routing.split.max_children`.
- `live.routing.split.parent_aggregation`.
- `live.routing.venues[]` can carry the same venue-level microstructure overrides used by backtests: `maker_rebate_bps`, `taker_fee_bps`, `queue_enabled`, `queue_progress_fraction`, `queue_default_visible_qty`, and `queue_depth_mode`.

## Account Margin

Live portfolio tracking now accepts a shared account margin profile:

- `account.margin.initial_margin_ratio`
- `account.margin.maintenance_margin_ratio`
- `account.margin.stop_out_margin_level`

You can override those specifically for live mode under:

- `live.account.margin.*`

These values are used for the engine's internal portfolio and dashboard snapshot calculations. They do not change broker-side margin handling.

## Alpaca Configuration

The live CLI also reads from environment variables if values are missing:

- `ALPACA_API_KEY`
- `ALPACA_API_SECRET`
- `ALPACA_PAPER_BASE_URL`
- `ALPACA_STREAM_URL`

Example:

```yaml
live:
  broker: alpaca
  paper: true
  broker_asset_class: equity
  symbols: ["AAPL", "MSFT"]
  reconnect:
    enabled: true
    initial_ms: 500
    max_ms: 10000
  heartbeat:
    enabled: true
    interval_ms: 10000
  risk:
    limits:
      max_notional: 50000
      max_position_pct: 0.2
  account:
    margin:
      initial_margin_ratio: 0.5
      maintenance_margin_ratio: 0.25
      stop_out_margin_level: 0.4
  broker_config:
    base_url: https://paper-api.alpaca.markets
    enable_streaming: true
    stream_subscribe_template: '{"action":"subscribe","bars":{symbols},"quotes":{symbols}}'
    stream_unsubscribe_template: '{"action":"unsubscribe","bars":{symbols},"quotes":{symbols}}'

strategy:
  name: buy_and_hold
  params:
    warmup_bars: 1

metrics:
  live:
    enable: true
    baseline_report: out/report.json
    output_dir: logs
    sinks: [file]
```

## Notes

- `.env` in the project root is automatically loaded.
- The CLI validates required env-backed fields for `alpaca`, `binance`, and `ib`.
- Secret variables also support the `*_FILE` convention for mounted secrets.
- Alpaca market-data streaming now authenticates during the WebSocket handshake using
  the same Trading API headers Alpaca documents for WebSocket auth:
  `APCA-API-KEY-ID` and `APCA-API-SECRET-KEY`.
- `stream_auth_template` remains available only as a compatibility fallback for feeds
  that still require post-connect auth messages.
- `live.broker_config` now accepts nested objects. They are flattened into broker adapter keys internally, so `defaults.exchange` and `contracts.EURUSD.security_type` work as expected.
- `binance` can be pointed at Spot Testnet or Demo Mode by setting `BINANCE_BASE_URL`
  and `BINANCE_STREAM_URL` in the environment.
  Current official examples:
  `https://api.binance.com/api`,
  `wss://stream.binance.com:9443/ws`,
  `https://demo-api.binance.com/api`,
  `wss://demo-stream.binance.com/ws`,
  and `wss://stream.testnet.binance.vision/ws`.
- `ib` can be configured from `IB_HOST`, `IB_PORT`, and `IB_CLIENT_ID`, then extended with `defaults.*` and `contracts.<symbol>.*` contract metadata for FX, futures, options, and non-US equities. Interactive Brokers can still block login or API access based on account policy, jurisdiction, IP origin, or other compliance controls; that boundary is external to RegimeFlow.
- Secret references are resolved before startup when a broker config value starts with one of:
  - `vault://<mount>/<path>#<field>`
  - `aws-sm://<secret-id>#<json_field>`
  - `gcp-sm://<project>/<secret>#<json_field>`
  - `azure-kv://<vault>/<secret>#<json_field>`
- AWS secret references map to `aws secretsmanager get-secret-value`.
- GCP secret references map to `gcloud secrets versions access`.
- Azure Key Vault references map to `az keyvault secret show`.
- Vault references map to `vault kv get`.
- Startup stderr/stdout and the live audit log redact registered secret values.
- `LiveTradingEngine::dashboard_snapshot_json()` exports the latest dashboard/account state as a compact JSON payload for dashboards or ops tooling.

## Secret-Manager Examples

```yaml
live:
  broker: alpaca
  broker_config:
    api_key: aws-sm://prod/regimeflow/alpaca#api_key
    secret_key: aws-sm://prod/regimeflow/alpaca#secret_key
    base_url: https://paper-api.alpaca.markets
```

```yaml
live:
  broker: binance
  broker_config:
    api_key: gcp-sm://quant-prod/regimeflow-binance#api_key
    secret_key: azure-kv://ops-vault/binance-credentials#secret_key
```

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
      primary_exchange: ARCA
    contracts:
      EURUSD:
        security_type: CASH
        exchange: IDEALPRO
      FGBL:
        security_type: FUT
        exchange: EUREX
        currency: EUR
        local_symbol: FGBL MAR 27
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

## Next Steps

- `live/brokers.md`
- `live/production-readiness.md`
- `guide/risk-management.md`
