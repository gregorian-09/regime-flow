# Configuration Reference

This is a practical map of the most important configuration knobs used across the system.

## Core
- `engine.audit_log_path`: backtest audit log file
- `symbols`: list of symbols to trade
- `plugins.search_paths`: directories to scan for plugin `.so`/`.dylib`/`.dll`
- `plugins.load`: explicit plugin filenames (resolved relative to search paths)

## Regime
- `hmm.kalman_enabled`
- `hmm.kalman_process_noise`
- `hmm.kalman_measurement_noise`

## Data
- `symbol_metadata_csv`
- `symbol_metadata_delimiter`
- `symbol_metadata_has_header`
- `symbol_metadata` (inline map)
- `data.type` = `alpaca` (Alpaca REST bars + trades)
- `data.api_key`, `data.secret_key`
- `data.trading_base_url`, `data.data_base_url`
- `data.timeout_seconds`
- `data.symbols` (comma-separated list, filtered against active Alpaca assets when available)

## Live
- `heartbeat_timeout`
- `reconnect_initial_ms`
- `reconnect_max_ms`

## Python
- CLI entrypoint `regimeflow-backtest`
- `BacktestConfig.plugins_search_paths` (list of directories)
- `BacktestConfig.plugins_load` (list of plugin filenames or absolute paths)


## Interpretation

Interpretation: configuration keys map directly to engine, data, and live subsystem behavior.
