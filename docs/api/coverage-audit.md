# API Coverage Audit

This page maps every public header under `include/regimeflow/**` to its current documentation location.
It is an audit page, not a claim that every header has full method-level reference coverage.

For member-level traceability, see [`public-symbol-index.md`](public-symbol-index.md).

## Coverage Summary

- Public headers audited: `132`
- Headers with at least one explicit topical or package doc mapping: `132`
- Headers currently missing from the package/file index even though they are mentioned elsewhere: `0`

Status meanings:

- `Documented in package and/or topical pages`: package API page exists and there is supporting narrative or reference coverage.
- `Only covered by package-level API page`: header inherits package coverage, but there is no stronger file-specific reference in the docs.
- `Mentioned elsewhere but missing from package/file index`: the header is discussed indirectly, but the API package docs or file index do not currently list it correctly.

## Coverage Gaps

- No current file-index or package-page gaps were found in the public header surface.

## Header Mapping

### `common`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/common/config.h` | `api/common.md`<br>`reference/configuration.md`<br>`reference/config-cheatsheet.md` | Documented in package and/or topical pages |
| `regimeflow/common/config_schema.h` | `api/common.md`<br>`reference/configuration.md`<br>`reference/config-schema.json` | Documented in package and/or topical pages |
| `regimeflow/common/json.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/lru_cache.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/memory.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/mpsc_queue.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/result.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/sha256.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/spsc_queue.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/time.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/types.h` | `api/common.md`<br>`reference/configuration.md` | Only covered by package-level API page |
| `regimeflow/common/yaml_config.h` | `api/common.md`<br>`reference/configuration.md` | Documented in package and/or topical pages |

### `data`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/data/alpaca_data_client.h` | `api/data.md`<br>`guide/data-sources.md`<br>`live/brokers.md` | Documented in package and/or topical pages |
| `regimeflow/data/alpaca_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Documented in package and/or topical pages |
| `regimeflow/data/api_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Documented in package and/or topical pages |
| `regimeflow/data/bar.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/bar_builder.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/corporate_actions.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/csv_reader.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/data_source_factory.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/data_validation.h` | `api/data.md`<br>`reference/data-validation.md` | Documented in package and/or topical pages |
| `regimeflow/data/db_client.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/db_csv_adapter.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/db_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/live_feed.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/memory_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/merged_iterator.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/metadata_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/mmap_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/mmap_reader.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/mmap_storage.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/mmap_writer.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/order_book.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/order_book_mmap.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/order_book_mmap_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/postgres_client.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/snapshot_access.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/symbol_metadata.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/tick.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/tick_csv_reader.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/tick_mmap.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/tick_mmap_data_source.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/time_series_query.h` | `api/data.md`<br>`guide/data-sources.md` | Only covered by package-level API page |
| `regimeflow/data/validation_config.h` | `api/data.md`<br>`reference/data-validation.md` | Documented in package and/or topical pages |
| `regimeflow/data/validation_utils.h` | `api/data.md`<br>`reference/data-validation.md` | Documented in package and/or topical pages |
| `regimeflow/data/websocket_feed.h` | `api/data.md`<br>`explanation/broker-streaming.md`<br>`live/brokers.md` | Documented in package and/or topical pages |

### `engine`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/engine/audit_log.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/backtest_engine.h` | `api/engine.md`<br>`guide/backtesting.md`<br>`getting-started/quickstart.md` | Documented in package and/or topical pages |
| `regimeflow/engine/backtest_results.h` | `api/engine.md`<br>`guide/backtesting.md` | Documented in package and/or topical pages |
| `regimeflow/engine/backtest_runner.h` | `api/engine.md`<br>`guide/backtesting.md` | Documented in package and/or topical pages |
| `regimeflow/engine/dashboard_snapshot.h` | `api/engine.md`<br>`guide/backtesting.md`<br>`tutorials/examples.md`<br>`live/config.md` | Documented in package and/or topical pages |
| `regimeflow/engine/engine_factory.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/event_generator.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/event_loop.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/execution_pipeline.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/market_data_cache.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/order.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/order_book_cache.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/order_manager.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/order_routing.h` | `api/engine.md`<br>`guide/execution-models.md`<br>`explanation/execution-flow.md` | Documented in package and/or topical pages |
| `regimeflow/engine/parity_checker.h` | `api/engine.md`<br>`tutorials/examples.md` | Documented in package and/or topical pages |
| `regimeflow/engine/parity_report.h` | `api/engine.md`<br>`tutorials/examples.md` | Documented in package and/or topical pages |
| `regimeflow/engine/portfolio.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/regime_tracker.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |
| `regimeflow/engine/timer_service.h` | `api/engine.md`<br>`guide/backtesting.md` | Only covered by package-level API page |

### `events`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/events/dispatcher.h` | `api/events.md`<br>`explanation/event-model.md` | Documented in package and/or topical pages |
| `regimeflow/events/event.h` | `api/events.md`<br>`explanation/event-model.md` | Documented in package and/or topical pages |
| `regimeflow/events/event_queue.h` | `api/events.md`<br>`explanation/event-model.md` | Documented in package and/or topical pages |
| `regimeflow/events/market_event.h` | `api/events.md`<br>`explanation/event-model.md` | Documented in package and/or topical pages |
| `regimeflow/events/order_event.h` | `api/events.md`<br>`explanation/event-model.md` | Documented in package and/or topical pages |
| `regimeflow/events/system_event.h` | `api/events.md`<br>`explanation/event-model.md` | Documented in package and/or topical pages |

