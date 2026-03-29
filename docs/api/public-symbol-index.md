# Public Symbol Index

This index enumerates the public symbol surface exported from `include/regimeflow/**`.
It complements the package reference pages by providing explicit symbol-level traceability for headers, types, and callable APIs.

## Summary

- Public headers indexed: `132`
- Public type declarations indexed: `324`
- Public callable declarations indexed: `1299`

Notes:

- This page is generated from the public headers and is intended as a completeness index.
- Narrative guidance still lives in the package pages and topical guides.

## `common`

### `regimeflow/common/config.h`

Types:
- `class ConfigValue`
- `class Config`

Callables:
- `ConfigValue() = default;`
- `ConfigValue(bool v) : value_(v)}`
- `ConfigValue(int64_t v) : value_(v)}`
- `ConfigValue(double v) : value_(v)}`
- `ConfigValue(std::string v) : value_(std::move(v))}`
- `ConfigValue(const char* v) : value_(std::string(v))}`
- `ConfigValue(Array v) : value_(std::move(v))}`
- `ConfigValue(Object v) : value_(std::move(v))}`
- `const Value& raw() const return value_; }`
- `const T* get_if() const return std::get_if<T>(&value_); }`
- `T* get_if() return std::get_if<T>(&value_); }`
- `Config() = default;`
- `explicit Config(Object values) : values_(std::move(values))}`
- `bool has(const std::string& key) const;`
- `const ConfigValue* get(const std::string& key) const;`
- `const ConfigValue* get_path(const std::string& path) const;`
- `std::optional<T> get_as(const std::string& key) const`
- `const auto* value = get(key);`
- `value = get_path(key);`
- `void set(std::string key, ConfigValue value);`
- `void set_path(const std::string& path, ConfigValue value);`

### `regimeflow/common/config_schema.h`

Types:
- `struct ConfigProperty`
- `struct ConfigSchema`

Callables:
- `inline bool config_value_matches(const ConfigValue& value, const std::string& type)`
- `inline Result<void> validate_config(const Config& config, const ConfigSchema& schema)`
- `const auto* value = config.get_path(key);`
- `inline Config apply_defaults(const Config& input, const ConfigSchema& schema)`
- `output.set_path(key, *prop.default_value);`

### `regimeflow/common/json.h`

Types:
- `class JsonValue`

Callables:
- `JsonValue() = default;`
- `explicit JsonValue(std::nullptr_t) : value_(std::monostate{})}`
- `explicit JsonValue(bool v) : value_(v)}`
- `explicit JsonValue(double v) : value_(v)}`
- `explicit JsonValue(std::string v) : value_(std::move(v))}`
- `explicit JsonValue(Array v) : value_(std::make_shared<Array>(std::move(v)))}`
- `explicit JsonValue(Object v) : value_(std::make_shared<Object>(std::move(v)))}`
- `[[nodiscard]] bool is_null() const return std::holds_alternative<std::monostate>(value_); }`
- `[[nodiscard]] bool is_bool() const return std::holds_alternative<bool>(value_); }`
- `[[nodiscard]] bool is_number() const return std::holds_alternative<double>(value_); }`
- `[[nodiscard]] bool is_string() const return std::holds_alternative<std::string>(value_); }`
- `[[nodiscard]] bool is_array() const return std::holds_alternative<ArrayPtr>(value_); }`
- `[[nodiscard]] bool is_object() const return std::holds_alternative<ObjectPtr>(value_); }`
- `[[nodiscard]] const bool* as_bool() const return std::get_if<bool>(&value_); }`
- `[[nodiscard]] const double* as_number() const return std::get_if<double>(&value_); }`
- `[[nodiscard]] const std::string* as_string() const return std::get_if<std::string>(&value_); }`
- `[[nodiscard]] const Array* as_array() const`
- `[[nodiscard]] const Object* as_object() const`
- `Result<JsonValue> parse_json(std::string_view input);`

### `regimeflow/common/lru_cache.h`

Types:
- `class LRUCache`

Callables:
- `explicit LRUCache(size_t capacity) : capacity_(capacity)}`
- `void set_capacity(size_t capacity)`
- `evict_if_needed();`
- `[[nodiscard]] size_t capacity() const return capacity_; }`
- `[[nodiscard]] size_t size() const return items_.size(); }`
- `std::optional<Value> get(const Key& key)`
- `auto it = map_.find(key);`
- `items_.splice(items_.begin(), items_, it->second);`
- `void put(const Key& key, Value value)`
- `it->second->second = std::move(value);`
- `items_.emplace_front(key, std::move(value));`
- `map_[items_.front().first] = items_.begin();`
- `void clear()`
- `items_.clear();`
- `map_.clear();`

### `regimeflow/common/memory.h`

Types:
- `class MonotonicArena`
- `class PoolAllocator`

Callables:
- `explicit MonotonicArena(size_t block_size = 1 << 20) : block_size_(block_size)`
- `blocks_.emplace_back(std::make_unique<uint8_t[]>(block_size_));`
- `void* allocate(size_t bytes, size_t alignment = alignof(std::max_align_t))`
- `size_t aligned = (current + alignment - 1) & ~(alignment - 1);`
- `void* ptr = blocks_.back().get() + aligned;`
- `void reset()`
- `blocks_.resize(1);`
- `explicit PoolAllocator(size_t capacity = 1024)`
- `reserve(capacity);`
- `T* allocate()`
- `std::lock_guard<std::mutex> lock(mutex_);`
- `reserve(chunks_.empty() ? 1024 : chunks_.size() * 2 * chunk_size_);`
- `T* ptr = free_.back();`
- `free_.pop_back();`
- `void deallocate(T* ptr)`
- `free_.push_back(ptr);`

### `regimeflow/common/mpsc_queue.h`

Types:
- `class MpscQueue`

Callables:
- `MpscQueue()`
- `Node* dummy = new Node();`
- `head_.store(dummy, std::memory_order_relaxed);`
- `tail_.store(dummy, std::memory_order_relaxed);`
- `~MpscQueue()`
- `Node* node = head_.load(std::memory_order_relaxed);`
- `MpscQueue(const MpscQueue&) = delete;`
- `MpscQueue& operator=(const MpscQueue&) = delete;`
- `void push(const T& value)`
- `Node* node = new Node(value);`
- `Node* prev = tail_.exchange(node, std::memory_order_acq_rel);`
- `prev->next.store(node, std::memory_order_release);`
- `void push(T&& value)`
- `Node* node = new Node(std::move(value));`
- `bool pop(T& out)`
- `Node* head = head_.load(std::memory_order_acquire);`
- `Node* next = head->next.load(std::memory_order_acquire);`
- `out = std::move(next->value);`
- `head_.store(next, std::memory_order_release);`
- `[[nodiscard]] bool empty() const`
- `Node() = default;`
- `explicit Node(T v) : value(std::move(v))}`

### `regimeflow/common/result.h`

Types:
- `struct Error`
- `enum class Code`
- `class Result`

Callables:
- `Error() : code(Code::Unknown), message(), details(std::nullopt), location(std::source_location::current())}`
- `Error(Code c, std::string msg, std::source_location loc = std::source_location::current()) : code(c), message(std::move(msg)), location(loc)}`
- `Error(const Error& other) : code(other.code), message(other.message), details(other.details), location(other.location)}`
- `Error& operator=(const Error& other)`
- `Error(Error&&) noexcept = default;`
- `Error& operator=(Error&&) noexcept = default;`
- `[[nodiscard]] std::string to_string() const`
- `out << "[" << static_cast<int>(code) << "] " << message << " (at " << location.file_name() << ":" << location.line() << ")";`
- `explicit Result(T value) : value_(std::move(value))}`
- `explicit Result(Error error) : value_(std::move(error))}`
- `[[nodiscard]] bool is_ok() const return std::holds_alternative<T>(value_); }`
- `[[nodiscard]] bool is_err() const return std::holds_alternative<Error>(value_); }`
- `T& value() &`
- `const T& value() const&`
- `T&& value() &&`
- `Error& error() & return std::get<Error>(value_); }`
- `[[nodiscard]] const Error& error() const& return std::get<Error>(value_); }`
- `auto map(F&& f) -> Result<std::invoke_result_t<F, T>>`
- `auto and_then(F&& f) -> std::invoke_result_t<F, T>`
- `T value_or(T default_value) const`
- `Result() = default;`
- `explicit Result(Error error) : error_(std::move(error))}`
- `[[nodiscard]] bool is_ok() const return !error_.has_value(); }`
- `[[nodiscard]] bool is_err() const return error_.has_value(); }`
- `[[nodiscard]] const Error& error() const return *error_; }`
- `Result<T> Ok(T value) return Result<T>(std::move(value)); }`
- `inline Result<void> Ok() return}; }`
- `Error Err(Error::Code code, std::string_view fmt, Args&&... args)`
- `Error Err(const Error::Code code, const std::string_view fmt, Args&&...)`
- `({ \`

### `regimeflow/common/sha256.h`

Types:
- `class Sha256`

Callables:
- `Sha256();`
- `void update(const void* data, size_t len);`
- `std::array<uint8_t, 32> digest();`

### `regimeflow/common/spsc_queue.h`

Types:
- `class SpscQueue`

Callables:
- `bool push(const T& value)`
- `const size_t head = head_.load(std::memory_order_relaxed);`
- `const size_t next = increment(head);`
- `head_.store(next, std::memory_order_release);`
- `bool pop(T& out)`
- `const size_t tail = tail_.load(std::memory_order_relaxed);`
- `tail_.store(increment(tail), std::memory_order_release);`
- `[[nodiscard]] bool empty() const`

### `regimeflow/common/time.h`

Types:
- `class Duration`
- `class Timestamp`

Callables:
- `static Duration microseconds(int64_t us) return Duration(us); }`
- `static Duration milliseconds(int64_t ms) return Duration(ms * 1000); }`
- `static Duration seconds(int64_t s) return Duration(s * 1'000'000); }`
- `static Duration minutes(int64_t m) return seconds(m * 60); }`
- `static Duration hours(int64_t h) return minutes(h * 60); }`
- `static Duration days(int64_t d) return hours(d * 24); }`
- `static Duration months(int m) return days(static_cast<int64_t>(m) * 30); }`
- `[[nodiscard]] int64_t total_microseconds() const return us_; }`
- `[[nodiscard]] int64_t total_milliseconds() const return us_ / 1000; }`
- `[[nodiscard]] int64_t total_seconds() const return us_ / 1'000'000; }`
- `Timestamp() = default;`
- `explicit Timestamp(const int64_t microseconds) : us_(microseconds)}`
- `static Timestamp now();`
- `static Timestamp from_string(const std::string& str, const std::string& fmt);`
- `static Timestamp from_date(int year, int month, int day);`
- `[[nodiscard]] int64_t microseconds() const return us_; }`
- `[[nodiscard]] int64_t milliseconds() const return us_ / 1000; }`
- `[[nodiscard]] int64_t seconds() const return us_ / 1'000'000; }`
- `[[nodiscard]] std::string to_string(const std::string& fmt = "%Y-%m-%d %H:%M:%S") const;`
- `auto operator<=>(const Timestamp& other) const = default;`
- `Timestamp operator+(Duration d) const;`
- `Timestamp operator-(Duration d) const;`
- `Duration operator-(const Timestamp& other) const;`

### `regimeflow/common/types.h`

Types:
- `enum class AssetClass`
- `class SymbolRegistry`
- `struct TimeRange`

Callables:
- `static SymbolRegistry& instance();`
- `SymbolId intern(std::string_view symbol);`
- `const std::string& lookup(SymbolId id) const;`
- `[[nodiscard]] bool contains(const Timestamp t) const return t >= start && t <= end; }`
- `[[nodiscard]] Duration duration() const return end - start; }`

### `regimeflow/common/yaml_config.h`

Types:
- `class YamlConfigLoader`

Callables:
- `static Config load_file(const std::string& path);`

## `data`

### `regimeflow/data/alpaca_data_client.h`

Types:
- `class AlpacaDataClient`
- `struct Config`

Callables:
- `explicit AlpacaDataClient(Config config);`
- `[[nodiscard]] Result<std::string> list_assets(const std::string& status = "active", const std::string& asset_class = "us_equity") const;`
- `[[nodiscard]] Result<std::string> get_bars(const std::vector<std::string>& symbols, const std::string& timeframe, const std::string& start, const std::string& end, int limit = 0, const std::string& page_token = "") const;`
- `[[nodiscard]] Result<std::string> get_trades(const std::vector<std::string>& symbols, const std::string& start, const std::string& end, int limit = 0, const std::string& page_token = "") const;`
- `[[nodiscard]] Result<std::string> get_snapshot(const std::string& symbol) const;`

### `regimeflow/data/alpaca_data_source.h`

Types:
- `class AlpacaDataSource`
- `struct Config`

Callables:
- `explicit AlpacaDataSource(Config config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`

### `regimeflow/data/api_data_source.h`

Types:
- `class ApiDataSource`
- `struct Config`

Callables:
- `explicit ApiDataSource(Config config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `const ValidationReport& last_report() const return last_report_; }`

### `regimeflow/data/bar.h`

Types:
- `struct Bar`
- `enum class BarType`

Callables:
- `[[nodiscard]] Price mid() const return (high + low) / 2; }`
- `[[nodiscard]] Price typical() const return (high + low + close) / 3; }`
- `[[nodiscard]] Price range() const return high - low; }`
- `[[nodiscard]] bool is_bullish() const return close > open; }`
- `[[nodiscard]] bool is_bearish() const return close < open; }`

### `regimeflow/data/bar_builder.h`

Types:
- `class BarBuilder`
- `struct Config`
- `class MultiSymbolBarBuilder`

Callables:
- `explicit BarBuilder(const Config& config);`
- `std::optional<Bar> process(const Tick& tick);`
- `std::optional<Bar> flush();`
- `void reset();`
- `explicit MultiSymbolBarBuilder(const BarBuilder::Config& config);`
- `std::vector<Bar> flush_all();`

### `regimeflow/data/corporate_actions.h`

Types:
- `enum class CorporateActionType`
- `struct CorporateAction`
- `class CorporateActionAdjuster`

Callables:
- `void add_actions(SymbolId symbol, std::vector<CorporateAction> actions);`
- `[[nodiscard]] Bar adjust_bar(SymbolId symbol, const Bar& bar) const;`
- `[[nodiscard]] SymbolId resolve_symbol(SymbolId symbol) const;`
- `[[nodiscard]] SymbolId resolve_symbol(SymbolId symbol, Timestamp at) const;`
- `[[nodiscard]] std::vector<SymbolId> aliases_for(SymbolId symbol) const;`

### `regimeflow/data/csv_reader.h`

Types:
- `class CSVDataSource`
- `struct Config`

Callables:
- `explicit CSVDataSource(const Config& config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `const ValidationReport& last_report() const return last_report_; }`
- `void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions) const;`

### `regimeflow/data/data_source.h`

Types:
- `struct SymbolInfo`
- `class DataIterator`
- `class TickIterator`
- `class OrderBookIterator`
- `class DataSource`

Callables:
- `virtual ~DataIterator() = default;`
- `[[nodiscard]] virtual bool has_next() const = 0;`
- `virtual Bar next() = 0;`
- `virtual void reset() = 0;`
- `virtual ~TickIterator() = default;`
- `virtual Tick next() = 0;`
- `virtual ~OrderBookIterator() = default;`
- `virtual OrderBook next() = 0;`
- `virtual ~DataSource() = default;`
- `[[nodiscard]] virtual std::vector<SymbolInfo> get_available_symbols() const = 0;`
- `[[nodiscard]] virtual TimeRange get_available_range(SymbolId symbol) const = 0;`
- `virtual std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) = 0;`
- `virtual std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) = 0;`
- `virtual std::vector<OrderBook> get_order_books([[maybe_unused]] SymbolId symbol, [[maybe_unused]] TimeRange range)`
- `virtual std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) = 0;`
- `virtual std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>&, TimeRange)`
- `virtual std::unique_ptr<OrderBookIterator> create_book_iterator( const std::vector<SymbolId>&, TimeRange)`
- `virtual std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) = 0;`

### `regimeflow/data/data_source_factory.h`

Types:
- `class DataSourceFactory`

Callables:
- `static std::unique_ptr<DataSource> create(const Config& config);`

### `regimeflow/data/data_validation.h`

Types:
- `enum class ValidationSeverity`
- `struct ValidationIssue`
- `class ValidationReport`

Callables:
- `void add_issue(ValidationIssue issue);`
- `[[nodiscard]] bool ok() const return errors_ == 0; }`
- `[[nodiscard]] int error_count() const return errors_; }`
- `[[nodiscard]] int warning_count() const return warnings_; }`
- `[[nodiscard]] const std::vector<ValidationIssue>& issues() const return issues_; }`
- `[[nodiscard]] std::string summary() const;`

### `regimeflow/data/db_client.h`

Types:
- `class DbClient`
- `class InMemoryDbClient`

Callables:
- `virtual ~DbClient() = default;`
- `virtual std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) = 0;`
- `virtual std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) = 0;`
- `virtual std::vector<SymbolInfo> list_symbols() = 0;`
- `virtual std::vector<SymbolInfo> list_symbols_with_metadata(const std::string& symbols_table)`
- `(void)symbols_table;`
- `virtual TimeRange get_available_range(SymbolId symbol) = 0;`
- `virtual std::vector<CorporateAction> query_corporate_actions(SymbolId symbol, TimeRange range) = 0;`
- `virtual std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) = 0;`
- `void add_bars(SymbolId symbol, std::vector<Bar> bars);`
- `void add_ticks(SymbolId symbol, std::vector<Tick> ticks);`
- `void add_symbol_info(SymbolInfo info);`
- `void add_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);`
- `void add_order_books(SymbolId symbol, std::vector<OrderBook> books);`
- `std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;`
- `std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<SymbolInfo> list_symbols() override;`
- `TimeRange get_available_range(SymbolId symbol) override;`
- `std::vector<CorporateAction> query_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;`

### `regimeflow/data/db_csv_adapter.h`

Types:
- `class CsvDbClient`

Callables:
- `explicit CsvDbClient(CSVDataSource source);`
- `std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;`
- `std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<SymbolInfo> list_symbols() override;`
- `TimeRange get_available_range(SymbolId symbol) override;`
- `std::vector<CorporateAction> query_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;`

### `regimeflow/data/db_source.h`

Types:
- `class DatabaseDataSource`
- `struct Config`

Callables:
- `explicit DatabaseDataSource(const Config& config);`
- `void set_client(std::shared_ptr<DbClient> client);`
- `void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::unique_ptr<OrderBookIterator> create_book_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `const ValidationReport& last_report() const return last_report_; }`

