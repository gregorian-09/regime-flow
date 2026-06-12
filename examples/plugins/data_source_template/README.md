# Data Source Plugin Template

Minimal dynamic plugin template for custom market-data sources.

## Build

```bash
cmake -S . -B build -DREGIMEFLOW_SOURCE_DIR=/path/to/regime-flow
cmake --build build
```

## Required ABI Exports

- `create_plugin`
- `destroy_plugin`
- `plugin_type`
- `plugin_name`
- `regimeflow_abi_version`

`plugin_type` must return `data_source`.

## Runtime Config

```yaml
plugins:
  search_paths:
    - examples/plugins/data_source_template/build
  load:
    - libregimeflow_data_source_template.so

data:
  source: data_source_template
  params:
    symbol: AAPL
```

Replace the in-memory sample data with database, object-store, or broker historical-data access.
Keep credentials out of the plugin binary and pass secrets through the normal RegimeFlow config path.