### `execution`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/execution/basic_execution_model.h` | `api/execution.md`<br>`guide/execution-models.md` | Documented in package and/or topical pages |
| `regimeflow/execution/commission.h` | `api/execution.md`<br>`guide/execution-models.md` | Only covered by package-level API page |
| `regimeflow/execution/execution_factory.h` | `api/execution.md`<br>`guide/execution-models.md` | Only covered by package-level API page |
| `regimeflow/execution/execution_model.h` | `api/execution.md`<br>`guide/execution-models.md` | Only covered by package-level API page |
| `regimeflow/execution/fill_simulator.h` | `api/execution.md`<br>`guide/execution-models.md`<br>`explanation/execution-models.md` | Documented in package and/or topical pages |
| `regimeflow/execution/latency_model.h` | `api/execution.md`<br>`guide/execution-models.md` | Documented in package and/or topical pages |
| `regimeflow/execution/market_impact.h` | `api/execution.md`<br>`explanation/execution-costs.md` | Documented in package and/or topical pages |
| `regimeflow/execution/order_book_execution_model.h` | `api/execution.md`<br>`guide/execution-models.md` | Documented in package and/or topical pages |
| `regimeflow/execution/slippage.h` | `api/execution.md`<br>`explanation/slippage-math.md` | Documented in package and/or topical pages |
| `regimeflow/execution/transaction_cost.h` | `api/execution.md`<br>`explanation/execution-costs.md` | Documented in package and/or topical pages |

### `live`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/live/alpaca_adapter.h` | `api/live.md`<br>`live/brokers.md`<br>`examples/live_paper_alpaca/README.md` | Documented in package and/or topical pages |
| `regimeflow/live/audit_log.h` | `api/live.md`<br>`live/production-readiness.md` | Documented in package and/or topical pages |
| `regimeflow/live/binance_adapter.h` | `api/live.md`<br>`live/brokers.md`<br>`examples/live_paper_binance/README.md` | Documented in package and/or topical pages |
| `regimeflow/live/broker_adapter.h` | `api/live.md`<br>`live/brokers.md` | Documented in package and/or topical pages |
| `regimeflow/live/event_bus.h` | `api/live.md`<br>`explanation/message-bus.md` | Documented in package and/or topical pages |
| `regimeflow/live/ib_adapter.h` | `api/live.md`<br>`live/brokers.md`<br>`examples/live_paper_ib/README.md` | Documented in package and/or topical pages |
| `regimeflow/live/live_engine.h` | `api/live.md`<br>`live/overview.md`<br>`live/config.md` | Documented in package and/or topical pages |
| `regimeflow/live/live_order_manager.h` | `api/live.md`<br>`explanation/live-order-reconciliation.md` | Documented in package and/or topical pages |
| `regimeflow/live/mq_adapter.h` | `api/live.md`<br>`explanation/message-bus.md` | Documented in package and/or topical pages |
| `regimeflow/live/mq_codec.h` | `api/live.md`<br>`explanation/message-bus.md` | Documented in package and/or topical pages |
| `regimeflow/live/secret_hygiene.h` | `api/live.md`<br>`live/config.md`<br>`live/production-readiness.md`<br>`_maintainer/security-credentials.md` | Documented in package and/or topical pages |
| `regimeflow/live/types.h` | `api/live.md`<br>`live/overview.md` | Documented in package and/or topical pages |