### `regimeflow/data/live_feed.h`

Types:
- `class LiveFeedAdapter`
- `class PollingRestFeed`
- `struct Config`

Callables:
- `virtual ~LiveFeedAdapter() = default;`
- `virtual Result<void> connect() = 0;`
- `virtual void disconnect() = 0;`
- `virtual bool is_connected() const = 0;`
- `virtual void subscribe(const std::vector<std::string>& symbols) = 0;`
- `virtual void unsubscribe(const std::vector<std::string>& symbols) = 0;`
- `virtual void on_bar(std::function<void(const Bar&)> cb) = 0;`
- `virtual void on_tick(std::function<void(const Tick&)> cb) = 0;`
- `virtual void on_book(std::function<void(const OrderBook&)> cb) = 0;`
- `virtual void poll() = 0;`
- `explicit PollingRestFeed(Config config);`
- `Result<void> connect() override;`
- `void disconnect() override;`
- `bool is_connected() const override;`
- `void subscribe(const std::vector<std::string>& symbols) override;`
- `void unsubscribe(const std::vector<std::string>& symbols) override;`
- `void on_bar(std::function<void(const Bar&)> cb) override;`
- `void on_tick(std::function<void(const Tick&)> cb) override;`
- `void on_book(std::function<void(const OrderBook&)> cb) override;`
- `void poll() override;`

### `regimeflow/data/memory_data_source.h`

Types:
- `class VectorBarIterator`
- `class VectorOrderBookIterator`
- `class VectorTickIterator`
- `class MemoryDataSource`

Callables:
- `explicit VectorBarIterator(std::vector<Bar> bars);`
- `[[nodiscard]] bool has_next() const override;`
- `Bar next() override;`
- `void reset() override;`
- `explicit VectorOrderBookIterator(std::vector<OrderBook> books);`
- `OrderBook next() override;`
- `explicit VectorTickIterator(std::vector<Tick> ticks);`
- `Tick next() override;`
- `void add_bars(SymbolId symbol, std::vector<Bar> bars);`
- `void add_ticks(SymbolId symbol, std::vector<Tick> ticks);`
- `void add_order_books(SymbolId symbol, std::vector<OrderBook> books);`
- `void add_symbol_info(SymbolInfo info);`
- `void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);`
- `[[nodiscard]] std::vector<SymbolInfo> get_available_symbols() const override;`
- `[[nodiscard]] TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::unique_ptr<OrderBookIterator> create_book_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`

### `regimeflow/data/merged_iterator.h`

Types:
- `class MergedBarIterator`
- `class MergedTickIterator`
- `class MergedOrderBookIterator`

Callables:
- `explicit MergedBarIterator(std::vector<std::unique_ptr<DataIterator>> iterators);`
- `[[nodiscard]] bool has_next() const override;`
- `Bar next() override;`
- `void reset() override;`
- `bool operator()(const HeapEntry& lhs, const HeapEntry& rhs) const`
- `explicit MergedTickIterator(std::vector<std::unique_ptr<TickIterator>> iterators);`
- `Tick next() override;`
- `explicit MergedOrderBookIterator(std::vector<std::unique_ptr<OrderBookIterator>> iterators);`
- `OrderBook next() override;`

### `regimeflow/data/metadata_data_source.h`

Types:
- `class MetadataOverlayDataSource`

Callables:
- `MetadataOverlayDataSource(std::unique_ptr<DataSource> inner, SymbolMetadataMap csv_metadata, SymbolMetadataMap config_metadata);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::unique_ptr<OrderBookIterator> create_book_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`

### `regimeflow/data/mmap_data_source.h`

Types:
- `class MemoryMappedDataSource`
- `struct Config`

Callables:
- `explicit MemoryMappedDataSource(const Config& config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);`

### `regimeflow/data/mmap_reader.h`

Types:
- `struct FileHeader`
- `struct DateIndex`
- `class MemoryMappedDataFile`
- `class BarView`
- `class Iterator`

Callables:
- `explicit MemoryMappedDataFile(const std::string& path);`
- `~MemoryMappedDataFile();`
- `MemoryMappedDataFile(const MemoryMappedDataFile&) = delete;`
- `MemoryMappedDataFile& operator=(const MemoryMappedDataFile&) = delete;`
- `MemoryMappedDataFile(MemoryMappedDataFile&& other) noexcept;`
- `MemoryMappedDataFile& operator=(MemoryMappedDataFile&& other) noexcept;`
- `[[nodiscard]] const FileHeader& header() const;`
- `[[nodiscard]] std::string symbol() const;`
- `[[nodiscard]] SymbolId symbol_id() const;`
- `[[nodiscard]] TimeRange time_range() const;`
- `[[nodiscard]] size_t bar_count() const;`
- `BarView(const MemoryMappedDataFile* file, size_t index);`
- `[[nodiscard]] Timestamp timestamp() const;`
- `[[nodiscard]] double open() const;`
- `[[nodiscard]] double high() const;`
- `[[nodiscard]] double low() const;`
- `[[nodiscard]] double close() const;`
- `[[nodiscard]] uint64_t volume() const;`
- `[[nodiscard]] Bar to_bar() const;`
- `BarView operator[](size_t index) const;`
- `[[nodiscard]] BarView at(size_t index) const;`
- `Iterator(const MemoryMappedDataFile* file, size_t index);`
- `BarView operator*() const;`
- `Iterator& operator++();`
- `bool operator!=(const Iterator& other) const;`
- `[[nodiscard]] Iterator begin() const;`
- `[[nodiscard]] Iterator end() const;`
- `[[nodiscard]] std::pair<size_t, size_t> find_range(TimeRange range) const;`
- `[[nodiscard]] std::span<const int64_t> timestamps() const;`
- `[[nodiscard]] std::span<const double> opens() const;`
- `[[nodiscard]] std::span<const double> highs() const;`
- `[[nodiscard]] std::span<const double> lows() const;`
- `[[nodiscard]] std::span<const double> closes() const;`
- `[[nodiscard]] std::span<const uint64_t> volumes() const;`
- `[[nodiscard]] size_t date_index_count() const return index_count_; }`
- `void preload_index() const;`

### `regimeflow/data/mmap_storage.h`

Types:
- `class MmapStorage`

Callables:
- `explicit MmapStorage(std::string path);`
- `bool open_read();`
- `[[nodiscard]] std::vector<Bar> read_bars(SymbolId symbol, TimeRange range) const;`

### `regimeflow/data/mmap_writer.h`

Types:
- `class MmapWriter`

Callables:
- `Result<void> write_bars(const std::string& path, const std::string& symbol, BarType bar_type, std::vector<Bar> bars);`

### `regimeflow/data/order_book.h`

Types:
- `struct BookLevel`
- `struct OrderBook`

Callables:
- None detected

### `regimeflow/data/order_book_mmap.h`

Types:
- `struct BookFileHeader`
- `struct BookDateIndex`
- `class OrderBookMmapFile`
- `class OrderBookMmapWriter`

