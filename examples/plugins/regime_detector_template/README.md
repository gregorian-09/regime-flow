# Regime Detector Plugin Template

Minimal dynamic plugin template for custom regime detectors.

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

`plugin_type` must return `regime_detector`.

## Runtime Config

```yaml
plugins:
  search_paths:
    - examples/plugins/regime_detector_template/build
  load:
    - libregimeflow_regime_detector_template.so

regime:
  type: regime_detector_template
  params: {}
```