### `metrics`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/metrics/attribution.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/drawdown.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/live_performance.h` | `api/metrics.md`<br>`live/overview.md`<br>`reference/configuration.md` | Documented in package and/or topical pages |
| `regimeflow/metrics/metrics_tracker.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/performance.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/performance_calculator.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/performance_metric.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/performance_metrics.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/regime_attribution.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |
| `regimeflow/metrics/report.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Documented in package and/or topical pages |
| `regimeflow/metrics/report_writer.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Documented in package and/or topical pages |
| `regimeflow/metrics/transition_metrics.h` | `api/metrics.md`<br>`explanation/performance-metrics.md` | Only covered by package-level API page |

### `plugins`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/plugins/hooks.h` | `api/plugins.md`<br>`reference/plugin-api.md` | Documented in package and/or topical pages |
| `regimeflow/plugins/interfaces.h` | `api/plugins.md`<br>`reference/plugin-api.md` | Documented in package and/or topical pages |
| `regimeflow/plugins/plugin.h` | `api/plugins.md`<br>`reference/plugin-api.md` | Documented in package and/or topical pages |
| `regimeflow/plugins/registry.h` | `api/plugins.md`<br>`reference/plugin-api.md` | Documented in package and/or topical pages |

### `regime`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/regime/constant_detector.h` | `api/regime.md`<br>`guide/regime-detection.md` | Only covered by package-level API page |
| `regimeflow/regime/ensemble.h` | `api/regime.md`<br>`guide/regime-detection.md`<br>`explanation/regime-detection.md` | Documented in package and/or topical pages |
| `regimeflow/regime/features.h` | `api/regime.md`<br>`explanation/regime-features.md` | Documented in package and/or topical pages |
| `regimeflow/regime/hmm.h` | `api/regime.md`<br>`explanation/hmm-math.md` | Documented in package and/or topical pages |
| `regimeflow/regime/kalman_filter.h` | `api/regime.md`<br>`explanation/regime-detection.md` | Documented in package and/or topical pages |
| `regimeflow/regime/regime_detector.h` | `api/regime.md`<br>`guide/regime-detection.md` | Documented in package and/or topical pages |
| `regimeflow/regime/regime_factory.h` | `api/regime.md`<br>`guide/regime-detection.md` | Documented in package and/or topical pages |
| `regimeflow/regime/state_manager.h` | `api/regime.md`<br>`explanation/regime-transitions.md` | Documented in package and/or topical pages |
| `regimeflow/regime/types.h` | `api/regime.md`<br>`guide/regime-detection.md` | Only covered by package-level API page |

### `risk`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/risk/position_sizer.h` | `api/risk.md`<br>`guide/risk-management.md` | Documented in package and/or topical pages |
| `regimeflow/risk/risk_factory.h` | `api/risk.md`<br>`guide/risk-management.md` | Documented in package and/or topical pages |
| `regimeflow/risk/risk_limits.h` | `api/risk.md`<br>`guide/risk-management.md`<br>`explanation/risk-limits-deep.md` | Documented in package and/or topical pages |
| `regimeflow/risk/stop_loss.h` | `api/risk.md`<br>`guide/risk-management.md` | Documented in package and/or topical pages |

### `strategy`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/strategy/context.h` | `api/strategy.md`<br>`guide/strategies.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategies/buy_and_hold.h` | `api/strategy.md`<br>`guide/strategies.md`<br>`getting-started/quickstart.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategies/harmonic_pattern.h` | `api/strategy.md`<br>`guide/strategies.md`<br>`explanation/strategy-selection.md`<br>`reports/intraday_strategy_tradecheck.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategies/moving_average_cross.h` | `api/strategy.md`<br>`guide/strategies.md`<br>`getting-started/quickstart.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategies/pairs_trading.h` | `api/strategy.md`<br>`guide/strategies.md`<br>`explanation/strategy-selection.md`<br>`reports/intraday_strategy_tradecheck.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategy.h` | `api/strategy.md`<br>`guide/strategies.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategy_factory.h` | `api/strategy.md`<br>`guide/strategies.md` | Documented in package and/or topical pages |
| `regimeflow/strategy/strategy_manager.h` | `api/strategy.md`<br>`guide/strategies.md` | Documented in package and/or topical pages |

### `walk-forward`

| Public Header | Current Docs Location | Status |
| --- | --- | --- |
| `regimeflow/walkforward/optimizer.h` | `api/walkforward.md`<br>`guide/walkforward.md`<br>`explanation/walk-forward.md` | Documented in package and/or topical pages |