Callables:
- `explicit OrderBookMmapFile(const std::string& path);`
- `~OrderBookMmapFile();`
- `OrderBookMmapFile(const OrderBookMmapFile&) = delete;`
- `OrderBookMmapFile& operator=(const OrderBookMmapFile&) = delete;`
- `OrderBookMmapFile(OrderBookMmapFile&& other) noexcept;`
- `OrderBookMmapFile& operator=(OrderBookMmapFile&& other) noexcept;`
- `[[nodiscard]] const BookFileHeader& header() const;`
- `[[nodiscard]] std::string symbol() const;`
- `[[nodiscard]] SymbolId symbol_id() const;`
- `[[nodiscard]] TimeRange time_range() const;`
- `[[nodiscard]] size_t book_count() const;`
- `[[nodiscard]] OrderBook at(size_t index) const;`
- `[[nodiscard]] std::pair<size_t, size_t> find_range(TimeRange range) const;`
- `Result<void> write_books(const std::string& path, const std::string& symbol, std::vector<OrderBook> books);`

### `regimeflow/data/order_book_mmap_data_source.h`

Types:
- `class OrderBookMmapDataSource`
- `struct Config`

Callables:
- `explicit OrderBookMmapDataSource(const Config& config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::unique_ptr<OrderBookIterator> create_book_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);`

### `regimeflow/data/postgres_client.h`

Types:
- `class ConnectionPool`
- `class PostgresDbClient`
- `struct Config`

Callables:
- `ConnectionPool(std::string connection_string, int size);`
- `~ConnectionPool();`
- `ConnectionPool(const ConnectionPool&) = delete;`
- `ConnectionPool& operator=(const ConnectionPool&) = delete;`
- `void* acquire();`
- `void release(void* connection);`
- `explicit PostgresDbClient(Config config);`
- `std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;`
- `std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;`
- `std::vector<SymbolInfo> list_symbols() override;`
- `std::vector<SymbolInfo> list_symbols_with_metadata(const std::string& symbols_table) override;`
- `TimeRange get_available_range(SymbolId symbol) override;`
- `std::vector<CorporateAction> query_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;`

### `regimeflow/data/snapshot_access.h`

Types:
- `class SnapshotAccess`

Callables:
- `explicit SnapshotAccess(std::shared_ptr<DataSource> source);`
- `std::optional<Bar> bar_at(SymbolId symbol, Timestamp ts, BarType bar_type) const;`
- `std::optional<Tick> tick_at(SymbolId symbol, Timestamp ts) const;`
- `std::optional<OrderBook> order_book_at(SymbolId symbol, Timestamp ts) const;`

### `regimeflow/data/symbol_metadata.h`

Types:
- `struct SymbolMetadata`

Callables:
- `SymbolMetadataMap load_symbol_metadata_csv(const std::string& path, char delimiter = ',', bool has_header = true);`
- `SymbolMetadataMap load_symbol_metadata_config(const Config& config, const std::string& key = "symbol_metadata");`
- `SymbolMetadataMap metadata_from_symbols(const std::vector<SymbolInfo>& symbols);`
- `void apply_symbol_metadata(std::vector<SymbolInfo>& symbols, const SymbolMetadataMap& metadata, bool overwrite = true);`

### `regimeflow/data/tick.h`

Types:
- `struct Tick`
- `struct Quote`

Callables:
- `[[nodiscard]] Price mid() const return (bid + ask) / 2; }`
- `[[nodiscard]] Price spread() const return ask - bid; }`
- `[[nodiscard]] Price spread_bps() const return spread() / mid() * 10000; }`

### `regimeflow/data/tick_csv_reader.h`

Types:
- `class CSVTickDataSource`
- `struct Config`

