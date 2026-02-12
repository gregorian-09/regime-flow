# Package: `regimeflow::plugins`

## Summary

Plugin subsystem for extending core functionality (strategies, risk policies, metrics, and hooks). Defines plugin interfaces, hook points, and the runtime registry.

Related diagrams:
- [Plugin API](../plugin-api.md)
- [Plugins](../plugins.md)

## File Index

| File | Purpose |
| --- | --- |
| `regimeflow/plugins/hooks.h` | Hook points and lifecycle callbacks. |
| `regimeflow/plugins/interfaces.h` | Plugin interfaces and contracts. |
| `regimeflow/plugins/plugin.h` | Base plugin type. |
| `regimeflow/plugins/registry.h` | Dynamic plugin registry. |

## Type Index

| Type | Description |
| --- | --- |
| `Plugin` | Base plugin class. |
| `PluginRegistry` | Loads, tracks, and resolves plugins. |
| `HookManager` | Manages hook execution. |

## Lifecycle & Usage Notes

- Plugins are discovered through the registry and executed through hook points.
- Keep plugin interfaces dependency-light to avoid leaking internal types.

## Type Details

### `Plugin`

Base class for all plugins. Defines initialization and teardown hooks.

Methods:

| Method | Description |
| --- | --- |
| `info()` | Return plugin metadata. |
| `on_load()` | Called on load. |
| `on_unload()` | Called on unload. |
| `on_initialize(config)` | Initialize with config. |
| `on_start()` | Called on start. |
| `on_stop()` | Called on stop. |
| `config_schema()` | Optional config schema. |
| `state()` | Current plugin state. |
| `set_state(state)` | Set plugin state. |

### `PluginInfo` / `PluginState`

Plugin metadata and lifecycle state enum.

### `PluginState`

Plugin lifecycle state enum.

### `PluginRegistry`

Resolves and loads plugins by name and shared library path.

Methods:

| Method | Description |
| --- | --- |
| `instance()` | Access singleton registry. |
| `register_plugin<T>(type, name)` | Register plugin class. |
| `register_factory(type, name, factory)` | Register factory callback. |
| `create<T>(type, name, config)` | Create and initialize plugin. |
| `list_types()` | List registered plugin types. |
| `list_plugins(type)` | List plugins for a type. |
| `get_info(type, name)` | Get plugin metadata. |
| `load_dynamic_plugin(path)` | Load dynamic plugin library. |
| `unload_dynamic_plugin(name)` | Unload dynamic plugin. |
| `scan_plugin_directory(path)` | Scan directory for plugins. |
| `start_plugin(plugin)` | Invoke `on_start`. |
| `stop_plugin(plugin)` | Invoke `on_stop`. |

### `HookManager`

Invokes pre/post hooks for engine lifecycle and strategy execution.

Methods:

| Method | Description |
| --- | --- |
| `register_hook(type, hook, priority)` | Register a generic hook. |
| `register_backtest_start(hook, priority)` | Register backtest-start hook. |
| `register_backtest_end(hook, priority)` | Register backtest-end hook. |
| `register_day_start(hook, priority)` | Register day-start hook. |
| `register_day_end(hook, priority)` | Register day-end hook. |
| `register_bar(hook, priority)` | Register bar hook. |
| `register_tick(hook, priority)` | Register tick hook. |
| `register_quote(hook, priority)` | Register quote hook. |
| `register_book(hook, priority)` | Register book hook. |
| `register_timer(hook, priority)` | Register timer hook. |
| `register_order_submit(hook, priority)` | Register order-submit hook. |
| `register_fill(hook, priority)` | Register fill hook. |
| `register_regime_change(hook, priority)` | Register regime-change hook. |
| `invoke(type, ctx)` | Invoke hooks for a type. |

### `HookContext`

Context passed to hook callbacks.

Methods:

| Method | Description |
| --- | --- |
| `portfolio()` | Access portfolio snapshot. |
| `market()` | Access market data cache. |
| `regime()` | Access current regime. |
| `current_time()` | Current simulated time. |
| `bar()` / `tick()` / `quote()` / `book()` | Access market payload. |
| `fill()` | Access fill payload. |
| `regime_change()` | Access regime transition payload. |
| `order()` | Access mutable order payload. |
| `results()` | Access backtest results. |
| `timer_id()` | Access timer ID. |
| `set_bar(...)` / `set_tick(...)` / `set_quote(...)` / `set_book(...)` | Attach payloads. |
| `set_fill(...)` / `set_regime_change(...)` / `set_order(...)` | Attach payloads. |
| `set_results(...)` / `set_timer_id(...)` | Attach payloads. |
| `modify_order(order)` | Replace order in context. |
| `inject_event(event)` | Inject an event into the queue. |

### `HookType` / `HookResult`

Hook categories and return directives.

### `HookResult`

Hook return directive enum.

### `HookSystem`

Facade for hook registration and invocation in the engine.

### `Plugin Interfaces`

Standard plugin interfaces for extensibility.

Types:

| Type | Description |
| --- | --- |
| `StrategyPlugin` | Strategy plugin interface. |
| `RiskManagerPlugin` | Risk manager plugin interface. |
| `RegimeDetectorPlugin` | Regime detector plugin interface. |
| `ExecutionModelPlugin` | Execution model plugin interface. |
| `MetricsPlugin` | Metrics plugin interface. |
| `DataSourcePlugin` | Data source plugin interface. |

### `StrategyPlugin`

Strategy plugin interface.

### `RiskManagerPlugin`

Risk manager plugin interface.

### `RegimeDetectorPlugin`

Regime detector plugin interface.

### `ExecutionModelPlugin`

Execution model plugin interface.

### `MetricsPlugin`

Metrics plugin interface.

### `DataSourcePlugin`

Data source plugin interface.

## Method Details

Type Hints:

- `type` → `std::string`
- `name` → `std::string`
- `config` → `Config`
- `ctx` → `HookContext`

### `Plugin`

#### `info()`
Parameters: None.
Returns: `PluginInfo`.
Throws: None.

#### `on_load()` / `on_unload()` / `on_initialize(config)` / `on_start()` / `on_stop()`
Parameters: optional config.
Returns: `Result<void>`.
Throws: `Error::PluginError` on failure.

### `PluginRegistry`

#### `register_plugin<T>(type, name)` / `register_factory(type, name, factory)`
Parameters: type, name, factory.
Returns: `bool`.
Throws: None.

#### `create<T>(type, name, config)`
Parameters: type, name, config.
Returns: plugin instance or null.
Throws: None. Returns `nullptr` when the plugin is missing, has a type mismatch, fails schema validation, or fails `on_initialize()`.

#### `load_dynamic_plugin(path)` / `unload_dynamic_plugin(name)`
Parameters: path or name.
Returns: `Result<void>`.
Throws: `Error::InvalidState` (unsupported platform, missing entry points, ABI mismatch, or plugin creation failure), `Error::IoError` (load failure), `Error::InvalidArgument` (missing plugin name), `Error::AlreadyExists` (duplicate name), `Error::NotFound` (unload missing plugin).

### `HookManager`

#### `register_*`
Parameters: hook callback and priority.
Returns: `void`.
Throws: None.

#### `invoke(type, ctx)`
Parameters: hook type and context.
Returns: `HookResult`.
Throws: None.

## Usage Examples

```cpp
#include "regimeflow/plugins/registry.h"

auto& registry = regimeflow::plugins::PluginRegistry::instance();
auto plugin = registry.create<regimeflow::plugins::Plugin>("strategy", "my_strategy");
```

## See Also

- [Plugin API](../plugin-api.md)
- [Plugins](../plugins.md)
