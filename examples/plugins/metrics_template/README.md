# Metrics Plugin Template

Minimal dynamic plugin template for custom performance metrics.

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

`plugin_type` must return `metrics`.

## Runtime Config

```yaml
plugins:
  search_paths:
    - examples/plugins/metrics_template/build
  load:
    - libregimeflow_metrics_template.so

metrics:
  custom:
    - metrics_template
```

Use this template for metrics that can be computed from an equity curve without coupling to broker or strategy internals.
