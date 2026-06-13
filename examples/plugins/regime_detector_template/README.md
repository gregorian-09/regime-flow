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
  params:
    model:
      version: "2026-06-12.hmm-demo"
      training_start_us: 1704067200000000
      training_end_us: 1735689600000000
      feature_schema: "template:v1:close_return,volatility,trend"
      parameter_digest: "sha256:replace-with-training-parameter-digest"
```

The template forwards model governance fields through `model_metadata()` so live audit events
can identify the detector version, training window, feature schema, and parameter digest used
for every runtime regime transition.
