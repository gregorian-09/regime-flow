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

## Notes

- The CLI fills Alpaca keys from `ALPACA_API_KEY`, `ALPACA_API_SECRET`, `ALPACA_PAPER_BASE_URL` when missing.
- Brokers are constructed at engine startup. Invalid config fields typically fail at connect time.