Callables:
- `explicit CSVTickDataSource(const Config& config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `const ValidationReport& last_report() const return last_report_; }`

### `regimeflow/data/tick_mmap.h`

Types:
- `struct TickFileHeader`
- `struct TickDateIndex`
- `class TickMmapFile`
- `class TickView`
- `class TickMmapWriter`

Callables:
- `explicit TickMmapFile(const std::string& path);`
- `~TickMmapFile();`
- `TickMmapFile(const TickMmapFile&) = delete;`
- `TickMmapFile& operator=(const TickMmapFile&) = delete;`
- `TickMmapFile(TickMmapFile&& other) noexcept;`
- `TickMmapFile& operator=(TickMmapFile&& other) noexcept;`
- `[[nodiscard]] const TickFileHeader& header() const;`
- `[[nodiscard]] std::string symbol() const;`
- `[[nodiscard]] SymbolId symbol_id() const;`
- `[[nodiscard]] TimeRange time_range() const;`
- `[[nodiscard]] size_t tick_count() const;`
- `TickView(const TickMmapFile* file, size_t index);`
- `[[nodiscard]] Timestamp timestamp() const;`
- `[[nodiscard]] double price() const;`
- `[[nodiscard]] double quantity() const;`
- `[[nodiscard]] uint8_t flags() const;`
- `[[nodiscard]] Tick to_tick() const;`
- `TickView operator[](size_t index) const;`
- `[[nodiscard]] TickView at(size_t index) const;`
- `[[nodiscard]] std::pair<size_t, size_t> find_range(TimeRange range) const;`
- `[[nodiscard]] std::span<const int64_t> timestamps() const;`
- `[[nodiscard]] std::span<const double> prices() const;`
- `[[nodiscard]] std::span<const double> quantities() const;`
- `[[nodiscard]] std::span<const uint32_t> flags() const;`
- `Result<void> write_ticks(const std::string& path, const std::string& symbol, std::vector<Tick> ticks);`

### `regimeflow/data/tick_mmap_data_source.h`

Types:
- `class TickMmapDataSource`
- `struct Config`

Callables:
- `explicit TickMmapDataSource(const Config& config);`
- `std::vector<SymbolInfo> get_available_symbols() const override;`
- `TimeRange get_available_range(SymbolId symbol) const override;`
- `std::vector<Bar> get_bars(SymbolId symbol, TimeRange range, BarType bar_type = BarType::Time_1Day) override;`
- `std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;`
- `std::unique_ptr<DataIterator> create_iterator( const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) override;`
- `std::unique_ptr<TickIterator> create_tick_iterator( const std::vector<SymbolId>& symbols, TimeRange range) override;`
- `std::vector<CorporateAction> get_corporate_actions(SymbolId symbol, TimeRange range) override;`
- `void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);`

### `regimeflow/data/time_series_query.h`

Types:
- `class TimeSeriesQuery`

Callables:
- `explicit TimeSeriesQuery(std::shared_ptr<DataSource> source);`
- `std::vector<Bar> bars(SymbolId symbol, TimeRange range, BarType bar_type) const;`
- `std::vector<Tick> ticks(SymbolId symbol, TimeRange range) const;`
- `std::vector<OrderBook> order_books(SymbolId symbol, TimeRange range) const;`

### `regimeflow/data/validation_config.h`

Types:
- `enum class ValidationAction`
- `struct ValidationConfig`

Callables:
- `Duration max_gap = Duration::days(2);`
- `Duration max_future_skew = Duration::seconds(0);`

### `regimeflow/data/validation_utils.h`

Types:
- None detected

Callables:
- `std::optional<Duration> bar_interval_for(BarType bar_type);`
- `std::vector<Bar> fill_missing_time_bars(const std::vector<Bar>& bars, Duration interval);`
- `std::vector<Bar> validate_bars(std::vector<Bar> bars, BarType bar_type, const ValidationConfig& config, bool fill_missing_bars, bool collect_report, ValidationReport* report);`
- `std::vector<Tick> validate_ticks(std::vector<Tick> ticks, const ValidationConfig& config, bool collect_report, ValidationReport* report);`

### `regimeflow/data/websocket_feed.h`

Types:
- `class WebSocketFeed`
- `struct ReconnectState`
- `struct Config`

Callables:
- `std::function<Result<void>()> connect_override;`
- `explicit WebSocketFeed(Config config);`
- `Result<void> connect() override;`
- `void disconnect() override;`
- `bool is_connected() const override;`
- `void subscribe(const std::vector<std::string>& symbols) override;`
- `void unsubscribe(const std::vector<std::string>& symbols) override;`
- `void on_bar(std::function<void(const Bar&)> cb) override;`
- `void on_tick(std::function<void(const Tick&)> cb) override;`
- `void on_book(std::function<void(const OrderBook&)> cb) override;`
- `void on_raw(std::function<void(const std::string&)> cb);`
- `void on_reconnect(std::function<void(const ReconnectState&)> cb);`
- `Result<void> validate_tls_config() const;`
- `void handle_message(const std::string& message);`
- `Result<void> send_raw(const std::string& message);`
- `void poll() override;`
- `void push(double value)`
- `mean += delta / static_cast<double>(count);`
- `[[nodiscard]] double stddev() const`

## `engine`

### `regimeflow/engine/audit_log.h`

Types:
- `struct AuditEvent`
- `enum class Type`
- `class AuditLogger`

Callables:
- `explicit AuditLogger(std::string path);`
- `Result<void> log(const AuditEvent& event);`
- `Result<void> log_error(const std::string& error);`
- `Result<void> log_regime_change(const regime::RegimeTransition& transition);`
- `static std::string type_to_string(AuditEvent::Type type);`

### `regimeflow/engine/backtest_engine.h`

Types:
- `struct ParallelContext`
- `class BacktestEngine`
- `enum class MarginCallAction`
- `enum class StopOutAction`
- `enum class TickSimulationMode`
- `enum class SyntheticTickProfile`

Callables:
- `explicit BacktestEngine(double initial_capital = 0.0, std::string currency = "USD");`
- `events::EventQueue& event_queue() return event_queue_; }`
- `events::EventDispatcher& dispatcher() return dispatcher_; }`
- `EventLoop& event_loop() return event_loop_; }`
- `OrderManager& order_manager() return order_manager_; }`
- `Portfolio& portfolio() return portfolio_; }`
- `MarketDataCache& market_data() return market_data_; }`
- `void enqueue(events::Event event);`
- `void load_data(std::unique_ptr<data::DataIterator> iterator);`
- `void load_data(std::unique_ptr<data::DataIterator> bar_iterator, std::unique_ptr<data::TickIterator> tick_iterator, std::unique_ptr<data::OrderBookIterator> book_iterator);`
- `void set_strategy(std::unique_ptr<strategy::Strategy> strategy, Config config =});`
- `void add_strategy(std::unique_ptr<strategy::Strategy> strategy);`
- `void set_execution_model(std::unique_ptr<execution::ExecutionModel> model);`
- `void set_commission_model(std::unique_ptr<execution::CommissionModel> model);`
- `void set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model);`
- `void set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model);`
- `void set_latency_model(std::unique_ptr<execution::LatencyModel> model);`
- `void set_regime_detector(std::unique_ptr<regime::RegimeDetector> detector);`
- `risk::RiskManager& risk_manager() return risk_manager_; }`
- `metrics::MetricsTracker& metrics() return metrics_; }`
- `const regime::RegimeState& current_regime() const return regime_tracker_.current_state(); }`
- `Timestamp current_time() const return event_loop_.current_time(); }`
- `plugins::HookSystem& hooks() return hooks_; }`
- `plugins::HookManager& hook_manager() return hook_manager_; }`
- `void configure_execution(const Config& config);`
- `void configure_risk(const Config& config);`
- `void configure_regime(const Config& config);`
- `void set_parallel_context(ParallelContext context);`
- `void set_dashboard_setup(DashboardSetup setup);`
- `std::vector<BacktestResults> run_parallel( const std::vector<std::map<std::string, double>>& param_sets, const std::function<std::unique_ptr<strategy::Strategy>( const std::map<std::string, double>&)>& strategy_factory, int num_threads = -1) const;`
- `void register_hook(plugins::HookType type, std::function<plugins::HookResult(plugins::HookContext&)> hook, int priority = 100);`
- `void on_progress(std::function<void(double pct, const std::string& msg)> callback);`
- `void set_audit_log_path(std::string path);`
- `BacktestResults results() const;`
- `[[nodiscard]] DashboardSnapshot dashboard_snapshot() const;`
- `[[nodiscard]] std::string dashboard_snapshot_json() const;`
- `[[nodiscard]] std::string dashboard_terminal( const DashboardRenderOptions& options =}) const;`
- `bool step();`
- `void run_until(Timestamp end_time);`
- `void run();`
- `void stop();`

### `regimeflow/engine/backtest_results.h`

Types:
- `struct TesterJournalEntry`
- `struct VenueFillAnalytics`
- `struct BacktestResults`
- `struct Aggregate`

Callables:
- `[[nodiscard]] std::vector<TesterJournalEntry> tester_journal() const;`
- `[[nodiscard]] DashboardSnapshot dashboard_snapshot() const;`
- `[[nodiscard]] std::string dashboard_snapshot_json() const;`
- `[[nodiscard]] std::string dashboard_terminal( const DashboardRenderOptions& options =}) const;`
- `[[nodiscard]] std::optional<engine::PortfolioSnapshot> latest_account_snapshot() const`
- `[[nodiscard]] std::vector<VenueFillAnalytics> venue_analytics() const`
- `by_venue.reserve(fills.size());`
- `const std::string venue = fill.venue.empty() ? "unassigned" : fill.venue;`
- `aggregate.stats.absolute_quantity += std::abs(fill.quantity);`
- `const double notional = std::abs(fill.price * fill.quantity);`
- `aggregate.orders.insert(fill.order_id);`
- `aggregate.parents.insert(fill.parent_order_id);`
- `(std::abs(fill.slippage) / std::abs(reference_price)) * 10000.0 * notional;`
- `results.reserve(by_venue.size());`
- `aggregate.stats.order_count = aggregate.orders.size();`
- `aggregate.stats.parent_order_count = aggregate.parents.size();`
- `static_cast<double>(aggregate.maker_fills) / static_cast<double>(aggregate.stats.fill_count);`
- `results.emplace_back(std::move(aggregate.stats));`
- `std::ranges::sort(results,}, &VenueFillAnalytics::venue);`

### `regimeflow/engine/backtest_runner.h`

Types:
- `struct BacktestRunSpec`
- `class BacktestRunner`

Callables:
- `BacktestRunner(BacktestEngine* engine, data::DataSource* data_source);`
- `BacktestResults run(std::unique_ptr<strategy::Strategy> strategy, const TimeRange& range, const std::vector<SymbolId>& symbols, data::BarType bar_type = data::BarType::Time_1Day) const;`
- `static std::vector<BacktestResults> run_parallel(const std::vector<BacktestRunSpec>& runs, int num_threads = -1);`

### `regimeflow/engine/dashboard_snapshot.h`

Types:
- `struct BacktestResults`
- `struct DashboardSetup`
- `struct DashboardVenueSummary`
- `struct DashboardOrderSummary`
- `struct DashboardQuoteSummary`
- `struct DashboardSnapshot`
- `enum class DashboardTab`
- `struct DashboardRenderOptions`

Callables:
- `[[nodiscard]] std::vector<DashboardVenueSummary> summarize_dashboard_venues( const std::vector<Fill>& fills);`
- `[[nodiscard]] DashboardSnapshot make_dashboard_snapshot(const BacktestResults& results);`
- `[[nodiscard]] std::string dashboard_snapshot_to_json(const DashboardSnapshot& snapshot);`
- `[[nodiscard]] std::string render_dashboard_terminal( const DashboardSnapshot& snapshot, const DashboardRenderOptions& options =});`

### `regimeflow/engine/engine_factory.h`

Types:
- `class EngineFactory`

Callables:
- `static std::unique_ptr<BacktestEngine> create(const Config& config);`

### `regimeflow/engine/event_generator.h`

Types:
- `class EventGenerator`
- `struct Config`

Callables:
- `Duration regime_check_interval = Duration::minutes(5);`
- `EventGenerator(std::unique_ptr<data::DataIterator> iterator, events::EventQueue* queue);`
- `EventGenerator(std::unique_ptr<data::DataIterator> iterator, events::EventQueue* queue, Config config);`
- `EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator, std::unique_ptr<data::TickIterator> tick_iterator, std::unique_ptr<data::OrderBookIterator> book_iterator, events::EventQueue* queue);`
- `EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator, std::unique_ptr<data::TickIterator> tick_iterator, std::unique_ptr<data::OrderBookIterator> book_iterator, events::EventQueue* queue, Config config);`
- `void enqueue_all() const;`

### `regimeflow/engine/event_loop.h`

Types:
- `class EventLoop`

Callables:
- `using Hook = std::function<void(const events::Event&)>;`
- `using ProgressCallback = std::function<void(size_t processed, size_t remaining)>;`
- `explicit EventLoop(events::EventQueue* queue);`
- `void set_dispatcher(events::EventDispatcher* dispatcher);`
- `void add_pre_hook(Hook hook);`
- `void add_post_hook(Hook hook);`
- `void set_progress_callback(ProgressCallback callback);`
- `void run();`
- `void run_until(Timestamp end_time);`
- `bool step();`
- `void stop();`
- `[[nodiscard]] Timestamp current_time() const return current_time_; }`

### `regimeflow/engine/execution_pipeline.h`

Types:
- `class ExecutionPipeline`
- `enum class FillPolicy`
- `enum class PriceDriftAction`
- `enum class QueueDepthMode`
- `enum class BarSimulationMode`
- `struct SessionPolicy`

Callables:
- `ExecutionPipeline(MarketDataCache* market_data, OrderBookCache* order_books, events::EventQueue* event_queue);`
- `void set_execution_model(std::unique_ptr<execution::ExecutionModel> model);`
- `void set_commission_model(std::unique_ptr<execution::CommissionModel> model);`
- `void set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model);`
- `void set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model);`
- `void set_latency_model(std::unique_ptr<execution::LatencyModel> model);`
- `void set_default_fill_policy(FillPolicy policy);`
- `void set_price_drift_rule(double max_deviation_bps, PriceDriftAction action);`
- `void set_queue_model(bool enabled, double progress_fraction, double default_visible_qty, QueueDepthMode mode = QueueDepthMode::TopOnly, double aging_fraction = 0.0, double replenishment_fraction = 0.0);`
- `void set_bar_simulation_mode(BarSimulationMode mode);`
- `void set_session_policy(SessionPolicy policy);`
- `void set_symbol_halt(SymbolId symbol, bool halted);`
- `void set_global_halt(bool halted);`
- `void on_order_submitted(const Order& order);`
- `void on_order_update(const Order& order);`
- `void on_market_update(SymbolId symbol, Timestamp timestamp);`
- `void on_bar(const data::Bar& bar);`
- `EvaluationContext() : executable_price_override(0.0), has_price_override(false), use_order_book(true)}`

### `regimeflow/engine/market_data_cache.h`

Types:
- `class MarketDataCache`

Callables:
- `void update(const data::Bar& bar);`
- `void update(const data::Tick& tick);`
- `void update(const data::Quote& quote);`
- `std::optional<data::Bar> latest_bar(SymbolId symbol) const;`
- `std::optional<data::Tick> latest_tick(SymbolId symbol) const;`
- `std::optional<data::Quote> latest_quote(SymbolId symbol) const;`
- `std::vector<data::Bar> recent_bars(SymbolId symbol, size_t count) const;`

### `regimeflow/engine/order.h`

Types:
- `enum class OrderSide`
- `enum class OrderType`
- `enum class TimeInForce`
- `enum class OrderStatus`
- `struct Order`
- `struct Fill`

Callables:
- `static Order market(SymbolId symbol, OrderSide side, Quantity qty);`
- `static Order limit(SymbolId symbol, OrderSide side, Quantity qty, Price price);`
- `static Order stop(SymbolId symbol, OrderSide side, Quantity qty, Price stop);`

### `regimeflow/engine/order_book_cache.h`

Types:
- `class OrderBookCache`

Callables:
- `void update(const data::OrderBook& book);`
- `std::optional<data::OrderBook> latest(SymbolId symbol) const;`

### `regimeflow/engine/order_manager.h`

Types:
- `struct OrderModification`
- `class OrderManager`

Callables:
- `using RoutingContextProvider = std::function<RoutingContext(const Order&)>;`
- `Result<OrderId> submit_order(Order order);`
- `Result<void> cancel_order(OrderId id);`
- `Result<void> modify_order(OrderId id, const OrderModification& mod);`
- `std::optional<Order> get_order(OrderId id) const;`
- `std::vector<Order> get_open_orders() const;`
- `std::vector<Order> get_open_orders(SymbolId symbol) const;`
- `std::vector<Order> get_orders_by_strategy(const std::string& strategy_id) const;`
- `std::vector<Fill> get_fills(OrderId id) const;`
- `std::vector<Fill> get_fills(SymbolId symbol, TimeRange range) const;`
- `void on_order_update(std::function<void(const Order&)> callback);`
- `void on_fill(std::function<void(const Fill&)> callback);`
- `void on_pre_submit(std::function<Result<void>(Order&)> callback);`
- `void set_router(std::unique_ptr<OrderRouter> router, RoutingContextProvider provider);`
- `void clear_router();`
- `Result<void> activate_routed_order(OrderId id);`
- `[[nodiscard]] bool is_routing_parent(OrderId id) const;`
- `[[nodiscard]] bool is_routing_child(OrderId id) const;`
- `std::vector<OrderId> expired_order_ids(Timestamp now) const;`
- `void process_fill(Fill fill);`
- `Result<void> update_order_status(OrderId id, OrderStatus status);`

### `regimeflow/engine/order_routing.h`

Types:
- `enum class RoutingMode`
- `enum class SplitMode`
- `enum class ParentAggregation`
- `struct RoutingVenue`
- `struct RoutingConfig`
- `struct RoutingContext`
- `struct RoutingPlan`
- `class OrderRouter`
- `class SmartOrderRouter`

Callables:
- `[[nodiscard]] bool split_enabled() const`
- `static RoutingConfig from_config(const Config& config, const std::string& prefix = "routing");`
- `[[nodiscard]] std::optional<Price> mid() const;`
- `[[nodiscard]] std::optional<double> spread_bps() const;`
- `virtual ~OrderRouter() = default;`
- `[[nodiscard]] virtual RoutingPlan route(const Order& order, const RoutingContext& ctx) const = 0;`
- `explicit SmartOrderRouter(RoutingConfig config);`
- `[[nodiscard]] RoutingPlan route(const Order& order, const RoutingContext& ctx) const override;`

### `regimeflow/engine/parity_checker.h`

Types:
- `class ParityChecker`

Callables:
- `static ParityReport check(const Config& backtest, const Config& live);`
- `static ParityReport check_files(const std::string& backtest_path, const std::string& live_path);`

### `regimeflow/engine/parity_report.h`

Types:
- `enum class ParityStatus`
- `struct ParityReport`

Callables:
- `[[nodiscard]] bool ok() const return status == ParityStatus::Pass; }`
- `[[nodiscard]] std::string status_name() const`

### `regimeflow/engine/portfolio.h`

Types:
- `struct MarginProfile`
- `struct MarginSnapshot`
- `struct Position`
- `struct PortfolioSnapshot`
- `class Portfolio`

Callables:
- `[[nodiscard]] static MarginProfile from_config(const Config& config, const std::string& prefix = "account.margin");`
- `[[nodiscard]] static MarginProfile from_config(const Config& config, const std::string& prefix, const MarginProfile& defaults);`
- `[[nodiscard]] double market_value() const return quantity * current_price; }`
- `[[nodiscard]] double unrealized_pnl() const return quantity * (current_price - avg_cost); }`
- `[[nodiscard]] double unrealized_pnl_pct() const`
- `explicit Portfolio(double initial_capital, std::string currency = "USD");`
- `void update_position(const Fill& fill);`
- `void mark_to_market(SymbolId symbol, Price price, Timestamp timestamp);`
- `void mark_to_market(const std::unordered_map<SymbolId, Price>& prices, Timestamp timestamp);`
- `void set_cash(double cash, Timestamp timestamp);`
- `void adjust_cash(double delta, Timestamp timestamp);`
- `void set_position(SymbolId symbol, Quantity quantity, Price avg_cost, Price current_price, Timestamp timestamp);`
- `void replace_positions(const std::unordered_map<SymbolId, Position>& positions, Timestamp timestamp);`
- `std::optional<Position> get_position(SymbolId symbol) const;`
- `std::vector<Position> get_all_positions() const;`
- `std::vector<SymbolId> get_held_symbols() const;`
- `double cash() const return cash_; }`
- `double initial_capital() const return initial_capital_; }`
- `const std::string& currency() const return currency_; }`
- `const MarginProfile& margin_profile() const return margin_profile_; }`
- `double equity() const;`
- `double gross_exposure() const;`
- `double net_exposure() const;`
- `double leverage() const;`
- `double total_unrealized_pnl() const;`
- `double total_realized_pnl() const return realized_pnl_; }`
- `void configure_margin(MarginProfile profile);`
- `[[nodiscard]] MarginSnapshot margin_snapshot() const;`
- `PortfolioSnapshot snapshot() const;`
- `PortfolioSnapshot snapshot(Timestamp timestamp) const;`
- `std::vector<PortfolioSnapshot> equity_curve() const return snapshots_; }`
- `void record_snapshot(Timestamp timestamp);`
- `std::vector<Fill> get_fills() const return all_fills_; }`
- `std::vector<Fill> get_fills(SymbolId symbol) const;`
- `std::vector<Fill> get_fills(TimeRange range) const;`
- `void on_position_change(std::function<void(const Position&)> callback);`
- `void on_equity_change(std::function<void(double)> callback);`

### `regimeflow/engine/regime_tracker.h`

Types:
- `class RegimeTracker`

Callables:
- `explicit RegimeTracker(std::unique_ptr<regime::RegimeDetector> detector);`
- `RegimeTracker(RegimeTracker&&) noexcept = default;`
- `RegimeTracker& operator=(RegimeTracker&&) noexcept = default;`
- `RegimeTracker(const RegimeTracker&) = delete;`
- `RegimeTracker& operator=(const RegimeTracker&) = delete;`
- `std::optional<regime::RegimeTransition> on_bar(const data::Bar& bar);`
- `std::optional<regime::RegimeTransition> on_tick(const data::Tick& tick);`
- `[[nodiscard]] const regime::RegimeState& current_state() const return current_state_; }`
- `[[nodiscard]] const std::deque<regime::RegimeState>& history() const return history_; }`
- `void set_history_size(size_t size) history_size_ = size; }`
- `void register_transition_callback( std::function<void(const regime::RegimeTransition&)> callback);`

### `regimeflow/engine/timer_service.h`

Types:
- `class TimerService`

Callables:
- `explicit TimerService(events::EventQueue* queue);`
- `void schedule(const std::string& id, Duration interval, Timestamp start);`
- `void cancel(const std::string& id);`
- `void on_time_advance(Timestamp now);`
- `Duration interval{Duration::microseconds(0)};`

## `events`

### `regimeflow/events/dispatcher.h`

Types:
- `class EventDispatcher`

Callables:
- `using Handler = std::function<void(const Event&)>;`
- `void set_market_handler(Handler handler) market_handler_ = std::move(handler); }`
- `void set_order_handler(Handler handler) order_handler_ = std::move(handler); }`
- `void set_system_handler(Handler handler) system_handler_ = std::move(handler); }`
- `void set_user_handler(Handler handler) user_handler_ = std::move(handler); }`
- `void dispatch(const Event& event) const`

### `regimeflow/events/event.h`

Types:
- `enum class EventType`
- `enum class MarketEventKind`
- `enum class OrderEventKind`
- `enum class SystemEventKind`
- `struct MarketEventPayload`
- `struct OrderEventPayload`
- `struct SystemEventPayload`
- `struct Event`

Callables:
- `inline uint8_t default_priority(EventType type)`
- `inline Event make_market_event(const data::Bar& bar)`
- `inline Event make_market_event(const data::Tick& tick)`
- `inline Event make_market_event(const data::Quote& quote)`
- `inline Event make_market_event(const data::OrderBook& book)`
- `inline Event make_system_event(SystemEventKind kind, Timestamp timestamp, int64_t code = 0, std::string id =})`
- `event.payload = SystemEventPayload{kind, code, std::move(id)};`
- `inline Event make_order_event(OrderEventKind kind, Timestamp timestamp, OrderId order_id, FillId fill_id = 0, Quantity quantity = 0, Price price = 0, SymbolId symbol = 0, double commission = 0.0, bool is_maker = false, double transaction_cost = 0.0, OrderId parent_order_id = 0, std::string venue =})`
- `std::move(venue)};`

### `regimeflow/events/event_queue.h`

Types:
- `struct EventComparator`
- `class EventQueue`

Callables:
- `bool operator()(const Event& a, const Event& b) const`
- `void push(Event event)`
- `event.sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);`
- `Node* node = pool_.allocate();`
- `new (node) Node{std::move(event), nullptr};`
- `Node* prev = pending_.exchange(node, std::memory_order_acq_rel);`
- `std::optional<Event> pop()`
- `drain_pending();`
- `Event event = queue_.top();`
- `queue_.pop();`
- `std::optional<Event> peek()`
- `bool empty()`
- `size_t size()`
- `void clear()`
- `queue_ = std::priority_queue<Event, std::vector<Event>, EventComparator>();`
- `~EventQueue() clear(); }`

### `regimeflow/events/market_event.h`

Types:
- None detected

Callables:
- None detected

### `regimeflow/events/order_event.h`

Types:
- None detected

Callables:
- None detected

### `regimeflow/events/system_event.h`

Types:
- None detected

Callables:
- None detected

## `execution`

### `regimeflow/execution/basic_execution_model.h`

Types:
- `class BasicExecutionModel`

Callables:
- `explicit BasicExecutionModel(std::shared_ptr<SlippageModel> slippage_model);`
- `std::vector<engine::Fill> execute(const engine::Order& order, Price reference_price, Timestamp timestamp) override;`

### `regimeflow/execution/commission.h`

Types:
- `class CommissionModel`
- `class ZeroCommissionModel`
- `class FixedPerFillCommissionModel`

Callables:
- `virtual ~CommissionModel() = default;`
- `[[nodiscard]] virtual double commission(const engine::Order& order, const engine::Fill& fill) const = 0;`
- `[[nodiscard]] double commission(const engine::Order&, const engine::Fill&) const override return 0.0; }`
- `explicit FixedPerFillCommissionModel(const double amount) : amount_(amount)}`
- `[[nodiscard]] double commission(const engine::Order&, const engine::Fill&) const override return amount_; }`

### `regimeflow/execution/execution_factory.h`

Types:
- `class ExecutionFactory`

Callables:
- `static std::unique_ptr<ExecutionModel> create_execution_model(const Config& config);`
- `static std::unique_ptr<SlippageModel> create_slippage_model(const Config& config);`
- `static std::unique_ptr<CommissionModel> create_commission_model(const Config& config);`
- `static std::unique_ptr<TransactionCostModel> create_transaction_cost_model(const Config& config);`
- `static std::unique_ptr<MarketImpactModel> create_market_impact_model(const Config& config);`
- `static std::unique_ptr<LatencyModel> create_latency_model(const Config& config);`

### `regimeflow/execution/execution_model.h`

Types:
- `class ExecutionModel`

Callables:
- `virtual ~ExecutionModel() = default;`
- `virtual std::vector<engine::Fill> execute(const engine::Order& order, Price reference_price, Timestamp timestamp) = 0;`

### `regimeflow/execution/fill_simulator.h`

Types:
- `class FillSimulator`

Callables:
- `explicit FillSimulator(std::shared_ptr<SlippageModel> slippage_model);`
- `[[nodiscard]] engine::Fill simulate(const engine::Order& order, Price reference_price, Timestamp timestamp, bool is_maker = false) const;`

### `regimeflow/execution/latency_model.h`

Types:
- `class LatencyModel`
- `class FixedLatencyModel`

Callables:
- `virtual ~LatencyModel() = default;`
- `[[nodiscard]] virtual Duration latency() const = 0;`
- `explicit FixedLatencyModel(Duration latency) : latency_(latency)}`
- `[[nodiscard]] Duration latency() const override return latency_; }`

### `regimeflow/execution/market_impact.h`

Types:
- `class MarketImpactModel`
- `class ZeroMarketImpactModel`
- `class FixedMarketImpactModel`
- `class OrderBookImpactModel`

Callables:
- `virtual ~MarketImpactModel() = default;`
- `virtual double impact_bps(const engine::Order& order, const data::OrderBook* book) const = 0;`
- `double impact_bps(const engine::Order&, const data::OrderBook*) const override return 0.0; }`
- `explicit FixedMarketImpactModel(double bps) : bps_(bps)}`
- `double impact_bps(const engine::Order&, const data::OrderBook*) const override return bps_; }`
- `explicit OrderBookImpactModel(double max_bps = 50.0) : max_bps_(max_bps)}`
- `double impact_bps(const engine::Order& order, const data::OrderBook* book) const override;`

### `regimeflow/execution/order_book_execution_model.h`

Types:
- `class OrderBookExecutionModel`

Callables:
- `explicit OrderBookExecutionModel(std::shared_ptr<data::OrderBook> book);`
- `std::vector<engine::Fill> execute(const engine::Order& order, Price reference_price, Timestamp timestamp) override;`

### `regimeflow/execution/slippage.h`

Types:
- `class SlippageModel`
- `class ZeroSlippageModel`
- `class FixedBpsSlippageModel`
- `class RegimeBpsSlippageModel`

Callables:
- `virtual ~SlippageModel() = default;`
- `virtual Price execution_price(const engine::Order& order, Price reference_price) const = 0;`
- `Price execution_price(const engine::Order& order, Price reference_price) const override;`
- `explicit FixedBpsSlippageModel(double bps);`
- `RegimeBpsSlippageModel(double default_bps, std::unordered_map<regime::RegimeType, double> bps_map);`

### `regimeflow/execution/transaction_cost.h`

Types:
- `class TransactionCostModel`
- `class ZeroTransactionCostModel`
- `class FixedBpsTransactionCostModel`
- `class PerShareTransactionCostModel`
- `class PerOrderTransactionCostModel`
- `struct TieredTransactionCostTier`
- `class TieredBpsTransactionCostModel`
- `class MakerTakerTransactionCostModel`

Callables:
- `virtual ~TransactionCostModel() = default;`
- `[[nodiscard]] virtual double cost(const engine::Order& order, const engine::Fill& fill) const = 0;`
- `[[nodiscard]] double cost(const engine::Order&, const engine::Fill&) const override return 0.0; }`
- `explicit FixedBpsTransactionCostModel(double bps) : bps_(bps)}`
- `[[nodiscard]] double cost(const engine::Order&, const engine::Fill& fill) const override`
- `explicit PerShareTransactionCostModel(double rate_per_share) : rate_per_share_(rate_per_share)}`
- `explicit PerOrderTransactionCostModel(double cost_per_order) : cost_per_order_(cost_per_order)}`
- `double cost(const engine::Order& order, const engine::Fill&) const override`
- `std::lock_guard<std::mutex> lock(mutex_);`
- `explicit TieredBpsTransactionCostModel(std::vector<TieredTransactionCostTier> tiers) : tiers_(std::move(tiers))}`
- `const double notional = std::abs(fill.price * fill.quantity);`
- `double bps = tiers_.back().bps;`
- `MakerTakerTransactionCostModel(double maker_rebate_bps, double taker_fee_bps) : maker_rebate_bps_(maker_rebate_bps), taker_fee_bps_(taker_fee_bps)}`
- `[[nodiscard]] double cost(const engine::Order& order, const engine::Fill& fill) const override`
- `const auto parse_override = [](const engine::Order& order, const char* key, double fallback)`
- `const double value = std::strtod(it->second.c_str(), &end);`
- `maker_rebate_bps = parse_override(order, "venue_maker_rebate_bps", maker_rebate_bps);`
- `taker_fee_bps = parse_override(order, "venue_taker_fee_bps", taker_fee_bps);`

## `live`

### `regimeflow/live/alpaca_adapter.h`

Types:
- `class AlpacaAdapter`
- `struct Config`

Callables:
- `explicit AlpacaAdapter(Config config);`
- `Result<void> connect() override;`
- `Result<void> disconnect() override;`
- `[[nodiscard]] bool is_connected() const override;`
- `void subscribe_market_data(const std::vector<std::string>& symbols) override;`
- `void unsubscribe_market_data(const std::vector<std::string>& symbols) override;`
- `Result<std::string> submit_order(const engine::Order& order) override;`
- `Result<void> cancel_order(const std::string& broker_order_id) override;`
- `Result<void> modify_order(const std::string& broker_order_id, const engine::OrderModification& mod) override;`
- `AccountInfo get_account_info() override;`
- `std::vector<Position> get_positions() override;`
- `std::vector<ExecutionReport> get_open_orders() override;`
- `void on_market_data(std::function<void(const MarketDataUpdate&)> cb) override;`
- `void on_execution_report(std::function<void(const ExecutionReport&)> cb) override;`
- `void on_position_update(std::function<void(const Position&)> cb) override;`
- `[[nodiscard]] int max_orders_per_second() const override;`
- `[[nodiscard]] int max_messages_per_second() const override;`
- `[[nodiscard]] bool supports_tif(engine::OrderType type, engine::TimeInForce tif) const override;`
- `void poll() override;`

### `regimeflow/live/audit_log.h`

Types:
- `struct AuditEvent`
- `enum class Type`
- `class AuditLogger`

Callables:
- `explicit AuditLogger(std::string path);`
- `Result<void> log(const AuditEvent& event);`
- `Result<void> log_error(const std::string& error);`
- `Result<void> log_regime_change(const regime::RegimeTransition& transition);`

### `regimeflow/live/binance_adapter.h`

Types:
- `class BinanceAdapter`
- `struct Config`

Callables:
- `R"({"method":"SUBSCRIBE","params":{symbols},"id":1})";`
- `R"({"method":"UNSUBSCRIBE","params":{symbols},"id":2})";`
- `explicit BinanceAdapter(Config config);`
- `Result<void> connect() override;`
- `Result<void> disconnect() override;`
- `[[nodiscard]] bool is_connected() const override;`
- `void subscribe_market_data(const std::vector<std::string>& symbols) override;`
- `void unsubscribe_market_data(const std::vector<std::string>& symbols) override;`
- `Result<std::string> submit_order(const engine::Order& order) override;`
- `Result<void> cancel_order(const std::string& broker_order_id) override;`
- `Result<void> modify_order(const std::string& broker_order_id, const engine::OrderModification& mod) override;`
- `AccountInfo get_account_info() override;`
- `std::vector<Position> get_positions() override;`
- `std::vector<ExecutionReport> get_open_orders() override;`
- `void on_market_data(std::function<void(const MarketDataUpdate&)> cb) override;`
- `void on_execution_report(std::function<void(const ExecutionReport&)> cb) override;`
- `void on_position_update(std::function<void(const Position&)> cb) override;`
- `[[nodiscard]] int max_orders_per_second() const override;`
- `[[nodiscard]] int max_messages_per_second() const override;`
- `[[nodiscard]] bool supports_tif(engine::OrderType type, engine::TimeInForce tif) const override;`
- `void poll() override;`

### `regimeflow/live/broker_adapter.h`

Types:
- `enum class LiveOrderStatus`
- `struct ExecutionReport`
- `class BrokerAdapter`

Callables:
- `virtual ~BrokerAdapter() = default;`
- `virtual Result<void> connect() = 0;`
- `virtual Result<void> disconnect() = 0;`
- `[[nodiscard]] virtual bool is_connected() const = 0;`
- `virtual void subscribe_market_data(const std::vector<std::string>& symbols) = 0;`
- `virtual void unsubscribe_market_data(const std::vector<std::string>& symbols) = 0;`
- `virtual Result<std::string> submit_order(const engine::Order& order) = 0;`
- `virtual Result<void> cancel_order(const std::string& broker_order_id) = 0;`
- `virtual Result<void> modify_order(const std::string& broker_order_id, const engine::OrderModification& mod) = 0;`
- `virtual AccountInfo get_account_info() = 0;`
- `virtual std::vector<Position> get_positions() = 0;`
- `virtual std::vector<ExecutionReport> get_open_orders() = 0;`
- `virtual void on_market_data(std::function<void(const MarketDataUpdate&)>) = 0;`
- `virtual void on_execution_report(std::function<void(const ExecutionReport&)>) = 0;`
- `virtual void on_position_update(std::function<void(const Position&)>) = 0;`
- `[[nodiscard]] virtual int max_orders_per_second() const = 0;`
- `[[nodiscard]] virtual int max_messages_per_second() const = 0;`
- `[[nodiscard]] virtual bool supports_tif(engine::OrderType type, engine::TimeInForce tif) const = 0;`
- `virtual void poll() = 0;`

### `regimeflow/live/event_bus.h`

Types:
- `enum class LiveTopic`
- `struct LiveMessage`
- `class EventBus`

Callables:
- `using Callback = std::function<void(const LiveMessage&)>;`
- `EventBus();`
- `~EventBus();`
- `void start();`
- `void stop();`
- `SubscriptionId subscribe(LiveTopic topic, Callback callback);`
- `void unsubscribe(SubscriptionId id);`
- `void publish(LiveMessage message);`

### `regimeflow/live/ib_adapter.h`

Types:
- `class IBAdapter`
- `struct ContractConfig`
- `struct Config`

Callables:
- `explicit IBAdapter(Config config);`
- `Result<void> connect() override;`
- `Result<void> disconnect() override;`
- `bool is_connected() const override;`
- `void subscribe_market_data(const std::vector<std::string>& symbols) override;`
- `void unsubscribe_market_data(const std::vector<std::string>& symbols) override;`
- `Result<std::string> submit_order(const engine::Order& order) override;`
- `Result<void> cancel_order(const std::string& broker_order_id) override;`
- `Result<void> modify_order(const std::string& broker_order_id, const engine::OrderModification& mod) override;`
- `AccountInfo get_account_info() override;`
- `std::vector<Position> get_positions() override;`
- `std::vector<ExecutionReport> get_open_orders() override;`
- `void on_market_data(std::function<void(const MarketDataUpdate&)> cb) override;`
- `void on_execution_report(std::function<void(const ExecutionReport&)> cb) override;`
- `void on_position_update(std::function<void(const Position&)> cb) override;`
- `int max_orders_per_second() const override;`
- `int max_messages_per_second() const override;`
- `bool supports_tif(engine::OrderType type, engine::TimeInForce tif) const override;`
- `[[nodiscard]] ContractConfig contract_config_for_symbol(std::string_view symbol) const;`
- `void poll() override;`

### `regimeflow/live/live_engine.h`

Types:
- `struct LiveConfig`
- `class LiveTradingEngine`
- `struct EngineStatus`
- `struct SystemHealth`

Callables:
- `Duration order_reconcile_interval = Duration::seconds(30);`
- `Duration position_reconcile_interval = Duration::seconds(60);`
- `Duration account_refresh_interval = Duration::seconds(30);`
- `Duration heartbeat_timeout = Duration::seconds(30);`
- `Duration reconnect_initial = Duration::seconds(1);`
- `Duration reconnect_max = Duration::seconds(30);`
- `Duration regime_retrain_interval = Duration::hours(24);`
- `explicit LiveTradingEngine(const LiveConfig& config);`
- `LiveTradingEngine(const LiveConfig& config, std::unique_ptr<BrokerAdapter> broker);`
- `~LiveTradingEngine();`
- `Result<void> start();`
- `void stop();`
- `bool is_running() const;`
- `EngineStatus get_status() const;`
- `DashboardSnapshot get_dashboard_snapshot() const;`
- `std::string dashboard_snapshot_json() const;`
- `std::string dashboard_terminal( const engine::DashboardRenderOptions& options =}) const;`
- `SystemHealth get_system_health() const;`
- `void enable_trading();`
- `void disable_trading();`
- `void close_all_positions() const;`
- `void on_trade(std::function<void(const Trade&)> cb);`
- `void on_regime_change(std::function<void(const regime::RegimeTransition&)> cb);`
- `void on_error(std::function<void(const std::string&)> cb);`
- `void on_dashboard_update(std::function<void(const DashboardSnapshot&)> cb);`

### `regimeflow/live/live_order_manager.h`

Types:
- `struct LiveOrder`
- `class LiveOrderManager`

Callables:
- `explicit LiveOrderManager(BrokerAdapter* broker);`
- `Result<engine::OrderId> submit_order(const engine::Order& order);`
- `Result<void> cancel_order(engine::OrderId id);`
- `Result<void> cancel_all_orders();`
- `Result<void> cancel_orders(const std::string& symbol);`
- `Result<void> modify_order(engine::OrderId id, const engine::OrderModification& mod);`
- `std::optional<LiveOrder> get_order(engine::OrderId id) const;`
- `std::vector<LiveOrder> get_open_orders() const;`
- `std::vector<LiveOrder> get_orders_by_status(LiveOrderStatus status) const;`
- `void on_execution_report(std::function<void(const ExecutionReport&)> cb);`
- `void on_order_update(std::function<void(const LiveOrder&)> cb);`
- `void handle_execution_report(const ExecutionReport& report);`
- `Result<void> reconcile_with_broker();`
- `bool validate_order(const engine::Order& order) const;`
- `std::optional<engine::OrderId> find_order_id_by_broker_id( const std::string& broker_order_id) const;`

### `regimeflow/live/mq_adapter.h`

Types:
- `struct MessageQueueConfig`
- `class MessageQueueAdapter`

Callables:
- `virtual ~MessageQueueAdapter() = default;`
- `virtual Result<void> connect() = 0;`
- `virtual Result<void> disconnect() = 0;`
- `[[nodiscard]] virtual bool is_connected() const = 0;`
- `virtual Result<void> publish(const LiveMessage& message) = 0;`
- `virtual void on_message(std::function<void(const LiveMessage&)> cb) = 0;`
- `virtual void poll() = 0;`
- `std::unique_ptr<MessageQueueAdapter> create_message_queue_adapter(const MessageQueueConfig& config);`

### `regimeflow/live/mq_codec.h`

Types:
- `class LiveMessageCodec`

Callables:
- `static std::string encode(const LiveMessage& message);`
- `static std::optional<LiveMessage> decode(const std::string& payload);`

### `regimeflow/live/secret_hygiene.h`

Types:
- None detected

Callables:
- `[[nodiscard]] std::optional<std::string> read_secret_env(const char* key);`
- `[[nodiscard]] bool is_secret_reference(std::string_view value);`
- `[[nodiscard]] Result<std::string> resolve_secret_reference(std::string_view value);`
- `[[nodiscard]] Result<void> resolve_secret_config(std::map<std::string, std::string>& values);`
- `[[nodiscard]] bool is_sensitive_secret_key(std::string_view key);`
- `void register_sensitive_value(std::string_view value);`
- `void register_sensitive_config(const std::map<std::string, std::string>& values);`
- `[[nodiscard]] std::string redact_sensitive_values(std::string_view message);`
- `void reset_sensitive_values_for_tests();`

### `regimeflow/live/types.h`

Types:
- `struct Position`
- `struct AccountInfo`
- `struct MarketDataUpdate`
- `struct Trade`

Callables:
- `[[nodiscard]] Timestamp timestamp() const;`
- `[[nodiscard]] SymbolId symbol() const;`

## `metrics`

### `regimeflow/metrics/attribution.h`

Types:
- `struct AttributionSnapshot`
- `class AttributionTracker`

Callables:
- `void update(Timestamp timestamp, const engine::Portfolio& portfolio);`
- `const AttributionSnapshot& last() const return last_; }`

### `regimeflow/metrics/drawdown.h`

Types:
- `struct DrawdownSnapshot`
- `class DrawdownTracker`

Callables:
- `void update(Timestamp timestamp, double equity);`
- `[[nodiscard]] double max_drawdown() const return max_drawdown_; }`
- `[[nodiscard]] Timestamp max_drawdown_start() const return max_start_; }`
- `[[nodiscard]] Timestamp max_drawdown_end() const return max_end_; }`
- `[[nodiscard]] DrawdownSnapshot last_snapshot() const return last_; }`

### `regimeflow/metrics/live_performance.h`

Types:
- `struct LivePerformanceConfig`
- `struct LivePerformanceSnapshot`
- `struct LivePerformanceSummary`
- `class LivePerformanceTracker`

Callables:
- `explicit LivePerformanceTracker(LivePerformanceConfig config);`
- `void start(double initial_equity);`
- `void update(Timestamp timestamp, double equity, double daily_pnl);`
- `void flush();`
- `[[nodiscard]] const LivePerformanceSummary& summary() const return summary_; }`
- `[[nodiscard]] const LivePerformanceConfig& config() const return config_; }`

### `regimeflow/metrics/metrics_tracker.h`

Types:
- `class MetricsTracker`

Callables:
- `void update(Timestamp timestamp, double equity);`
- `void update(Timestamp timestamp, const engine::Portfolio& portfolio, std::optional<regime::RegimeType> regime = std::nullopt);`
- `void update(Timestamp timestamp, const engine::Portfolio& portfolio, const regime::RegimeState& regime);`
- `const EquityCurve& equity_curve() const return equity_curve_; }`
- `const std::vector<engine::PortfolioSnapshot>& portfolio_snapshots() const`
- `const DrawdownTracker& drawdown() const return drawdown_; }`
- `const AttributionTracker& attribution() const return attribution_; }`
- `const RegimeAttribution& regime_attribution() const return regime_attribution_; }`
- `const TransitionMetrics& transition_metrics() const return transition_metrics_; }`
- `const std::vector<regime::RegimeState>& regime_history() const`

### `regimeflow/metrics/performance.h`

Types:
- `class EquityCurve`

Callables:
- `void add_point(Timestamp timestamp, double equity);`
- `[[nodiscard]] const std::vector<Timestamp>& timestamps() const return timestamps_; }`
- `[[nodiscard]] const std::vector<double>& equities() const return equities_; }`
- `[[nodiscard]] double total_return() const;`

### `regimeflow/metrics/performance_calculator.h`

Types:
- `struct PerformanceSummary`
- `struct RegimeMetrics`
- `struct TransitionMetricsSummary`
- `struct AttributionResult`
- `class PerformanceCalculator`

Callables:
- `Duration max_drawdown_duration = Duration::seconds(0);`
- `Duration avg_duration = Duration::seconds(0);`
- `PerformanceSummary calculate(const std::vector<engine::PortfolioSnapshot>& equity_curve, const std::vector<engine::Fill>& fills, double risk_free_rate = 0.0, const std::vector<double>* benchmark_returns = nullptr) const;`
- `std::map<regime::RegimeType, RegimeMetrics> calculate_by_regime( const std::vector<engine::PortfolioSnapshot>& equity_curve, const std::vector<engine::Fill>& fills, const std::vector<regime::RegimeState>& regimes, double risk_free_rate = 0.0) const;`
- `std::vector<TransitionMetricsSummary> calculate_transitions( const std::vector<engine::PortfolioSnapshot>& equity_curve, const std::vector<regime::RegimeTransition>& transitions) const;`

### `regimeflow/metrics/performance_metric.h`

Types:
- `class PerformanceMetric`

Callables:
- `virtual ~PerformanceMetric() = default;`
- `[[nodiscard]] virtual std::string name() const = 0;`
- `[[nodiscard]] virtual double compute(const EquityCurve& curve, double periods_per_year) const = 0;`

### `regimeflow/metrics/performance_metrics.h`

Types:
- `struct PerformanceStats`

Callables:
- `PerformanceStats compute_stats(const EquityCurve& curve, double periods_per_year);`

### `regimeflow/metrics/regime_attribution.h`

Types:
- `struct RegimePerformance`
- `class RegimeAttribution`

Callables:
- `void update(Timestamp timestamp, regime::RegimeType regime, double equity_return);`
- `[[nodiscard]] const std::map<regime::RegimeType, RegimePerformance>& results() const return results_; }`

### `regimeflow/metrics/report.h`

Types:
- `struct Report`

Callables:
- `Report build_report(const MetricsTracker& tracker, double periods_per_year);`
- `Report build_report(const MetricsTracker& tracker, const std::vector<engine::Fill>& fills, double risk_free_rate = 0.0, const std::vector<double>* benchmark_returns = nullptr);`

### `regimeflow/metrics/report_writer.h`

Types:
- `class ReportWriter`

Callables:
- `static std::string to_csv(const Report& report);`
- `static std::string to_json(const Report& report);`

### `regimeflow/metrics/transition_metrics.h`

Types:
- `struct TransitionStats`
- `class TransitionMetrics`

Callables:
- `void update(regime::RegimeType from, regime::RegimeType to, double equity_return);`
- `[[nodiscard]] const std::map<std::pair<regime::RegimeType, regime::RegimeType>, TransitionStats>& results() const`

## `plugins`

### `regimeflow/plugins/hooks.h`

Types:
- `struct BacktestResults`
- `enum class HookResult`
- `enum class HookType`
- `class HookContext`
- `class HookManager`
- `class HookSystem`

Callables:
- `HookContext(const engine::Portfolio* portfolio, const engine::MarketDataCache* market, const regime::RegimeState* regime, events::EventQueue* queue, const Timestamp current_time) : portfolio_(portfolio), market_(market), regime_(regime), queue_(queue), current_time_(current_time)}`
- `[[nodiscard]] const engine::Portfolio& portfolio() const return *portfolio_; }`
- `[[nodiscard]] const engine::MarketDataCache& market() const return *market_; }`
- `[[nodiscard]] const regime::RegimeState& regime() const return *regime_; }`
- `[[nodiscard]] Timestamp current_time() const return current_time_; }`
- `[[nodiscard]] const data::Bar* bar() const return bar_; }`
- `[[nodiscard]] const data::Tick* tick() const return tick_; }`
- `[[nodiscard]] const data::Quote* quote() const return quote_; }`
- `[[nodiscard]] const data::OrderBook* book() const return book_; }`
- `[[nodiscard]] const engine::Fill* fill() const return fill_; }`
- `[[nodiscard]] const regime::RegimeTransition* regime_change() const return regime_change_; }`
- `[[nodiscard]] engine::Order* order() const return order_; }`
- `[[nodiscard]] const engine::BacktestResults* results() const return results_; }`
- `[[nodiscard]] const std::string& timer_id() const return timer_id_; }`
- `void set_bar(const data::Bar* bar) bar_ = bar; }`
- `void set_tick(const data::Tick* tick) tick_ = tick; }`
- `void set_quote(const data::Quote* quote) quote_ = quote; }`
- `void set_book(const data::OrderBook* book) book_ = book; }`
- `void set_fill(const engine::Fill* fill) fill_ = fill; }`
- `void set_regime_change(const regime::RegimeTransition* change) regime_change_ = change; }`
- `void set_order(engine::Order* order) order_ = order; }`
- `void set_results(const engine::BacktestResults* results) results_ = results; }`
- `void set_timer_id(std::string id) timer_id_ = std::move(id); }`
- `void modify_order(const engine::Order& order) const`
- `void inject_event(events::Event event) const`
- `queue_->push(std::move(event));`
- `using Hook = std::function<HookResult(HookContext&)>;`
- `using BacktestStartHook = std::function<HookResult(HookContext&)>;`
- `using BacktestEndHook = std::function<HookResult(HookContext&, const engine::BacktestResults&)>;`
- `using DayStartHook = std::function<HookResult(HookContext&, Timestamp)>;`
- `using DayEndHook = std::function<HookResult(HookContext&, Timestamp)>;`
- `using BarHook = std::function<HookResult(HookContext&, const data::Bar&)>;`
- `using TickHook = std::function<HookResult(HookContext&, const data::Tick&)>;`
- `using QuoteHook = std::function<HookResult(HookContext&, const data::Quote&)>;`
- `using BookHook = std::function<HookResult(HookContext&, const data::OrderBook&)>;`
- `using TimerHook = std::function<HookResult(HookContext&, const std::string&)>;`
- `using OrderSubmitHook = std::function<HookResult(HookContext&, engine::Order&)>;`
- `using FillHook = std::function<HookResult(HookContext&, const engine::Fill&)>;`
- `using RegimeChangeHook = std::function<HookResult(HookContext&, const regime::RegimeTransition&)>;`
- `void register_hook(HookType type, Hook hook, int priority = 100);`
- `void register_backtest_start(BacktestStartHook hook, int priority = 100);`
- `void register_backtest_end(BacktestEndHook hook, int priority = 100);`
- `void register_day_start(DayStartHook hook, int priority = 100);`
- `void register_day_end(DayEndHook hook, int priority = 100);`
- `void register_on_bar(BarHook hook, int priority = 100);`
- `void register_on_tick(TickHook hook, int priority = 100);`
- `void register_on_quote(QuoteHook hook, int priority = 100);`
- `void register_on_book(BookHook hook, int priority = 100);`
- `void register_on_timer(TimerHook hook, int priority = 100);`
- `void register_order_submit(OrderSubmitHook hook, int priority = 100);`
- `void register_on_fill(FillHook hook, int priority = 100);`
- `void register_regime_change(RegimeChangeHook hook, int priority = 100);`
- `HookResult invoke(HookType type, HookContext& ctx) const;`
- `void clear_all_hooks();`
- `void disable_hooks();`
- `void enable_hooks();`
- `using EventHook = std::function<void(const events::Event&)>;`
- `using SimpleHook = std::function<void()>;`
- `void add_pre_event_hook(EventHook hook);`
- `void add_post_event_hook(EventHook hook);`
- `void add_on_start(SimpleHook hook);`
- `void add_on_stop(SimpleHook hook);`
- `void run_pre_event(const events::Event& event) const;`
- `void run_post_event(const events::Event& event) const;`
- `void run_start() const;`
- `void run_stop() const;`

### `regimeflow/plugins/interfaces.h`

Types:
- `class RegimeDetectorPlugin`
- `class ExecutionModelPlugin`
- `class DataSourcePlugin`
- `class RiskManagerPlugin`
- `class StrategyPlugin`
- `class MetricsPlugin`

Callables:
- `virtual std::unique_ptr<regime::RegimeDetector> create_detector() = 0;`
- `virtual std::unique_ptr<execution::SlippageModel> create_slippage_model() = 0;`
- `virtual std::unique_ptr<execution::CommissionModel> create_commission_model() = 0;`
- `virtual std::unique_ptr<data::DataSource> create_data_source() = 0;`
- `virtual std::unique_ptr<risk::RiskManager> create_risk_manager() = 0;`
- `virtual std::unique_ptr<strategy::Strategy> create_strategy() = 0;`
- `virtual std::unique_ptr<metrics::PerformanceMetric> create_metric() = 0;`

### `regimeflow/plugins/plugin.h`

Types:
- `struct PluginInfo`
- `enum class PluginState`
- `class Plugin`

Callables:
- `virtual ~Plugin() = default;`
- `[[nodiscard]] virtual PluginInfo info() const = 0;`
- `virtual Result<void> on_load() return Ok(); }`
- `virtual Result<void> on_unload() return Ok(); }`
- `virtual Result<void> on_initialize([[maybe_unused]] const Config& config) return Ok(); }`
- `virtual Result<void> on_start() return Ok(); }`
- `virtual Result<void> on_stop() return Ok(); }`
- `[[nodiscard]] virtual std::optional<ConfigSchema> config_schema() const return std::nullopt; }`
- `[[nodiscard]] PluginState state() const return state_; }`
- `void set_state(const PluginState state) state_ = state; }`

### `regimeflow/plugins/registry.h`

Types:
- `class PluginRegistry`

Callables:
- `static PluginRegistry& instance();`
- `using PluginPtr = std::unique_ptr<Plugin, std::function<void(Plugin*)>>;`
- `bool register_plugin(const std::string& type, const std::string& name)`
- `auto factory = []()`
- `bool register_factory(const std::string& type, const std::string& name, std::function<PluginPtr()> factory);`
- `std::unique_ptr<PluginT, std::function<void(PluginT*)>> create( const std::string& type, const std::string& name, const Config& config =})`
- `auto plugin = create_plugin(type, name);`
- `auto* typed = dynamic_cast<PluginT*>(plugin.get());`
- `auto normalized = ::regimeflow::apply_defaults(config, *schema);`
- `plugin->set_state(PluginState::Error);`
- `plugin->set_state(PluginState::Loaded);`
- `auto deleter = plugin.get_deleter();`
- `plugin.release();`
- `auto result = std::unique_ptr<PluginT, std::function<void(PluginT*)>>( typed, [deleter](PluginT* p) deleter(p); });`
- `result->set_state(PluginState::Initialized);`
- `std::vector<std::string> list_types() const;`
- `std::vector<std::string> list_plugins(const std::string& type) const;`
- `std::optional<PluginInfo> get_info(const std::string& type, const std::string& name) const;`
- `Result<void> load_dynamic_plugin(const std::string& path);`
- `Result<void> unload_dynamic_plugin(const std::string& name);`
- `void scan_plugin_directory(const std::string& path);`
- `Result<void> start_plugin(Plugin& plugin);`
- `Result<void> stop_plugin(Plugin& plugin);`
- `std::function<Plugin*()> create;`
- `std::function<void(Plugin*)> destroy;`

## `regime`

### `regimeflow/regime/constant_detector.h`

Types:
- `class ConstantRegimeDetector`

Callables:
- `explicit ConstantRegimeDetector(RegimeType regime) : regime_(regime)}`
- `RegimeState on_bar(const data::Bar& bar) override;`
- `RegimeState on_tick(const data::Tick& tick) override;`
- `void save(const std::string& path) const override;`
- `void load(const std::string& path) override;`
- `void configure(const Config& config) override;`
- `[[nodiscard]] int num_states() const override return 1; }`
- `[[nodiscard]] std::vector<std::string> state_names() const override return"Constant"}; }`

### `regimeflow/regime/ensemble.h`

Types:
- `class EnsembleRegimeDetector`
- `enum class VotingMethod`

Callables:
- `void add_detector(std::unique_ptr<RegimeDetector> detector, double weight = 1.0);`
- `void set_voting_method(VotingMethod method) voting_method_ = method; }`
- `void save(const std::string& path) const override;`
- `void load(const std::string& path) override;`
- `void configure(const Config& config) override;`
- `[[nodiscard]] int num_states() const override;`
- `[[nodiscard]] std::vector<std::string> state_names() const override;`
- `RegimeState on_bar(const data::Bar& bar) override;`
- `RegimeState on_tick(const data::Tick& tick) override;`
- `RegimeState on_book(const data::OrderBook& book) override;`
- `void train(const std::vector<FeatureVector>& data) override;`

### `regimeflow/regime/features.h`

Types:
- `enum class FeatureType`
- `enum class NormalizationMode`
- `class FeatureExtractor`

Callables:
- `explicit FeatureExtractor(int window = 20);`
- `void set_window(int window);`
- `void set_features(std::vector<FeatureType> features);`
- `void set_normalize(bool normalize);`
- `void set_normalization_mode(NormalizationMode mode);`
- `const std::vector<FeatureType>& features() const return features_; }`
- `NormalizationMode normalization_mode() const return normalization_mode_; }`
- `FeatureVector on_bar(const data::Bar& bar);`
- `FeatureVector on_book(const data::OrderBook& book);`
- `void update_cross_asset_features(double market_breadth, double sector_rotation, double correlation_eigen, double risk_appetite);`

### `regimeflow/regime/hmm.h`

Types:
- `struct GaussianParams`
- `class HMMRegimeDetector`

Callables:
- `explicit HMMRegimeDetector(int states = 4, int window = 20);`
- `RegimeState on_bar(const data::Bar& bar) override;`
- `RegimeState on_tick(const data::Tick& tick) override;`
- `RegimeState on_book(const data::OrderBook& book) override;`
- `void train(const std::vector<FeatureVector>& data) override;`
- `void baum_welch(const std::vector<FeatureVector>& data, int max_iter = 50, double tol = 1e-4);`
- `double log_likelihood(const std::vector<FeatureVector>& data) const;`
- `void set_transition_matrix(const std::vector<std::vector<double>>& matrix);`
- `void set_emission_params(std::vector<GaussianParams> params);`
- `void set_features(std::vector<FeatureType> features);`
- `void set_normalize_features(bool normalize);`
- `void set_normalization_mode(NormalizationMode mode);`
- `void save(const std::string& path) const override;`
- `void load(const std::string& path) override;`
- `void configure(const Config& config) override;`
- `int num_states() const override return states_; }`
- `std::vector<std::string> state_names() const override;`

### `regimeflow/regime/kalman_filter.h`

Types:
- `class KalmanFilter1D`

Callables:
- `KalmanFilter1D() = default;`
- `KalmanFilter1D(double process_noise, double measurement_noise) : q_(process_noise), r_(measurement_noise)}`
- `void configure(double process_noise, double measurement_noise)`
- `void reset()`
- `double update(const double measurement)`
- `double k = p_ / (p_ + r_);`
- `x_ = x_ + k * (measurement - x_);`
- `p_ = (1.0 - k) * p_;`

### `regimeflow/regime/regime_detector.h`

Types:
- `class RegimeDetector`

Callables:
- `virtual ~RegimeDetector() = default;`
- `virtual RegimeState on_bar(const data::Bar& bar) = 0;`
- `virtual RegimeState on_tick(const data::Tick& tick) = 0;`
- `virtual RegimeState on_book(const data::OrderBook& book)`
- `double mid = (bid + ask) / 2.0;`
- `virtual void train(const std::vector<FeatureVector>&)}`
- `virtual void save(const std::string& path) const (void)path; }`
- `virtual void load(const std::string& path) (void)path; }`
- `virtual void configure(const Config& config) (void)config; }`
- `[[nodiscard]] virtual int num_states() const return 0; }`
- `[[nodiscard]] virtual std::vector<std::string> state_names() const return}; }`

### `regimeflow/regime/regime_factory.h`

Types:
- `class RegimeFactory`

Callables:
- `static std::unique_ptr<RegimeDetector> create_detector(const Config& config);`

### `regimeflow/regime/state_manager.h`

Types:
- `class RegimeStateManager`

Callables:
- `explicit RegimeStateManager(size_t history_size = 256);`
- `void update(const RegimeState& state);`
- `[[nodiscard]] RegimeType current_regime() const;`
- `[[nodiscard]] double time_in_current_regime() const;`
- `[[nodiscard]] std::vector<RegimeTransition> recent_transitions(size_t n) const;`
- `[[nodiscard]] std::map<RegimeType, double> regime_frequencies() const;`
- `[[nodiscard]] double avg_regime_duration(RegimeType regime) const;`
- `[[nodiscard]] std::vector<std::vector<double>> empirical_transition_matrix() const;`
- `void register_transition_callback( std::function<void(const RegimeTransition&)> callback);`

### `regimeflow/regime/types.h`

Types:
- `enum class RegimeType`
- `struct RegimeState`
- `struct RegimeTransition`

Callables:
- None detected

## `risk`

### `regimeflow/risk/position_sizer.h`

Types:
- `struct PositionSizingContext`
- `class PositionSizer`
- `class FixedFractionalSizer`
- `class VolatilityTargetSizer`
- `class KellySizer`
- `class RegimeScaledSizer`

Callables:
- `virtual ~PositionSizer() = default;`
- `[[nodiscard]] virtual Quantity size(const PositionSizingContext& ctx) const = 0;`
- `explicit FixedFractionalSizer(double risk_per_trade) : risk_per_trade_(risk_per_trade)}`
- `[[nodiscard]] Quantity size(const PositionSizingContext& ctx) const override;`
- `explicit VolatilityTargetSizer(double target_vol) : target_vol_(target_vol)}`
- `explicit KellySizer(const double max_fraction = 1.0) : max_fraction_(max_fraction)}`
- `explicit RegimeScaledSizer(std::unique_ptr<PositionSizer> base) : base_(std::move(base))}`

### `regimeflow/risk/risk_factory.h`

Types:
- `class RiskFactory`

Callables:
- `static RiskManager create_risk_manager(const Config& config);`

### `regimeflow/risk/risk_limits.h`

Types:
- `class RiskLimit`
- `class MaxNotionalLimit`
- `class MaxPositionLimit`
- `class MaxPositionPctLimit`
- `class MaxDrawdownLimit`
- `class MaxGrossExposureLimit`
- `class MaxLeverageLimit`
- `class MaxNetExposureLimit`
- `class RiskManager`
- `class MaxSectorExposureLimit`
- `class MaxIndustryExposureLimit`
- `class MaxCorrelationExposureLimit`
- `struct Config`

Callables:
- `virtual ~RiskLimit() = default;`
- `[[nodiscard]] virtual Result<void> validate(const engine::Order& order, const engine::Portfolio& portfolio) const = 0;`
- `[[nodiscard]] virtual Result<void> validate_portfolio(const engine::Portfolio& portfolio) const`
- `(void)portfolio;`
- `explicit MaxNotionalLimit(double max_notional) : max_notional_(max_notional)}`
- `[[nodiscard]] Result<void> validate(const engine::Order& order, const engine::Portfolio& portfolio) const override`
- `const double order_notional = std::abs(order.quantity) * price;`
- `explicit MaxPositionLimit(const Quantity max_quantity) : max_quantity_(max_quantity)}`
- `const auto pos = portfolio.get_position(order.symbol);`
- `[[nodiscard]] Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override`
- `explicit MaxPositionPctLimit(double max_pct) : max_pct_(max_pct)}`
- `const double equity = portfolio.equity();`
- `const double notional = std::abs(static_cast<double>(projected) * price);`
- `const double notional = std::abs(position.quantity * position.current_price);`
- `explicit MaxDrawdownLimit(double max_drawdown) : max_drawdown_(max_drawdown)}`
- `Result<void> validate(const engine::Order& order, const engine::Portfolio& portfolio) const override`
- `(void)order;`
- `Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override`
- `explicit MaxGrossExposureLimit(const double max_gross_exposure) : max_gross_exposure_(max_gross_exposure)}`
- `explicit MaxLeverageLimit(double max_leverage) : max_leverage_(max_leverage)}`
- `const double projected = portfolio.gross_exposure() + std::abs(order.quantity) * price;`
- `explicit MaxNetExposureLimit(const double max_net_exposure) : max_net_exposure_(max_net_exposure)}`
- `void add_limit(std::unique_ptr<RiskLimit> limit);`
- `Result<void> validate(const engine::Order& order, const engine::Portfolio& portfolio) const;`
- `Result<void> validate_portfolio(const engine::Portfolio& portfolio) const;`
- `void set_regime_limits( std::unordered_map<std::string, std::vector<std::unique_ptr<RiskLimit>>> limits)`
- `regime_limits_ = std::move(limits);`
- `MaxSectorExposureLimit(std::unordered_map<std::string, double> limits, std::unordered_map<std::string, std::string> symbol_to_sector) : limits_(std::move(limits)), symbol_to_sector_(std::move(symbol_to_sector))}`
- `const auto sector = sector_for(order.symbol);`
- `const auto it = limits_.find(sector);`
- `const double sector_notional = sector_exposure(portfolio, sector);`
- `MaxIndustryExposureLimit(std::unordered_map<std::string, double> limits, std::unordered_map<std::string, std::string> symbol_to_industry) : limits_(std::move(limits)), symbol_to_industry_(std::move(symbol_to_industry))}`
- `const auto industry = industry_for(order.symbol);`
- `const auto it = limits_.find(industry);`
- `explicit MaxCorrelationExposureLimit(const Config& cfg, std::unordered_map<std::string, std::string> symbol_to_sector =})`
- `: config_(cfg), symbol_to_sector_(std::move(symbol_to_sector))}`
- `update_history(portfolio);`
- `const auto symbols = portfolio.get_held_symbols();`

### `regimeflow/risk/stop_loss.h`

Types:
- `struct StopLossConfig`
- `class StopLossManager`

Callables:
- `void configure(const StopLossConfig& config);`
- `void on_position_update(const engine::Position& position);`
- `void on_bar(const data::Bar& bar, engine::OrderManager& order_manager);`
- `void on_tick(const data::Tick& tick, engine::OrderManager& order_manager);`

## `strategy`

### `regimeflow/strategy/context.h`

Types:
- `class EventLoop`
- `class RegimeTracker`
- `class StrategyContext`

Callables:
- `StrategyContext(engine::OrderManager* order_manager, engine::Portfolio* portfolio, engine::EventLoop* event_loop, engine::MarketDataCache* market_data, engine::OrderBookCache* order_books, engine::TimerService* timer_service, engine::RegimeTracker* regime_tracker, Config config =});`
- `Result<engine::OrderId> submit_order(engine::Order order) const;`
- `Result<void> cancel_order(engine::OrderId id) const;`
- `const Config& config() const return config_; }`
- `std::optional<T> get_as(const std::string& key) const`
- `engine::Portfolio& portfolio();`
- `const engine::Portfolio& portfolio() const;`
- `std::optional<data::Bar> latest_bar(SymbolId symbol) const;`
- `std::optional<data::Tick> latest_tick(SymbolId symbol) const;`
- `std::optional<data::Quote> latest_quote(SymbolId symbol) const;`
- `std::vector<data::Bar> recent_bars(SymbolId symbol, size_t count) const;`
- `std::optional<data::OrderBook> latest_order_book(SymbolId symbol) const;`
- `const regime::RegimeState& current_regime() const;`
- `void schedule_timer(const std::string& id, Duration interval) const;`
- `void cancel_timer(const std::string& id) const;`
- `Timestamp current_time() const;`

### `regimeflow/strategy/strategies/buy_and_hold.h`

Types:
- `class BuyAndHoldStrategy`

Callables:
- `void initialize(StrategyContext& ctx) override;`
- `void on_bar(const data::Bar& bar) override;`

### `regimeflow/strategy/strategies/harmonic_pattern.h`

Types:
- `class HarmonicPatternStrategy`

Callables:
- `void initialize(StrategyContext& ctx) override;`
- `void on_bar(const data::Bar& bar) override;`

### `regimeflow/strategy/strategies/moving_average_cross.h`

Types:
- `class MovingAverageCrossStrategy`

Callables:
- `void initialize(StrategyContext& ctx) override;`
- `void on_bar(const data::Bar& bar) override;`

### `regimeflow/strategy/strategies/pairs_trading.h`

Types:
- `class PairsTradingStrategy`

Callables:
- `void initialize(StrategyContext& ctx) override;`
- `void on_bar(const data::Bar& bar) override;`

### `regimeflow/strategy/strategy.h`

Types:
- `class Strategy`

Callables:
- `virtual ~Strategy() = default;`
- `void set_context(StrategyContext* ctx) ctx_ = ctx; }`
- `[[nodiscard]] StrategyContext* context() const return ctx_; }`
- `virtual void initialize(StrategyContext& ctx) = 0;`
- `virtual void on_start()}`
- `virtual void on_stop()}`
- `virtual void on_bar([[maybe_unused]] const data::Bar& bar)}`
- `virtual void on_tick([[maybe_unused]] const data::Tick& tick)}`
- `virtual void on_quote([[maybe_unused]] const data::Quote& quote)}`
- `virtual void on_order_book([[maybe_unused]] const data::OrderBook& book)}`
- `virtual void on_order_update([[maybe_unused]] const engine::Order& order)}`
- `virtual void on_fill([[maybe_unused]] const engine::Fill& fill)}`
- `virtual void on_regime_change([[maybe_unused]] const regime::RegimeTransition& transition)}`
- `virtual void on_end_of_day([[maybe_unused]] const Timestamp& date)}`
- `virtual void on_timer([[maybe_unused]] const std::string& timer_id)}`

### `regimeflow/strategy/strategy_factory.h`

Types:
- `class StrategyFactory`

Callables:
- `using Creator = std::function<std::unique_ptr<Strategy>(const Config&)>;`
- `static StrategyFactory& instance();`
- `void register_creator(std::string name, Creator creator);`
- `std::unique_ptr<Strategy> create(const Config& config) const;`
- `void register_builtin_strategies();`

### `regimeflow/strategy/strategy_manager.h`

Types:
- `class StrategyManager`

Callables:
- `void add_strategy(std::unique_ptr<Strategy> strategy);`
- `void clear();`
- `void initialize(StrategyContext& ctx) const;`
- `void start() const;`
- `void stop() const;`
- `void on_bar(const data::Bar& bar) const;`
- `void on_tick(const data::Tick& tick) const;`
- `void on_quote(const data::Quote& quote) const;`
- `void on_order_book(const data::OrderBook& book) const;`
- `void on_order_update(const engine::Order& order) const;`
- `void on_fill(const engine::Fill& fill) const;`
- `void on_regime_change(const regime::RegimeTransition& transition) const;`
- `void on_timer(const std::string& timer_id) const;`

## `walk-forward`

### `regimeflow/walkforward/optimizer.h`

Types:
- `struct WalkForwardConfig`
- `enum class WindowType`
- `enum class OptMethod`
- `struct ParameterDef`
- `enum class Type`
- `enum class Distribution`
- `struct WindowResult`
- `struct WalkForwardResults`
- `class WalkForwardOptimizer`
- `struct RegimeTrainingContext`

Callables:
- `Duration in_sample_period = Duration::months(12);`
- `Duration out_of_sample_period = Duration::months(3);`
- `Duration step_size = Duration::months(3);`
- `using RegimeTrainingHook = std::function<bool(const RegimeTrainingContext&)>;`
- `using RegimeTrainingCallback = std::function<void(const RegimeTrainingContext&)>;`
- `explicit WalkForwardOptimizer(const WalkForwardConfig& config);`
- `WalkForwardResults optimize( const std::vector<ParameterDef>& params, std::function<std::unique_ptr<strategy::Strategy>(const ParameterSet&)> strategy_factory, data::DataSource* data_source, const TimeRange& full_range, std::function<std::unique_ptr<regime::RegimeDetector>()> detector_factory =}) const;`
- `void on_window_complete(std::function<void(const WindowResult&)> callback);`
- `void on_trial_complete(std::function<void(const ParameterSet&, double)> callback);`
- `void on_regime_train(RegimeTrainingHook callback);`
- `void on_regime_trained(RegimeTrainingCallback callback);`
- `void cancel();`

