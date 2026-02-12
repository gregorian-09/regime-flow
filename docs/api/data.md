# Package: `regimeflow::data`

## Summary

Data access and normalization layer. Provides tick/bar types, data sources (memory, mmap, database, live), iterators, validation utilities, and symbol metadata. The data package is responsible for delivering consistent market data into the event pipeline.

Related diagrams:
- [Data Flow](../explanation/data-flow.md)
- [Structural Flow](../diagrams/structural-flow.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/data/api_data_source.h` | API-backed data source interface. |
| `regimeflow/data/alpaca_data_client.h` | Alpaca REST client (assets, bars, snapshots). |
| `regimeflow/data/alpaca_data_source.h` | Alpaca REST-backed data source. |
| `regimeflow/data/bar.h` | Bar OHLCV type and helpers. |
| `regimeflow/data/bar_builder.h` | Bar aggregation utilities. |
| `regimeflow/data/corporate_actions.h` | Splits/dividends and adjustment metadata. |
| `regimeflow/data/csv_reader.h` | CSV market data reader. |
| `regimeflow/data/data_source.h` | Base data source interface. |
| `regimeflow/data/data_source_factory.h` | Factory for constructing data sources. |
| `regimeflow/data/data_validation.h` | Validation entrypoints and orchestration. |
| `regimeflow/data/db_client.h` | Database client abstraction. |
| `regimeflow/data/db_csv_adapter.h` | CSV bridge for DB-like inputs. |
| `regimeflow/data/db_source.h` | Database-backed data source. |
| `regimeflow/data/live_feed.h` | Live feed base interface. |
| `regimeflow/data/memory_data_source.h` | In-memory data source for tests and small runs. |
| `regimeflow/data/merged_iterator.h` | Merge-join iterators for multi-symbol data. |
| `regimeflow/data/metadata_data_source.h` | Symbol metadata access. |
| `regimeflow/data/mmap_data_source.h` | Memory-mapped data source. |
| `regimeflow/data/mmap_reader.h` | Mmap reader utilities. |
| `regimeflow/data/mmap_storage.h` | Mmap storage manager and layout. |
| `regimeflow/data/mmap_writer.h` | Mmap writer utilities. |
| `regimeflow/data/order_book.h` | Order book representation. |
| `regimeflow/data/order_book_mmap.h` | Mmap order book layout. |
| `regimeflow/data/order_book_mmap_data_source.h` | Mmap-backed order book source. |
| `regimeflow/data/postgres_client.h` | PostgreSQL client implementation. |
| `regimeflow/data/snapshot_access.h` | Consistent snapshot read helpers. |
| `regimeflow/data/symbol_metadata.h` | Symbol metadata structures. |
| `regimeflow/data/tick.h` | Tick type and helpers. |
| `regimeflow/data/tick_csv_reader.h` | Tick CSV reader. |
| `regimeflow/data/tick_mmap.h` | Mmap tick layout. |
| `regimeflow/data/tick_mmap_data_source.h` | Mmap-backed tick source. |
| `regimeflow/data/time_series_query.h` | Query constraints for time-series access. |
| `regimeflow/data/validation_config.h` | Validation configuration schema. |
| `regimeflow/data/validation_utils.h` | Validation helpers for ticks/bars. |
| `regimeflow/data/websocket_feed.h` | Generic websocket live feed with schema validation. |

## Type Index

| Type | Description |
| --- | --- |
| `Bar`, `Tick` | Canonical market data types. |
| `BarBuilder` | Aggregates ticks into bars. |
| `DataSource` | Common interface for data iteration. |
| `AlpacaDataClient` | Alpaca REST helper for assets/bars/trades/snapshots. |
| `AlpacaDataSource` | REST-backed data source using Alpaca bars and trades. |
| `LiveFeed` | Base interface for streaming live data. |
| `MmapDataSource` | High-throughput, low-latency playback source. |
| `MemoryDataSource` | Lightweight in-memory data source. |
| `MergedTickIterator`, `MergedOrderBookIterator` | Multi-stream merge iterators. |
| `OrderBook` | Book snapshot state container. |
| `SymbolMetadata` | Metadata for contract sizing, currency, and exchange info. |
| `ValidationConfig` | Validation thresholds for data integrity. |

## Lifecycle & Usage Notes

- Backtests typically use `MmapDataSource` + `MergedTickIterator` for deterministic playback.
- Live trading uses `LiveFeed` implementations and `WebSocketFeed` schema validation.
- Data validation is designed to run pre-ingest and at runtime for backtest integrity.
- `SymbolMetadata` should be loaded before portfolio and risk checks to ensure sizing is correct.
- Use `AlpacaDataClient` for lightweight Alpaca REST pulls (assets, bars, trades, snapshots).

## Type Details

### `Bar` / `Tick` / `Quote`

Canonical market data types used by the engine.

Fields and Helpers:

| Member | Description |
| --- | --- |
| `Bar` fields | Timestamp, symbol, OHLCV, trade count, VWAP. |
| `Bar::mid()` | Mid price between high/low. |
| `Bar::typical()` | Typical price. |
| `Bar::range()` | High-low range. |
| `Bar::is_bullish()` | Close > open. |
| `Bar::is_bearish()` | Close < open. |
| `Tick` fields | Timestamp, symbol, price, quantity, flags. |
| `Quote` fields | Timestamp, symbol, bid/ask, sizes. |
| `Quote::mid()` | Mid price. |
| `Quote::spread()` | Absolute spread. |
| `Quote::spread_bps()` | Spread in bps. |
| `BarType` | Bar aggregation enum. |

### `BarBuilder`

Aggregates ticks into bars by time or volume.

Methods:

| Method | Description |
| --- | --- |
| `BarBuilder(config)` | Construct builder with configuration. |
| `process(tick)` | Process a tick and emit bar if complete. |
| `flush()` | Flush current in-progress bar. |
| `reset()` | Reset builder state. |

Method Details:

#### `BarBuilder(config)`
Parameters: builder config.
Returns: Instance.
Throws: None.

#### `process(tick)`
Parameters: `tick` input.
Returns: Optional bar.
Throws: None.

#### `flush()`
Parameters: None.
Returns: Optional bar.
Throws: None.

#### `reset()`
Parameters: None.
Returns: `void`.
Throws: None.

### `MultiSymbolBarBuilder`

Aggregates bars for multiple symbols simultaneously.

Methods:

| Method | Description |
| --- | --- |
| `MultiSymbolBarBuilder(config)` | Construct multi-symbol builder. |
| `process(tick)` | Process a tick and emit bar if complete. |
| `flush_all()` | Flush all in-progress bars. |

Method Details:

#### `MultiSymbolBarBuilder(config)`
Parameters: builder config.
Returns: Instance.
Throws: None.

#### `process(tick)`
Parameters: `tick` input.
Returns: Optional bar.
Throws: None.

#### `flush_all()`
Parameters: None.
Returns: Vector of bars.
Throws: None.

### `DataSource`

Abstract interface for historical data access.

Methods:

| Method | Description |
| --- | --- |
| `get_available_symbols()` | Enumerate available symbols. |
| `get_available_range(symbol)` | Get available time range. |
| `get_bars(symbol, range, bar_type)` | Fetch bars. |
| `get_ticks(symbol, range)` | Fetch ticks. |
| `get_order_books(symbol, range)` | Fetch order books. |
| `create_iterator(symbols, range, bar_type)` | Create bar iterator. |
| `create_tick_iterator(symbols, range)` | Create tick iterator. |
| `create_book_iterator(symbols, range)` | Create order book iterator. |
| `get_corporate_actions(symbol, range)` | Fetch corporate actions. |

Method Details:

#### `get_available_symbols()`
Parameters: None.
Returns: Vector of symbols.
Throws: None.

#### `get_available_range(symbol)`
Parameters: `symbol` ID.
Returns: `TimeRange`.
Throws: None.

#### `get_bars(symbol, range, bar_type)`
Parameters: symbol, range, bar type.
Returns: Vector of bars.
Throws: None.

#### `get_ticks(symbol, range)`
Parameters: symbol, range.
Returns: Vector of ticks.
Throws: None.

#### `get_order_books(symbol, range)`
Parameters: symbol, range.
Returns: Vector of order books.
Throws: None.

#### `create_iterator(symbols, range, bar_type)`
Parameters: symbol list, range, bar type.
Returns: `DataIterator` pointer.
Throws: None.

#### `create_tick_iterator(symbols, range)`
Parameters: symbol list, range.
Returns: `TickIterator` pointer.
Throws: None.

#### `create_book_iterator(symbols, range)`
Parameters: symbol list, range.
Returns: `OrderBookIterator` pointer.
Throws: None.

#### `get_corporate_actions(symbol, range)`
Parameters: symbol, range.
Returns: Vector of corporate actions.
Throws: None.

### `DataIterator` / `TickIterator` / `OrderBookIterator`

Abstract iterators for bars, ticks, and order books.

Methods:

| Method | Description |
| --- | --- |
| `has_next()` | True if more data exists. |
| `next()` | Retrieve next item. |
| `reset()` | Reset to beginning. |

Method Details:

#### `has_next()`
Parameters: None.
Returns: `bool`.
Throws: None.

#### `next()`
Parameters: None.
Returns: Next element.
Throws: None.

#### `reset()`
Parameters: None.
Returns: `void`.
Throws: None.

### `MergedBarIterator` / `MergedTickIterator` / `MergedOrderBookIterator`

Merge multiple iterators into a time-ordered stream.

Methods:

| Method | Description |
| --- | --- |
| `Merged*Iterator(iterators)` | Construct with iterator list. |
| `has_next()` | True if more items exist. |
| `next()` | Return next merged item. |
| `reset()` | Reset iterators and heap. |

Method Details:

#### `Merged*Iterator(iterators)`
Parameters: iterator list.
Returns: Instance.
Throws: None.

#### `has_next()` / `next()` / `reset()`
Parameters: None.
Returns: Bool/next element/void.
Throws: None.

### `CSVDataSource` / `CSVTickDataSource`

CSV-backed data sources for bars and ticks.

Methods:

| Method | Description |
| --- | --- |
| `CSVDataSource(config)` | Construct bar CSV source. |
| `CSVTickDataSource(config)` | Construct tick CSV source. |
| `get_available_symbols()` | Enumerate available symbols. |
| `get_available_range(symbol)` | Get available time range. |
| `get_bars(...)` | Fetch bars (bar CSV). |
| `get_ticks(...)` | Fetch ticks. |
| `create_iterator(...)` | Create bar iterator. |
| `create_tick_iterator(...)` | Create tick iterator (tick CSV). |
| `get_corporate_actions(...)` | Fetch corporate actions. |
| `last_report()` | Last validation report. |

### `ApiDataSource`

REST API-backed data source.

Methods:

| Method | Description |
| --- | --- |
| `ApiDataSource(config)` | Construct API source. |

### `AlpacaDataClient`

Lightweight REST client for Alpaca assets and market data.

Methods:

| Method | Description |
| --- | --- |
| `list_assets(status, asset_class)` | Fetch asset metadata. |
| `get_bars(symbols, timeframe, start, end, limit, page_token)` | Fetch historical bars (paginated). |
| `get_trades(symbols, start, end, limit, page_token)` | Fetch trades/ticks (paginated). |
| `get_snapshot(symbol)` | Fetch latest snapshot for a symbol. |

### `AlpacaDataSource`

REST-backed `DataSource` that pulls historical bars and trades from Alpaca.

Notes:
- Supports bars + trades (ticks). Order books not implemented.
- Handles Alpaca REST pagination transparently.
- Filters configured `data.symbols` against active Alpaca assets when available.
- Uses Alpaca REST responses and maps them into `Bar` / `Tick` records.
| `get_available_symbols()` | Enumerate symbols from API. |
| `get_available_range(symbol)` | Get available range. |
| `get_bars(...)` | Fetch bars via API. |
| `get_ticks(...)` | Fetch ticks via API. |
| `create_iterator(...)` | Create bar iterator. |
| `get_corporate_actions(...)` | Fetch corporate actions. |
| `last_report()` | Last validation report. |

### `DbClient` / `DatabaseDataSource`

Database-backed client and data source.

Methods:

| Method | Description |
| --- | --- |
| `query_bars(symbol, range, bar_type)` | Query bars via DB client. |
| `query_ticks(symbol, range)` | Query ticks via DB client. |
| `list_symbols()` | List symbols. |
| `list_symbols_with_metadata(table)` | List symbols with metadata. |
| `get_available_range(symbol)` | Available range. |
| `query_corporate_actions(symbol, range)` | Query corporate actions. |
| `query_order_books(symbol, range)` | Query order books. |
| `DatabaseDataSource(config)` | Construct DB data source. |
| `set_client(client)` | Inject DB client. |
| `set_corporate_actions(symbol, actions)` | Inject corporate actions. |
| `get_available_symbols()` | Enumerate symbols. |
| `get_available_range(symbol)` | Available range. |
| `get_bars(...)` / `get_ticks(...)` / `get_order_books(...)` | Fetch data. |
| `create_iterator(...)` / `create_tick_iterator(...)` / `create_book_iterator(...)` | Create iterators. |
| `get_corporate_actions(...)` | Fetch corporate actions. |
| `last_report()` | Last validation report. |

### `MemoryDataSource`

In-memory data source for tests or ad-hoc usage.

Methods:

| Method | Description |
| --- | --- |
| `add_bars(symbol, bars)` | Add bars to memory store. |
| `add_ticks(symbol, ticks)` | Add ticks to memory store. |
| `add_order_books(symbol, books)` | Add order books. |
| `add_symbol_info(info)` | Add symbol metadata. |
| `set_corporate_actions(symbol, actions)` | Add corporate actions. |
| `get_available_symbols()` | Enumerate symbols. |
| `get_available_range(symbol)` | Get time range. |
| `get_bars(...)` | Fetch bars. |
| `get_ticks(...)` | Fetch ticks. |
| `get_order_books(...)` | Fetch order books. |
| `create_iterator(...)` | Create bar iterator. |
| `create_tick_iterator(...)` | Create tick iterator. |
| `create_book_iterator(...)` | Create order book iterator. |
| `get_corporate_actions(...)` | Fetch corporate actions. |

### `MemoryMappedDataSource`

Mmap-backed data source for large historical datasets.

Methods:

| Method | Description |
| --- | --- |
| `MemoryMappedDataSource(config)` | Construct mmap data source. |
| `get_available_symbols()` | Enumerate symbols. |
| `get_available_range(symbol)` | Available time range. |
| `get_bars(...)` | Fetch bars. |
| `get_ticks(...)` | Fetch ticks. |
| `create_iterator(...)` | Create bar iterator. |
| `get_corporate_actions(...)` | Fetch corporate actions. |
| `set_corporate_actions(symbol, actions)` | Inject corporate actions. |

### `MemoryMappedDataFile`

Memory-mapped access to bar data files.

Methods:

| Method | Description |
| --- | --- |
| `MemoryMappedDataFile(path)` | Map a file into memory. |
| `~MemoryMappedDataFile()` | Unmap and close file. |
| `header()` | Access file header. |
| `symbol()` | Symbol string. |
| `symbol_id()` | Symbol ID. |
| `time_range()` | Covered time range. |
| `bar_count()` | Number of bars. |
| `operator[](index)` | Bar view (unchecked). |
| `at(index)` | Bar view (checked). |
| `begin()` / `end()` | Iterate bar views. |
| `find_range(range)` | Find index range for time range. |
| `timestamps()` / `opens()` / `highs()` / `lows()` / `closes()` / `volumes()` | Column views. |
| `date_index_count()` | Date index count. |
| `preload_index()` | Preload date index. |

### `OrderBookMmapFile` / `OrderBookMmapWriter`

Memory-mapped order book snapshots and writer.

Methods:

| Method | Description |
| --- | --- |
| `OrderBookMmapFile(path)` | Map order book file. |
| `~OrderBookMmapFile()` | Unmap and close file. |
| `header()` | Access file header. |
| `symbol()` | Symbol string. |
| `symbol_id()` | Symbol ID. |
| `time_range()` | Covered time range. |
| `book_count()` | Number of snapshots. |
| `at(index)` | Read snapshot at index. |
| `find_range(range)` | Find index range for time range. |
| `OrderBookMmapWriter::write_books(path, symbol, books)` | Write snapshots to file. |

### `OrderBookMmapDataSource`

Data source for memory-mapped order book snapshots.

Methods:

| Method | Description |
| --- | --- |
| `OrderBookMmapDataSource(config)` | Construct data source. |
| `get_available_symbols()` | Enumerate symbols. |
| `get_available_range(symbol)` | Available range. |
| `get_order_books(symbol, range)` | Fetch order books. |
| `create_book_iterator(symbols, range)` | Create book iterator. |
| `set_corporate_actions(symbol, actions)` | Inject corporate actions. |

### `TickMmapFile` / `TickMmapWriter`

Memory-mapped tick storage and writer.

Methods:

| Method | Description |
| --- | --- |
| `TickMmapFile(path)` | Map tick file. |
| `~TickMmapFile()` | Unmap and close file. |
| `header()` | Access file header. |
| `symbol()` | Symbol string. |
| `symbol_id()` | Symbol ID. |
| `time_range()` | Covered time range. |
| `tick_count()` | Number of ticks. |
| `operator[](index)` / `at(index)` | Tick view access. |
| `find_range(range)` | Find index range for time range. |
| `timestamps()` / `prices()` / `quantities()` / `flags()` | Column views. |
| `TickMmapWriter::write_ticks(path, symbol, ticks)` | Write ticks to file. |

### `TickMmapDataSource`

Data source for memory-mapped tick data.

Methods:

| Method | Description |
| --- | --- |
| `TickMmapDataSource(config)` | Construct data source. |
| `get_available_symbols()` | Enumerate symbols. |
| `get_available_range(symbol)` | Available range. |
| `get_ticks(symbol, range)` | Fetch ticks. |
| `create_tick_iterator(symbols, range)` | Create tick iterator. |
| `set_corporate_actions(symbol, actions)` | Inject corporate actions. |

### `MmapWriter` / `MmapStorage`

Helpers for writing and reading mmap bar files.

Methods:

| Method | Description |
| --- | --- |
| `MmapWriter::write_bars(path, symbol, bar_type, bars)` | Write bars to mmap file. |
| `MmapStorage(path)` | Construct storage wrapper. |
| `open_read()` | Open mmap file for reading. |
| `read_bars(symbol, range)` | Read bars from file. |

### `SnapshotAccess`

Point-in-time snapshot access from a data source.

Methods:

| Method | Description |
| --- | --- |
| `SnapshotAccess(source)` | Construct snapshot accessor. |
| `bar_at(symbol, ts, bar_type)` | Get bar at timestamp. |
| `tick_at(symbol, ts)` | Get tick at timestamp. |
| `order_book_at(symbol, ts)` | Get order book at timestamp. |

### `TimeSeriesQuery`

Convenience wrapper for time-series queries.

Methods:

| Method | Description |
| --- | --- |
| `TimeSeriesQuery(source)` | Construct query wrapper. |
| `bars(symbol, range, bar_type)` | Query bars. |
| `ticks(symbol, range)` | Query ticks. |
| `order_books(symbol, range)` | Query order books. |

### `MetadataOverlayDataSource`

Data source wrapper that overlays symbol metadata from CSV/config.

Methods:

| Method | Description |
| --- | --- |
| `MetadataOverlayDataSource(inner, csv_metadata, config_metadata)` | Construct overlay. |
| `get_available_symbols()` | Enumerate symbols with metadata. |
| `get_available_range(symbol)` | Available range. |
| `get_bars(...)` / `get_ticks(...)` / `get_order_books(...)` | Fetch data. |
| `create_iterator(...)` / `create_tick_iterator(...)` / `create_book_iterator(...)` | Create iterators. |
| `get_corporate_actions(...)` | Fetch corporate actions. |

### `SymbolMetadata`

Symbol metadata load and apply utilities.

Functions:

| Function | Description |
| --- | --- |
| `load_symbol_metadata_csv(path, delimiter, has_header)` | Load metadata from CSV. |
| `load_symbol_metadata_config(config, key)` | Load metadata from config. |
| `metadata_from_symbols(symbols)` | Convert SymbolInfo to metadata map. |
| `apply_symbol_metadata(symbols, metadata, overwrite)` | Apply metadata to symbols. |

### `CsvDbClient` / `PostgresDbClient`

DB clients backed by CSV or PostgreSQL.

Methods:

| Method | Description |
| --- | --- |
| `CsvDbClient(source)` | Construct DB client from CSV source. |
| `query_bars(...)` / `query_ticks(...)` / `query_order_books(...)` | Query data. |
| `list_symbols()` | List symbols. |
| `get_available_range(symbol)` | Available range. |
| `query_corporate_actions(symbol, range)` | Query corporate actions. |
| `PostgresDbClient(config)` | Construct Postgres client. |
| `list_symbols_with_metadata(table)` | List symbols with metadata. |

### `LiveFeed` / `PollingRestFeed`

Live feed interfaces and polling implementation.

Methods:

| Method | Description |
| --- | --- |
| `connect()` | Establish connection. |
| `disconnect()` | Disconnect feed. |
| `is_connected()` | Connection status. |
| `subscribe(symbols)` | Subscribe to symbols. |
| `unsubscribe(symbols)` | Unsubscribe. |
| `on_bar(cb)` | Register bar callback. |
| `on_tick(cb)` | Register tick callback. |
| `on_book(cb)` | Register order book callback. |
| `poll()` | Poll for new data. |

### `WebSocketFeed`

WebSocket live feed with validation and reconnect logic.

Methods:

| Method | Description |
| --- | --- |
| `WebSocketFeed(config)` | Construct feed. |
| `connect()` | Connect to endpoint. |
| `disconnect()` | Disconnect feed. |
| `is_connected()` | Connection status. |
| `subscribe(symbols)` | Subscribe to symbols. |
| `unsubscribe(symbols)` | Unsubscribe. |
| `on_bar(cb)` | Register bar callback. |
| `on_tick(cb)` | Register tick callback. |
| `on_book(cb)` | Register order book callback. |
| `on_raw(cb)` | Register raw message callback. |
| `on_reconnect(cb)` | Register reconnect callback. |
| `validate_tls_config()` | Validate TLS configuration. |
| `handle_message(message)` | Process raw message. |
| `send_raw(message)` | Send raw message. |
| `poll()` | Poll socket for data. |

### `OrderBook`

Order book snapshot types.

Fields:

| Member | Description |
| --- | --- |
| `BookLevel` | Price level (price, quantity, orders). |
| `OrderBook` | Timestamp, symbol, bids, asks. |

### `CorporateActionAdjuster`

Applies corporate actions to data and symbol mappings.

Methods:

| Method | Description |
| --- | --- |
| `add_actions(symbol, actions)` | Add corporate actions. |
| `adjust_bar(symbol, bar)` | Adjust bar for actions. |
| `resolve_symbol(symbol)` | Resolve latest symbol. |
| `resolve_symbol(symbol, at)` | Resolve symbol at time. |
| `aliases_for(symbol)` | Get aliases. |

### `DataSourceFactory`

Creates data sources from configuration.

Methods:

| Method | Description |
| --- | --- |
| `create(config)` | Build data source from config. |

### `ValidationReport`

Aggregated validation report for ingestion.

Methods:

| Method | Description |
| --- | --- |
| `add_issue(issue)` | Add validation issue. |
| `ok()` | True if no errors. |
| `error_count()` | Count of errors. |
| `warning_count()` | Count of warnings. |
| `issues()` | All issues. |
| `summary()` | Summary string. |

### `ValidationConfig`

Validation thresholds for data integrity.

Fields:

| Member | Description |
| --- | --- |
| `ValidationConfig` | Limits for gaps, jumps, bounds, and outliers. |
| `ValidationAction` | Action enum for validation outcomes. |

### `ValidationUtils`

Validation helpers for bars and ticks.

Functions:

| Function | Description |
| --- | --- |
| `bar_interval_for(bar_type)` | Resolve expected bar interval. |
| `fill_missing_time_bars(bars, interval)` | Fill gaps for time bars. |
| `validate_bars(bars, bar_type, config, fill, collect, report)` | Validate and optionally repair bars. |
| `validate_ticks(ticks, config, collect, report)` | Validate tick data. |

### `SymbolMetadata`

Instrument metadata used for contract sizing and compliance.

Fields:

| Member | Description |
| --- | --- |
| `SymbolInfo` | Symbol metadata record (exchange, tick size, sector, etc). |

### `SymbolInfo`

Metadata describing a tradeable symbol.

Fields:

| Member | Description |
| --- | --- |
| `ticker` | Symbol string. |
| `exchange` | Exchange venue. |
| `asset_class` | Asset class enum. |
| `currency` | Base currency. |
| `tick_size` | Minimum price increment. |
| `lot_size` | Lot size. |
| `multiplier` | Contract multiplier. |
| `sector` / `industry` | Sector and industry tags. |

### `CorporateActionType` / `CorporateAction`

Corporate actions and adjustments metadata.

Fields:

| Member | Description |
| --- | --- |
| `Split` / `Dividend` / `SymbolChange` | Corporate action types. |
| `effective_date` | Action effective date. |
| `factor` / `amount` | Adjustment parameters. |
| `new_symbol` | New symbol after change. |

### `ValidationSeverity` / `ValidationIssue` / `ValidationAction`

Validation issue metadata and actions.

Fields:

| Member | Description |
| --- | --- |
| `ValidationSeverity` | Error or warning. |
| `ValidationIssue` | Line, severity, and message. |
| `ValidationAction` | Fail/Skip/Fill/Continue actions. |

### `BookLevel`

Single level in an order book.

Fields:

| Member | Description |
| --- | --- |
| `price` | Level price. |
| `quantity` | Level quantity. |
| `num_orders` | Number of orders at level. |

### `LiveFeedAdapter`

Base interface for live data feeds.

Methods:

| Method | Description |
| --- | --- |
| `connect()` / `disconnect()` / `is_connected()` | Connection lifecycle. |
| `subscribe(symbols)` / `unsubscribe(symbols)` | Symbol subscriptions. |
| `on_bar(cb)` / `on_tick(cb)` / `on_book(cb)` | Data callbacks. |
| `poll()` | Poll for updates. |

### `VectorBarIterator` / `VectorTickIterator` / `VectorOrderBookIterator`

In-memory iterators over bars, ticks, and order books.

Methods:

| Method | Description |
| --- | --- |
| `has_next()` / `next()` / `reset()` | Standard iterator lifecycle. |

### `InMemoryDbClient`

In-memory DB client for tests and ad-hoc data.

Methods:

| Method | Description |
| --- | --- |
| `add_bars(...)` / `add_ticks(...)` / `add_order_books(...)` | Add data. |
| `add_symbol_info(...)` / `add_corporate_actions(...)` | Add metadata. |
| `query_bars(...)` / `query_ticks(...)` / `query_order_books(...)` | Query data. |
| `list_symbols()` | List symbols. |
| `get_available_range(symbol)` | Available range. |

### `DatabaseDataSource`

Database-backed data source (wraps `DbClient`).

Methods:

| Method | Description |
| --- | --- |
| `DatabaseDataSource(config)` | Construct data source. |
| `set_client(client)` | Inject DB client. |
| `set_corporate_actions(symbol, actions)` | Inject corporate actions. |
| `get_available_symbols()` | Enumerate symbols. |
| `get_available_range(symbol)` | Available range. |
| `get_bars(...)` / `get_ticks(...)` / `get_order_books(...)` | Fetch data. |
| `create_iterator(...)` / `create_tick_iterator(...)` / `create_book_iterator(...)` | Create iterators. |
| `get_corporate_actions(...)` | Fetch corporate actions. |

### `FileHeader` / `DateIndex` / `BarView` / `Iterator`

Mmap bar file layout and views.

Fields and Helpers:

| Member | Description |
| --- | --- |
| `FileHeader` | Mmap bar file header layout. |
| `DateIndex` | Date-to-offset entry. |
| `BarView` | Zero-copy bar view. |
| `Iterator` | Forward iterator for bar views. |

### `BookFileHeader` / `BookDateIndex`

Mmap order book file layout types.

### `TickFileHeader` / `TickDateIndex` / `TickView`

Mmap tick file layout types and views.

### `BarType`

Bar aggregation enum (time/volume/tick/dollar).

### `CorporateAction` / `CorporateActionType`

Corporate action records and enum.

### `Tick` / `Quote`

Tick and quote data types.

### `TickIterator` / `OrderBookIterator`

Iterator interfaces for ticks and order books.

### `ValidationIssue` / `ValidationSeverity` / `ValidationAction`

Validation issue metadata and severity/action enums.

### `PollingRestFeed`

Polling REST feed implementation of `LiveFeedAdapter`.

### `CSVTickDataSource`

Tick CSV data source.

### `VectorTickIterator` / `VectorOrderBookIterator`

In-memory iterators for ticks and order books.

### `MergedTickIterator` / `MergedOrderBookIterator`

Merged iterators for multi-symbol streams.

### `MmapStorage`

Wrapper for reading bars from mmap files.

### `OrderBookMmapWriter`

Writer for mmap order book files.

### `TickMmapWriter`

Writer for mmap tick files.

### `ConnectionPool` / `PostgresDbClient`

PostgreSQL connection pool and DB client.

### `ReconnectState`

WebSocket reconnect state snapshot.

### `OrderBookIterator`

Order book iterator interface.

### `VectorOrderBookIterator`

In-memory order book iterator.

### `MergedOrderBookIterator`

Merged iterator for order book streams.

### `PostgresDbClient`

PostgreSQL-backed DB client.

### `Quote`

Best bid/ask quote snapshot type.

### `ValidationAction`

Validation action enum for data ingestion.

## Method Details

Type Hints:

- `symbol` → `SymbolId`
- `symbols` → `std::vector<SymbolId>`
- `range` → `TimeRange`
- `bar_type` → `BarType`
- `bar` → `Bar`
- `tick` → `Tick`
- `book` → `OrderBook`
- `config` → data source config struct

### `DataSource`

#### `get_available_symbols()`
Parameters: None.
Returns: Vector of symbols.
Throws: None.

#### `get_available_range(symbol)`
Parameters: `symbol` ID.
Returns: `TimeRange`.
Throws: None.

#### `get_bars(symbol, range, bar_type)`
Parameters: symbol, range, bar type.
Returns: Vector of bars.
Throws: None.

#### `get_ticks(symbol, range)`
Parameters: symbol, range.
Returns: Vector of ticks.
Throws: None.

#### `get_order_books(symbol, range)`
Parameters: symbol, range.
Returns: Vector of order books.
Throws: None.

#### `create_iterator(symbols, range, bar_type)`
Parameters: symbol list, range, bar type.
Returns: `DataIterator` pointer.
Throws: None.

#### `create_tick_iterator(symbols, range)`
Parameters: symbol list, range.
Returns: `TickIterator` pointer.
Throws: None.

#### `create_book_iterator(symbols, range)`
Parameters: symbol list, range.
Returns: `OrderBookIterator` pointer.
Throws: None.

#### `get_corporate_actions(symbol, range)`
Parameters: symbol, range.
Returns: Vector of corporate actions.
Throws: None.

### `DataIterator` / `TickIterator` / `OrderBookIterator`

#### `has_next()`
Parameters: None.
Returns: `bool`.
Throws: None.

#### `next()`
Parameters: None.
Returns: Next element.
Throws: None.

#### `reset()`
Parameters: None.
Returns: `void`.
Throws: None.

### `CSVDataSource` / `CSVTickDataSource`

#### `CSVDataSource(config)` / `CSVTickDataSource(config)`
Parameters: config.
Returns: Instance.
Throws: None.

#### `get_available_symbols()` / `get_available_range(symbol)`
Parameters: None or symbol.
Returns: symbol list / time range.
Throws: None.

#### `get_bars(...)` / `get_ticks(...)`
Parameters: symbol, range, bar type.
Returns: bars/ticks.
Throws: `Error::ParseError` for invalid CSV.

#### `create_iterator(...)` / `create_tick_iterator(...)`
Parameters: symbols, range, bar type.
Returns: iterator pointer.
Throws: None.

#### `get_corporate_actions(...)`
Parameters: symbol, range.
Returns: corporate actions.
Throws: None.

#### `last_report()`
Parameters: None.
Returns: `ValidationReport`.
Throws: None.

### `ApiDataSource`

#### `ApiDataSource(config)`
Parameters: API config.
Returns: Instance.
Throws: None.

#### `get_available_symbols()` / `get_available_range(symbol)`
Parameters: None or symbol.
Returns: symbol list / time range.
Throws: `Error::NetworkError` on API failure.

#### `get_bars(...)` / `get_ticks(...)`
Parameters: symbol, range, bar type.
Returns: bars/ticks.
Throws: `Error::NetworkError`/`Error::ParseError`.

#### `create_iterator(...)`
Parameters: symbols, range, bar type.
Returns: iterator pointer.
Throws: None.

#### `get_corporate_actions(...)`
Parameters: symbol, range.
Returns: corporate actions.
Throws: None.

### `MemoryDataSource`

#### `add_bars(...)` / `add_ticks(...)` / `add_order_books(...)`
Parameters: symbol, data.
Returns: `void`.
Throws: None.

#### `add_symbol_info(...)` / `set_corporate_actions(...)`
Parameters: metadata.
Returns: `void`.
Throws: None.

#### `get_available_symbols()` / `get_available_range(symbol)`
Parameters: None or symbol.
Returns: symbol list / time range.
Throws: None.

#### `get_bars(...)` / `get_ticks(...)` / `get_order_books(...)`
Parameters: symbol, range.
Returns: bars/ticks/books.
Throws: None.

#### `create_iterator(...)` / `create_tick_iterator(...)` / `create_book_iterator(...)`
Parameters: symbols, range, bar type.
Returns: iterator pointer.
Throws: None.

#### `get_corporate_actions(...)`
Parameters: symbol, range.
Returns: corporate actions.
Throws: None.

### `LiveFeed` / `PollingRestFeed`

#### `connect()` / `disconnect()` / `is_connected()`
Parameters: None.
Returns: `Result<void>` or `bool`.
Throws: `Error::InvalidState` for polling feeds when the data source is missing. Adapter implementations may also return `Error::NetworkError`.

#### `subscribe(symbols)` / `unsubscribe(symbols)`
Parameters: symbol list.
Returns: `void`.
Throws: None.

#### `on_bar(cb)` / `on_tick(cb)` / `on_book(cb)`
Parameters: callback.
Returns: `void`.
Throws: None.

#### `poll()`
Parameters: None.
Returns: `void`.
Throws: None.

### `WebSocketFeed`

#### `WebSocketFeed(config)`
Parameters: feed config.
Returns: Instance.
Throws: None.

#### `connect()` / `disconnect()` / `is_connected()`
Parameters: None.
Returns: `Result<void>` or `bool`.
Throws: `Error::InvalidArgument` for invalid URL or missing host, `Error::InvalidState` when the build lacks Boost.Beast/OpenSSL support, `Error::NetworkError` on connection failures.

#### `subscribe(symbols)` / `unsubscribe(symbols)`
Parameters: symbol list.
Returns: `void`.
Throws: None.

#### `on_bar(cb)` / `on_tick(cb)` / `on_book(cb)` / `on_raw(cb)` / `on_reconnect(cb)`
Parameters: callbacks.
Returns: `void`.
Throws: None.

#### `validate_tls_config()`
Parameters: None.
Returns: `Result<void>`.
Throws: `Error::InvalidArgument` for invalid CA bundle, `Error::InvalidState` when TLS support is unavailable.

#### `handle_message(message)`
Parameters: raw message.
Returns: `void`.
Throws: None.

#### `send_raw(message)`
Parameters: raw message.
Returns: `Result<void>`.
Throws: `Error::InvalidState` when not connected or the stream is uninitialized, `Error::InvalidArgument` for empty messages, `Error::NetworkError` on send failures.

#### `poll()`
Parameters: None.
Returns: `void`.
Throws: None.

### `OrderBookMmapDataSource`

#### `OrderBookMmapDataSource(config)`
Parameters: config.
Returns: Instance.
Throws: None.

#### `get_available_symbols()` / `get_available_range(symbol)`
Parameters: None or symbol.
Returns: symbols / time range.
Throws: None.

#### `get_order_books(symbol, range)`
Parameters: symbol, range.
Returns: order books.
Throws: None.

#### `create_book_iterator(symbols, range)`
Parameters: symbols, range.
Returns: iterator pointer.
Throws: None.

#### `set_corporate_actions(symbol, actions)`
Parameters: symbol, actions.
Returns: `void`.
Throws: None.

### `TickMmapDataSource`

#### `TickMmapDataSource(config)`
Parameters: config.
Returns: Instance.
Throws: None.

#### `get_available_symbols()` / `get_available_range(symbol)`
Parameters: None or symbol.
Returns: symbols / time range.
Throws: None.

#### `get_ticks(symbol, range)`
Parameters: symbol, range.
Returns: ticks.
Throws: None.

#### `create_tick_iterator(symbols, range)`
Parameters: symbols, range.
Returns: iterator pointer.
Throws: None.

#### `set_corporate_actions(symbol, actions)`
Parameters: symbol, actions.
Returns: `void`.
Throws: None.

### `MmapWriter` / `MmapStorage`

#### `MmapWriter::write_bars(path, symbol, bar_type, bars)`
Parameters: output path, symbol, bar type, bars.
Returns: `Result<void>`.
Throws: `Error::IoError` on write failure.

#### `MmapStorage(path)`
Parameters: path.
Returns: Instance.
Throws: None.

#### `open_read()`
Parameters: None.
Returns: `bool`.
Throws: None.

#### `read_bars(symbol, range)`
Parameters: symbol, range.
Returns: bars.
Throws: None.

### `SnapshotAccess`

#### `SnapshotAccess(source)`
Parameters: data source.
Returns: Instance.
Throws: None.

#### `bar_at(symbol, ts, bar_type)`
Parameters: symbol, timestamp, bar type.
Returns: Optional bar.
Throws: None.

#### `tick_at(symbol, ts)`
Parameters: symbol, timestamp.
Returns: Optional tick.
Throws: None.

#### `order_book_at(symbol, ts)`
Parameters: symbol, timestamp.
Returns: Optional order book.
Throws: None.

### `TimeSeriesQuery`

#### `TimeSeriesQuery(source)`
Parameters: data source.
Returns: Instance.
Throws: None.

#### `bars(symbol, range, bar_type)`
Parameters: symbol, range, bar type.
Returns: bars.
Throws: None.

#### `ticks(symbol, range)`
Parameters: symbol, range.
Returns: ticks.
Throws: None.

#### `order_books(symbol, range)`
Parameters: symbol, range.
Returns: order books.
Throws: None.

## Usage Examples

```cpp
#include \"regimeflow/data/csv_reader.h\"
#include \"regimeflow/data/data_source_factory.h\"
#include \"regimeflow/data/merged_iterator.h\"

regimeflow::data::CSVDataSource::Config cfg;
cfg.data_directory = \"./data\";
regimeflow::data::CSVDataSource source(cfg);

auto symbols = source.get_available_symbols();
auto it = source.create_iterator({symbols[0].id}, symbols[0].trading_hours,
                                 regimeflow::data::BarType::Time_1Day);
while (it->has_next()) {
    auto bar = it->next();
    (void)bar;
}
```

## See Also

- [Data Flow](../explanation/data-flow.md)
- [Data Validation](../how-to/data-validation.md)
- [Mmap Storage](../how-to/mmap-storage.md)
- [Symbol Metadata](../how-to/symbol-metadata.md)
