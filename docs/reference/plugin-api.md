# Plugin API

This page documents how to build plugins that integrate with the RegimeFlow runtime. Plugins can extend regime detection, strategies, execution models, data sources, risk managers, and metrics.

## Plugin Types

Supported plugin interfaces (from `include/regimeflow/plugins/interfaces.h`):

- `RegimeDetectorPlugin`
- `ExecutionModelPlugin`
- `DataSourcePlugin`
- `RiskManagerPlugin`
- `StrategyPlugin`
- `MetricsPlugin`

## Required Exports (Dynamic Plugins)

Dynamic plugins must export these C symbols:

- `create_plugin()` returns a new plugin instance.
- `destroy_plugin(plugin)` destroys a plugin instance.
- `plugin_type()` returns the plugin category string.
- `plugin_name()` returns the plugin name string.
- `regimeflow_abi_version()` returns `REGIMEFLOW_ABI_VERSION`.

These are used by `PluginRegistry::load_dynamic_plugin`.

## ABI Versioning

The ABI version is defined in `include/regimeflow/plugins/plugin.h`:

- `REGIMEFLOW_ABI_VERSION` is currently `"1.0"`.

If the ABI version does not match, the plugin load will fail with `Plugin ABI version mismatch`.

## Plugin Lifecycle

The base class `plugins::Plugin` provides:

- `on_load()` when the library is loaded.
- `on_initialize(config)` when configuration is applied.
- `on_start()` and `on_stop()` for runtime state.
- `on_unload()` when the library is unloaded.

`PluginRegistry::create` validates optional config schemas, applies defaults, and calls `on_initialize`.

## Config Schemas

Plugins can return a `ConfigSchema` via `config_schema()`. If present, the registry:

1. Applies defaults via `apply_defaults`.
2. Validates required fields via `validate_config`.
3. Calls `on_initialize` with the normalized config.

## Minimal Plugin Skeleton

```cpp
#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"

class MyDetectorPlugin : public regimeflow::plugins::RegimeDetectorPlugin {
public:
    regimeflow::plugins::PluginInfo info() const override {
        return {"my_detector", "0.1.0", "Custom detector", "YourTeam", {}};
    }

    regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override {
        config_ = config;
        return regimeflow::Ok();
    }

    std::unique_ptr<regimeflow::regime::RegimeDetector> create_detector() override {
        auto det = std::make_unique<MyDetector>();
        det->configure(config_);
        return det;
    }

private:
    regimeflow::Config config_;
};

extern "C" {
    REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
        return new MyDetectorPlugin();
    }
    REGIMEFLOW_EXPORT void destroy_plugin(const regimeflow::plugins::Plugin* plugin) {
        delete plugin;
    }
    REGIMEFLOW_EXPORT const char* plugin_type() { return "regime_detector"; }
    REGIMEFLOW_EXPORT const char* plugin_name() { return "my_detector"; }
    REGIMEFLOW_EXPORT const char* regimeflow_abi_version() { return REGIMEFLOW_ABI_VERSION; }
}
```

## Build Guidance

Plugins are standard shared libraries:

- Linux: `.so`
- macOS: `.dylib`
- Windows: `.dll`

CMake example:

```cmake
add_library(my_detector SHARED my_detector.cpp)
target_include_directories(my_detector PRIVATE ${REGIMEFLOW_INCLUDE_DIRS})
```

## Runtime Loading

Use `plugins.search_paths` and `plugins.load` in config files, or call `PluginRegistry::load_dynamic_plugin` directly.

Example:

```yaml
plugins:
  search_paths:
    - examples/plugins/custom_regime/build
  load:
    - libcustom_regime_detector.so
```
