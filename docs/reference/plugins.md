# Plugins

Plugins let you extend RegimeFlow without changing core code. The registry can load plugins statically (compiled into the binary) or dynamically (shared library).

## Plugin Types

- `regime_detector`
- `strategy`
- `execution_model`
- `data_source`
- `risk_manager`
- `metrics`

These map to the interfaces in `include/regimeflow/plugins/interfaces.h`.

## Plugin Discovery

The registry searches configured directories and loads specific plugin files:

```yaml
plugins:
  search_paths:
    - examples/plugins/custom_regime/build
  load:
    - libcustom_regime_detector.so
    - libcustom_regime_strategy.so
```

Discovery behavior:

- Relative plugin paths are resolved against `plugins.search_paths`.
- `.so`, `.dylib`, or `.dll` are accepted depending on OS.
- The plugin is validated for required symbols and ABI version.

## Registry Lifecycle

- `load_dynamic_plugin(path)` loads and registers a plugin factory.
- `create(type, name, config)` instantiates and initializes the plugin.
- `start_plugin(plugin)` invokes `on_start` and sets state to `Active`.
- `stop_plugin(plugin)` invokes `on_stop` and sets state to `Stopped`.

## Example: Custom Regime Detector

The custom detector in `examples/plugins/custom_regime/` exports:

- `create_plugin`, `destroy_plugin`
- `plugin_type` = `regime_detector`
- `plugin_name` = `custom_regime`
- `regimeflow_abi_version`

This allows:

```yaml
regime:
  detector: custom_regime
  params:
    window: 60
    trend_threshold: 0.02
    vol_threshold: 0.015
```

## Example: Custom Strategy Plugin

The custom strategy in `examples/plugins/custom_regime/` exports:

- `plugin_type` = `strategy`
- `plugin_name` = `custom_regime_strategy`

And can be selected with:

```yaml
strategy:
  name: custom_regime_strategy
  params:
    symbol: AAPL
    base_qty: 10
    trend_qty: 20
    stress_qty: 5
```

## Error Handling

Common failure modes:

- **Missing symbols**: plugin library does not export required C symbols.
- **ABI mismatch**: plugin compiled against a different `REGIMEFLOW_ABI_VERSION`.
- **Duplicate name**: plugin name already loaded.

See `operations/troubleshooting.md` for resolution steps.
