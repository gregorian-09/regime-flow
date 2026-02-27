# Performance Tuning

This page focuses on backtest throughput and live latency considerations.

## Backtest Throughput

- Use `mmap` or `mmap_ticks` data sources for large datasets.
- Enable `fill_missing_bars` only when required; it adds processing cost.
- Prefer longer bar intervals when analyzing regime behavior (e.g., `1d` or `1h`).
- Use `run_parallel` or walk-forward optimizer to exploit CPU cores.

## Memory

- Memory-mapped data reduces memory pressure by paging.
- Large symbol universes increase the size of iterators and caches.

## Execution Pipeline

- Slippage and impact models add computation per order.
- If running large grids, use minimal execution cost models.

## Live Latency

- Set `execution.latency.ms` to model expected broker latency in backtests.
- Ensure `max_orders_per_second` does not exceed broker limits.
- Use streaming market data when available.
