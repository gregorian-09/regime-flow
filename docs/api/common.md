# Package: `regimeflow::common`

## Summary

Core utilities and foundational types used across the system: configuration loading, result handling, time primitives, hashing, lock-free queues, memory helpers, and lightweight data structures. This package is dependency-light and safe to use in low-level subsystems.

Related diagrams:
- [Component Diagram](../diagrams/component-diagram.md)
- [Structural Flow](../diagrams/structural-flow.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/common/config.h` | User-facing configuration object and access helpers. |
| `regimeflow/common/config_schema.h` | Configuration schema definitions and validation contracts. |
| `regimeflow/common/json.h` | JSON parse/emit utilities and safe helpers. |
| `regimeflow/common/lru_cache.h` | Bounded LRU cache for hot data. |
| `regimeflow/common/memory.h` | Memory utilities and safe allocation helpers. |
| `regimeflow/common/mpsc_queue.h` | Multi-producer/single-consumer queue. |
| `regimeflow/common/result.h` | `Result<T>` error propagation type. |
| `regimeflow/common/sha256.h` | SHA-256 hashing helper. |
| `regimeflow/common/spsc_queue.h` | Single-producer/single-consumer queue. |
| `regimeflow/common/time.h` | Timestamp and time conversion utilities. |
| `regimeflow/common/types.h` | Common typedefs and shared enums. |
| `regimeflow/common/yaml_config.h` | YAML configuration loader and overrides. |

## Type Index

| Type | Description |
| --- | --- |
| `Config` | Central configuration object used by engines and services. |
| `ConfigSchema` | Schema metadata used to validate config files. |
| `Json` helpers | Minimal JSON parse/emit helpers with guardrails. |
| `LruCache<Key, Value>` | Fixed-capacity LRU cache. |
| `Result<T>` / `Result<void>` | Error-or-value return type used throughout. |
| `Timestamp` | Monotonic / wall-clock time abstraction. |
| `MpscQueue<T>` | Lock-free MPSC queue. |
| `SpscQueue<T>` | Lock-free SPSC queue. |
| `Sha256` helpers | Deterministic hashing for identifiers and cache keys. |

## Lifecycle & Usage Notes

- `Result<T>` is the preferred error surface for non-throwing paths.
- `Timestamp` is used consistently across events, bars, and fills to avoid time-domain drift.
- Queue types are safe for the event loop and live adapters; keep usage aligned with producer/consumer expectations.
- `Config` and `YamlConfig` are used by both backtest and live engine factories; config validation should happen before engine initialization.

## Type Details

### `Config`

Central configuration object for engines, data sources, and risk policies. Provides typed accessors and validates against `ConfigSchema`.

Methods:

| Method | Description |
| --- | --- |
| `Config()` | Construct empty config. |
| `Config(values)` | Construct from object map. |
| `has(key)` | Check top-level key presence. |
| `get(key)` | Get top-level value pointer. |
| `get_path(path)` | Get nested value by dotted path. |
| `get_as<T>(key)` | Typed fetch by key or path. |
| `set(key, value)` | Set top-level value. |
| `set_path(path, value)` | Set nested value by dotted path. |

Method Details:

Type Hints:

- `key` → `std::string`
- `path` → `std::string` (dotted path)
- `value` → `ConfigValue`
- `config` → `Config`
- `schema` → `ConfigSchema`

#### `Config()`
Parameters: None.
Returns: Instance.
Throws: None.

#### `Config(values)`
Parameters: `values` root object map.
Returns: Instance.
Throws: None.

#### `has(key)`
Parameters: `key` top-level key.
Returns: `bool`.
Throws: None.

#### `get(key)`
Parameters: `key` top-level key.
Returns: Pointer to `ConfigValue` or null.
Throws: None.

#### `get_path(path)`
Parameters: `path` dotted path.
Returns: Pointer to `ConfigValue` or null.
Throws: None.

#### `get_as<T>(key)`
Parameters: `key` key or dotted path.
Returns: Optional `T`.
Throws: None.

#### `set(key, value)`
Parameters: `key` top-level key; `value` config value.
Returns: `void`.
Throws: None.

#### `set_path(path, value)`
Parameters: `path` dotted path; `value` config value.
Returns: `void`.
Throws: None.
Related:
| Helper | Description |
| --- | --- |
| `YamlConfigLoader::load_file(path)` | Load config from YAML. |

### `ConfigSchema`

Schema definition for configuration validation. Ensures required keys, types, and ranges are enforced.

Members and Helpers:

| Member | Description |
| --- | --- |
| `properties` | Property map of schema entries. |
| `config_value_matches(value, type)` | Check value against schema type. |
| `validate_config(config, schema)` | Validate config against schema. |
| `apply_defaults(input, schema)` | Apply schema defaults to config. |

Method Details:

#### `validate_config(config, schema)`
Parameters: `config` to validate; `schema` schema definition.
Returns: `Result<void>`.
Throws: `Error::ConfigError` for missing/invalid fields.

#### `apply_defaults(input, schema)`
Parameters: `input` config; `schema` schema definition.
Returns: Config with defaults applied.
Throws: None.
### `ConfigValue`

Variant-backed configuration value container.

Methods:

| Method | Description |
| --- | --- |
| `ConfigValue()` | Construct empty value. |
| `ConfigValue(bool/int/double/string)` | Construct scalar value. |
| `ConfigValue(Array/Object)` | Construct array/object value. |
| `raw()` | Access underlying variant. |
| `get_if<T>()` | Typed pointer accessor. |

Method Details:

#### `ConfigValue()`
Parameters: None.
Returns: Instance.
Throws: None.

#### `ConfigValue(bool/int/double/string/array/object)`
Parameters: Typed value.
Returns: Instance.
Throws: None.

#### `raw()`
Parameters: None.
Returns: Underlying variant.
Throws: None.

#### `get_if<T>()`
Parameters: None.
Returns: Typed pointer or null.
Throws: None.
### `Result<T>` / `Result<void>`

Error-or-value wrapper for non-throwing control flow. Propagates error messages and error codes.

Methods:

| Method | Description |
| --- | --- |
| `Result(value)` | Construct success result. |
| `Result(error)` | Construct error result. |
| `is_ok()` | True if success. |
| `is_err()` | True if error. |
| `value()` | Access value or throw on error. |
| `error()` | Access stored error. |
| `map(f)` | Map value to new Result. |
| `and_then(f)` | Chain fallible computation. |
| `value_or(default)` | Return value or default. |

Method Details:

#### `Result(value)`
Parameters: `value` to store.
Returns: Instance.
Throws: None.

#### `Result(error)`
Parameters: `error` to store.
Returns: Instance.
Throws: None.

#### `is_ok()` / `is_err()`
Parameters: None.
Returns: `bool`.
Throws: None.

#### `value()`
Parameters: None.
Returns: Value reference.
Throws: `std::runtime_error` on error state.

#### `error()`
Parameters: None.
Returns: Error reference.
Throws: None.

#### `map(f)`
Parameters: `f` mapping function.
Returns: Mapped `Result`.
Throws: None.

#### `and_then(f)`
Parameters: `f` continuation.
Returns: Result of continuation.
Throws: None.

#### `value_or(default)`
Parameters: `default` value.
Returns: Value or default.
Throws: None.
Helpers:

| Helper | Description |
| --- | --- |
| `Ok(value)` / `Ok()` | Construct successful result. |
| `Err(code, fmt, ...)` | Construct error with message. |
| `TRY(expr)` | Propagate error or unwrap value. |

### `Error`

Structured error information with code, message, and source location.

Methods:

| Method | Description |
| --- | --- |
| `Error()` | Construct unknown error. |
| `Error(code, msg)` | Construct error with code/message. |
| `to_string()` | Render error as string. |

Method Details:

#### `Error()`
Parameters: None.
Returns: Instance.
Throws: None.

#### `Error(code, msg)`
Parameters: error code and message.
Returns: Instance.
Throws: None.

#### `to_string()`
Parameters: None.
Returns: String.
Throws: None.
### `Code`

Error code enumeration (`Error::Code`).

### `Timestamp`

Unified time abstraction used across the engine. Avoids mixing wall-clock and monotonic times.

Methods:

| Method | Description |
| --- | --- |
| `Timestamp()` | Construct epoch timestamp. |
| `Timestamp(microseconds)` | Construct from epoch microseconds. |
| `now()` | Current wall-clock timestamp. |
| `from_string(str, fmt)` | Parse from string with format. |
| `from_date(year, month, day)` | Construct from calendar date. |
| `microseconds()` | Microseconds since epoch. |
| `milliseconds()` | Milliseconds since epoch. |
| `seconds()` | Seconds since epoch. |
| `to_string(fmt)` | Format as string. |
| `operator+(duration)` | Add duration. |
| `operator-(duration)` | Subtract duration. |
| `operator-(timestamp)` | Difference as duration. |

Method Details:

#### `Timestamp()`
Parameters: None.
Returns: Instance.
Throws: None.

#### `Timestamp(microseconds)`
Parameters: microseconds since epoch.
Returns: Instance.
Throws: None.

#### `now()`
Parameters: None.
Returns: Current timestamp.
Throws: None.

#### `from_string(str, fmt)`
Parameters: string and format.
Returns: Parsed timestamp.
Throws: `Error::ParseError` on invalid input.

#### `from_date(year, month, day)`
Parameters: date parts.
Returns: Timestamp at midnight.
Throws: None.

#### `microseconds()` / `milliseconds()` / `seconds()`
Parameters: None.
Returns: Time in units.
Throws: None.

#### `to_string(fmt)`
Parameters: format string.
Returns: formatted time string.
Throws: None.

#### `operator+(duration)` / `operator-(duration)`
Parameters: duration.
Returns: New timestamp.
Throws: None.

#### `operator-(timestamp)`
Parameters: timestamp.
Returns: duration.
Throws: None.
### `Duration`

Duration wrapper stored in microseconds.

Methods:

| Method | Description |
| --- | --- |
| `microseconds(us)` | Construct duration from microseconds. |
| `milliseconds(ms)` | Construct duration from milliseconds. |
| `seconds(s)` | Construct duration from seconds. |
| `minutes(m)` | Construct duration from minutes. |
| `hours(h)` | Construct duration from hours. |
| `days(d)` | Construct duration from days. |
| `months(m)` | Construct duration from months (30-day). |
| `total_microseconds()` | Total microseconds. |
| `total_milliseconds()` | Total milliseconds. |
| `total_seconds()` | Total seconds. |

Method Details:

#### `microseconds/ms/seconds/minutes/hours/days/months(...)`
Parameters: numeric value.
Returns: Duration.
Throws: None.

#### `total_microseconds()` / `total_milliseconds()` / `total_seconds()`
Parameters: None.
Returns: Total time in units.
Throws: None.
### `TimeRange`

Inclusive time range with convenience helpers.

Methods:

| Method | Description |
| --- | --- |
| `contains(t)` | True if timestamp in range. |
| `duration()` | Duration between end and start. |

Method Details:

#### `contains(t)`
Parameters: timestamp `t`.
Returns: `bool`.
Throws: None.

#### `duration()`
Parameters: None.
Returns: `Duration`.
Throws: None.
### `MpscQueue`

Template alias: `MpscQueue<T>`.

### `SpscQueue`

Template alias: `SpscQueue<T, Capacity>`.

### `MpscQueue<T>` / `SpscQueue<T>`

Lock-free queues used in event dispatch and live adapters. Ensure producer/consumer topology matches the queue type.

Methods:

| Method | Description |
| --- | --- |
| `MpscQueue()` | Construct with dummy node. |
| `push(value)` | Enqueue item (copy/move). |
| `pop(out)` | Dequeue item if available. |
| `empty()` | Check if empty. |
| `SpscQueue::push(value)` | Enqueue item (single producer). |
| `SpscQueue::pop(out)` | Dequeue item (single consumer). |
| `SpscQueue::empty()` | Check if empty. |

Method Details:

#### `MpscQueue()`
Parameters: None.
Returns: Instance.
Throws: None.

#### `push(value)` / `push(std::move(value))`
Parameters: value.
Returns: `void`.
Throws: None.

#### `pop(out)`
Parameters: output reference.
Returns: `bool`.
Throws: None.

#### `empty()`
Parameters: None.
Returns: `bool`.
Throws: None.

#### `SpscQueue::push(value)`
Parameters: value.
Returns: `bool` (false if full).
Throws: None.

#### `SpscQueue::pop(out)`
Parameters: output reference.
Returns: `bool` (false if empty).
Throws: None.

#### `SpscQueue::empty()`
Parameters: None.
Returns: `bool`.
Throws: None.
### `LRUCache`

Template alias: `LRUCache<Key, Value>`.

### `LruCache<Key, Value>`

Bounded cache for hot-path data. Used by data sources and metadata fetches where repeated reads are common.

Methods:

| Method | Description |
| --- | --- |
| `LRUCache(capacity)` | Construct cache. |
| `set_capacity(capacity)` | Update capacity. |
| `capacity()` | Current capacity. |
| `size()` | Current size. |
| `get(key)` | Fetch value and mark MRU. |
| `put(key, value)` | Insert/update value. |
| `clear()` | Clear cache. |

Method Details:

#### `LRUCache(capacity)`
Parameters: capacity.
Returns: Instance.
Throws: None.

#### `set_capacity(capacity)`
Parameters: capacity.
Returns: `void`.
Throws: None.

#### `capacity()` / `size()`
Parameters: None.
Returns: `size_t`.
Throws: None.

#### `get(key)`
Parameters: key.
Returns: Optional value.
Throws: None.

#### `put(key, value)`
Parameters: key, value.
Returns: `void`.
Throws: None.

#### `clear()`
Parameters: None.
Returns: `void`.
Throws: None.
### `YamlConfig`

YAML-based configuration loader with overrides. This is the canonical config entrypoint for CLI and services.

Methods:

| Method | Description |
| --- | --- |
| `YamlConfigLoader::load_file(path)` | Load config from YAML. |

Method Details:

#### `load_file(path)`
Parameters: YAML path.
Returns: `Config`.
Throws: `Error::IoError`/`Error::ParseError`.
### `JsonValue`

Lightweight JSON value representation.

Methods:

| Method | Description |
| --- | --- |
| `JsonValue()` | Construct null value. |
| `JsonValue(nullptr/bool/number/string/array/object)` | Construct typed JSON value. |
| `is_null()` / `is_bool()` / `is_number()` / `is_string()` / `is_array()` / `is_object()` | Type predicates. |
| `as_bool()` / `as_number()` / `as_string()` / `as_array()` / `as_object()` | Typed accessors. |

Functions:

| Function | Description |
| --- | --- |
| `parse_json(input)` | Parse JSON text into `JsonValue`. |

Method Details:

#### `parse_json(input)`
Parameters: JSON text.
Returns: `Result<JsonValue>`.
Throws: `Error::ParseError`.
### `Sha256`

Streaming SHA-256 hash implementation.

Methods:

| Method | Description |
| --- | --- |
| `Sha256()` | Construct hasher. |
| `update(data, len)` | Add bytes to hash. |
| `digest()` | Finalize and return digest. |

Method Details:

#### `Sha256()`
Parameters: None.
Returns: Instance.
Throws: None.

#### `update(data, len)`
Parameters: pointer and length.
Returns: `void`.
Throws: None.

#### `digest()`
Parameters: None.
Returns: 32-byte digest.
Throws: None.
### `SymbolRegistry`

Thread-safe registry mapping symbols to compact IDs.

Methods:

| Method | Description |
| --- | --- |
| `instance()` | Access singleton registry. |
| `intern(symbol)` | Intern a symbol string. |
| `lookup(id)` | Lookup symbol string by ID. |

Method Details:

#### `instance()`
Parameters: None.
Returns: Singleton reference.
Throws: None.

#### `intern(symbol)`
Parameters: symbol string.
Returns: `SymbolId`.
Throws: None.

#### `lookup(id)`
Parameters: symbol ID.
Returns: symbol string.
Throws: `std::out_of_range` if ID invalid.
### `MonotonicArena`

Simple monotonic arena allocator.

Methods:

| Method | Description |
| --- | --- |
| `MonotonicArena(block_size)` | Construct with block size. |
| `allocate(bytes, alignment)` | Allocate bytes from arena. |
| `reset()` | Reset arena and free blocks. |

Method Details:

#### `MonotonicArena(block_size)`
Parameters: block size bytes.
Returns: Instance.
Throws: None.

#### `allocate(bytes, alignment)`
Parameters: size and alignment.
Returns: pointer.
Throws: `std::bad_alloc` on allocation failure.

#### `reset()`
Parameters: None.
Returns: `void`.
Throws: None.
### `PoolAllocator`

Template alias: `PoolAllocator<T>`.

### `PoolAllocator<T>`

Thread-safe object pool allocator.

Methods:

| Method | Description |
| --- | --- |
| `PoolAllocator(capacity)` | Construct with initial capacity. |
| `allocate()` | Allocate an object. |
| `deallocate(ptr)` | Return an object to pool. |

Method Details:

#### `PoolAllocator(capacity)`
Parameters: initial capacity.
Returns: Instance.
Throws: None.

#### `allocate()`
Parameters: None.
Returns: pointer to object.
Throws: `std::bad_alloc` on allocation failure.

#### `deallocate(ptr)`
Parameters: object pointer.
Returns: `void`.
Throws: None.
### `AssetClass`

Asset class enumeration used across data and risk.

Values:

| Value | Description |
| --- | --- |
| `Equity` | Stocks and ETFs. |
| `Futures` | Futures contracts. |
| `Forex` | Foreign exchange. |
| `Crypto` | Digital assets. |
| `Options` | Options contracts. |
| `Other` | Other/unknown. |

### `YamlConfigLoader`

YAML configuration loader.

Methods:

| Method | Description |
| --- | --- |
| `load_file(path)` | Load config from YAML file. |

## Usage Examples

```cpp
#include "regimeflow/common/config.h"
#include "regimeflow/common/yaml_config.h"

regimeflow::Config config = regimeflow::YamlConfigLoader::load_file("config.yml");
auto risk_limit = config.get_as<double>("risk.limits.max_drawdown");
if (!risk_limit) {
    // handle missing config
}
```

## See Also

- [Configuration Reference](../reference/config-reference.md)
- [Data Validation](../how-to/data-validation.md)
