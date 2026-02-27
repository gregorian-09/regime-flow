# Data Sources

RegimeFlow data sources are configured via the `data` block (C++ config) or `data_source` + `data` (Python config). The factory is implemented in `src/data/data_source_factory.cpp`.

## Supported Types

- `csv` for OHLCV bars from CSV files.
- `tick_csv` for tick data from CSV files.
- `memory` for in-memory data injection.
- `mmap` for bar data in memory-mapped files.
- `mmap_ticks` for tick data in memory-mapped files.
- `mmap_books` for order books in memory-mapped files.
- `api` for generic HTTP time-series sources.
- `alpaca` for Alpaca REST bars and trades.
- `database` or `db` for Postgres-backed sources.
- Plugin types via `data_source` plugins.

## CSV Bars (`type: csv`)

Key fields:

- `data_directory` path to CSVs.
- `file_pattern` filename format, e.g. `{symbol}.csv`.
- `has_header` boolean.
- `delimiter` single character.
- `date_format` or `datetime_format`.
- `actions_directory` and `actions_file_pattern` for corporate actions.
- `allow_symbol_column` and `symbol_column` for multi-symbol files.
- `utc_offset_seconds` to normalize timestamps.
- `collect_validation_report` to emit validation stats.
- `fill_missing_bars` to fill time gaps.

## Tick CSV (`type: tick_csv`)

Key fields:

- `data_directory`, `file_pattern`, `has_header`, `delimiter`.
- `datetime_format` and `utc_offset_seconds`.
- `collect_validation_report`.

## Memory-Mapped Data

- `mmap` for bars.
- `mmap_ticks` for ticks.
- `mmap_books` for order books.

Key fields:

- `data_directory`.
- `preload_index` (bars only).
- `max_cached_files` and `max_cached_ranges`.

## API Data Source (`type: api`)

Key fields:

- `base_url`, `bars_endpoint`, `ticks_endpoint`.
- `api_key` and `api_key_header`.
- `format` and `time_format`.
- `timeout_seconds`.
- `fill_missing_bars` and `collect_validation_report`.
- `symbols` as a comma-delimited string.

## Alpaca (`type: alpaca`)

Key fields:

- `api_key`, `secret_key`.
- `trading_base_url`, `data_base_url`.
- `timeout_seconds`.
- `symbols` as a comma-delimited string.

## Database (`type: database` or `db`)

Key fields:

- `connection_string`.
- `bars_table`, `ticks_table`, `actions_table`, `order_books_table`, `symbols_table`.
- `connection_pool_size`.
- `bars_has_bar_type`.
- `fill_missing_bars` and `collect_validation_report`.

## Validation Configuration

Validation rules are defined under the `validation` prefix:

- `validation.require_monotonic_timestamps`
- `validation.check_price_bounds`
- `validation.check_gap`
- `validation.max_gap_seconds`
- `validation.check_price_jump`
- `validation.max_jump_pct`
- `validation.check_future_timestamps`
- `validation.max_future_skew_seconds`
- `validation.check_trading_hours`
- `validation.trading_start_seconds`
- `validation.trading_end_seconds`
- `validation.check_volume_bounds`
- `validation.max_volume`
- `validation.max_price`
- `validation.check_outliers`
- `validation.outlier_zscore`
- `validation.outlier_warmup`
- `validation.on_error`
- `validation.on_gap`
- `validation.on_warning`

`on_error`, `on_gap`, and `on_warning` accept: `skip`, `fill`, `continue`, or `fail`.

## Symbol Metadata Overlay

Symbol metadata can be layered on top of any data source:

- `symbol_metadata_csv` path to CSV metadata.
- `symbol_metadata_delimiter` and `symbol_metadata_has_header`.
- Inline metadata via config keys consumed by `load_symbol_metadata_config`.

## Next Steps

- `reference/configuration.md`
- `reference/data-validation.md`
