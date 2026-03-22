# Live Configuration

Live config is loaded by `src/tools/live_main.cpp` and mapped into `live::LiveConfig`.

## Required Keys

- `live.broker` broker adapter name. Current adapters include `alpaca` and `ib`.
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
- The CLI currently validates Alpaca keys and base URL when `live.broker: alpaca`.
- `LiveTradingEngine::dashboard_snapshot_json()` exports the latest dashboard/account state as a compact JSON payload for dashboards or ops tooling.

## Next Steps

- `live/brokers.md`
- `guide/risk-management.md`
