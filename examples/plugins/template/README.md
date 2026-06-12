# RegimeFlow Plugin Template

This directory is a minimal strategy-plugin SDK starting point. Copy it when you want a clean plugin project without modifying RegimeFlow core code.

## Build

From this directory:

```bash
cmake -S . -B build -DREGIMEFLOW_SOURCE_DIR=/path/to/regime-flow
cmake --build build
```

The output is a shared library:

- Linux: `libregimeflow_strategy_template.so`
- macOS: `libregimeflow_strategy_template.dylib`
- Windows: `regimeflow_strategy_template.dll`

## Required ABI Exports

Every dynamic plugin must export:

- `create_plugin`
- `destroy_plugin`
- `plugin_type`
- `plugin_name`
- `regimeflow_abi_version`

The registry rejects missing symbols or an ABI version mismatch.

## Runtime Config

```yaml
plugins:
  search_paths:
    - examples/plugins/template/build
  load:
    - libregimeflow_strategy_template.so

strategy:
  name: strategy_template
```

Use the platform-specific library filename on macOS or Windows.
