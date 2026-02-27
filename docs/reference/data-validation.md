# Data Validation Reference

Validation settings are configured under the `validation` prefix and are parsed by `DataSourceFactory`.

## Validation Keys

- `validation.require_monotonic_timestamps` boolean.
- `validation.check_price_bounds` boolean.
- `validation.check_gap` boolean.
- `validation.max_gap_seconds` integer.
- `validation.check_price_jump` boolean.
- `validation.max_jump_pct` double.
- `validation.check_future_timestamps` boolean.
- `validation.max_future_skew_seconds` integer.
- `validation.check_trading_hours` boolean.
- `validation.trading_start_seconds` integer (seconds since midnight).
- `validation.trading_end_seconds` integer (seconds since midnight).
- `validation.check_volume_bounds` boolean.
- `validation.max_volume` integer.
- `validation.max_price` double.
- `validation.check_outliers` boolean.
- `validation.outlier_zscore` double.
- `validation.outlier_warmup` integer.

## Actions

These keys control how the pipeline reacts:

- `validation.on_error`: `skip`, `fill`, `continue`, `fail`.
- `validation.on_gap`: `skip`, `fill`, `continue`, `fail`.
- `validation.on_warning`: `skip`, `fill`, `continue`, `fail`.

## Example

```yaml
data:
  type: csv
  data_directory: data
  file_pattern: "{symbol}.csv"
  has_header: true
  validation:
    require_monotonic_timestamps: true
    check_gap: true
    max_gap_seconds: 600
    check_outliers: true
    outlier_zscore: 4.0
    on_error: fail
    on_gap: fill
    on_warning: continue
```
