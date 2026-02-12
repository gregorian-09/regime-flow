# Full File Index

This index lists every public header under `include/regimeflow/**` and serves as a quick map of the API surface.

## Common

- `regimeflow/common/config.h`
- `regimeflow/common/config_schema.h`
- `regimeflow/common/json.h`
- `regimeflow/common/lru_cache.h`
- `regimeflow/common/memory.h`
- `regimeflow/common/mpsc_queue.h`
- `regimeflow/common/result.h`
- `regimeflow/common/sha256.h`
- `regimeflow/common/spsc_queue.h`
- `regimeflow/common/time.h`
- `regimeflow/common/types.h`
- `regimeflow/common/yaml_config.h`

## Data

- `regimeflow/data/api_data_source.h`
- `regimeflow/data/alpaca_data_client.h`
- `regimeflow/data/alpaca_data_source.h`
- `regimeflow/data/bar.h`
- `regimeflow/data/bar_builder.h`
- `regimeflow/data/corporate_actions.h`
- `regimeflow/data/csv_reader.h`
- `regimeflow/data/data_source.h`
- `regimeflow/data/data_source_factory.h`
- `regimeflow/data/data_validation.h`
- `regimeflow/data/db_client.h`
- `regimeflow/data/db_csv_adapter.h`
- `regimeflow/data/db_source.h`
- `regimeflow/data/live_feed.h`
- `regimeflow/data/memory_data_source.h`
- `regimeflow/data/merged_iterator.h`
- `regimeflow/data/metadata_data_source.h`
- `regimeflow/data/mmap_data_source.h`
- `regimeflow/data/mmap_reader.h`
- `regimeflow/data/mmap_storage.h`
- `regimeflow/data/mmap_writer.h`
- `regimeflow/data/order_book.h`
- `regimeflow/data/order_book_mmap.h`
- `regimeflow/data/order_book_mmap_data_source.h`
- `regimeflow/data/postgres_client.h`
- `regimeflow/data/snapshot_access.h`
- `regimeflow/data/symbol_metadata.h`
- `regimeflow/data/tick.h`
- `regimeflow/data/tick_csv_reader.h`
- `regimeflow/data/tick_mmap.h`
- `regimeflow/data/tick_mmap_data_source.h`
- `regimeflow/data/time_series_query.h`
- `regimeflow/data/validation_config.h`
- `regimeflow/data/validation_utils.h`
- `regimeflow/data/websocket_feed.h`

## Engine

- `regimeflow/engine/audit_log.h`
- `regimeflow/engine/backtest_engine.h`
- `regimeflow/engine/backtest_results.h`
- `regimeflow/engine/backtest_runner.h`
- `regimeflow/engine/engine_factory.h`
- `regimeflow/engine/event_generator.h`
- `regimeflow/engine/event_loop.h`
- `regimeflow/engine/execution_pipeline.h`
- `regimeflow/engine/market_data_cache.h`
- `regimeflow/engine/order.h`
- `regimeflow/engine/order_book_cache.h`
- `regimeflow/engine/order_manager.h`
- `regimeflow/engine/portfolio.h`
- `regimeflow/engine/regime_tracker.h`
- `regimeflow/engine/timer_service.h`

## Events

- `regimeflow/events/dispatcher.h`
- `regimeflow/events/event.h`
- `regimeflow/events/event_queue.h`
- `regimeflow/events/market_event.h`
- `regimeflow/events/order_event.h`
- `regimeflow/events/system_event.h`

## Execution

- `regimeflow/execution/basic_execution_model.h`
- `regimeflow/execution/commission.h`
- `regimeflow/execution/execution_factory.h`
- `regimeflow/execution/execution_model.h`
- `regimeflow/execution/fill_simulator.h`
- `regimeflow/execution/latency_model.h`
- `regimeflow/execution/market_impact.h`
- `regimeflow/execution/order_book_execution_model.h`
- `regimeflow/execution/slippage.h`
- `regimeflow/execution/transaction_cost.h`

## Live

- `regimeflow/live/alpaca_adapter.h`
- `regimeflow/live/audit_log.h`
- `regimeflow/live/binance_adapter.h`
- `regimeflow/live/broker_adapter.h`
- `regimeflow/live/event_bus.h`
- `regimeflow/live/ib_adapter.h`
- `regimeflow/live/live_engine.h`
- `regimeflow/live/live_order_manager.h`
- `regimeflow/live/mq_adapter.h`
- `regimeflow/live/mq_codec.h`
- `regimeflow/live/types.h`

## Metrics

- `regimeflow/metrics/attribution.h`
- `regimeflow/metrics/drawdown.h`
- `regimeflow/metrics/metrics_tracker.h`
- `regimeflow/metrics/performance.h`
- `regimeflow/metrics/performance_calculator.h`
- `regimeflow/metrics/performance_metric.h`
- `regimeflow/metrics/performance_metrics.h`
- `regimeflow/metrics/regime_attribution.h`
- `regimeflow/metrics/report.h`
- `regimeflow/metrics/report_writer.h`
- `regimeflow/metrics/transition_metrics.h`

## Plugins

- `regimeflow/plugins/hooks.h`
- `regimeflow/plugins/interfaces.h`
- `regimeflow/plugins/plugin.h`
- `regimeflow/plugins/registry.h`

## Regime

- `regimeflow/regime/constant_detector.h`
- `regimeflow/regime/ensemble.h`
- `regimeflow/regime/features.h`
- `regimeflow/regime/hmm.h`
- `regimeflow/regime/kalman_filter.h`
- `regimeflow/regime/regime_detector.h`
- `regimeflow/regime/regime_factory.h`
- `regimeflow/regime/state_manager.h`
- `regimeflow/regime/types.h`

## Risk

- `regimeflow/risk/position_sizer.h`
- `regimeflow/risk/risk_factory.h`
- `regimeflow/risk/risk_limits.h`
- `regimeflow/risk/stop_loss.h`

## Strategy

- `regimeflow/strategy/context.h`
- `regimeflow/strategy/strategies/buy_and_hold.h`
- `regimeflow/strategy/strategies/moving_average_cross.h`
- `regimeflow/strategy/strategy.h`
- `regimeflow/strategy/strategy_factory.h`
- `regimeflow/strategy/strategy_manager.h`

## Walk-Forward

- `regimeflow/walkforward/optimizer.h`
