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

## Next Steps

- `live/brokers.md`
- `guide/risk-management.md`
