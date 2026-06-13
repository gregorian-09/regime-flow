# Broker Adapter Plugin Template

This template shows the minimal shape of a dynamic live broker adapter plugin.
Use it when a broker does not belong in RegimeFlow core, but should still satisfy
the same `BrokerAdapter` capability, market-data, order, account, and callback
contracts used by the live engine.

Required dynamic ABI exports:

- `create_plugin`
- `destroy_plugin`
- `plugin_type` returning `broker_adapter`
- `plugin_name` returning `broker_adapter_template`
- `regimeflow_abi_version`

## Build

```bash
cmake -S examples/plugins/broker_adapter_template -B examples/plugins/broker_adapter_template/build \
  -DREGIMEFLOW_SOURCE_DIR=$(pwd)
cmake --build examples/plugins/broker_adapter_template/build
```

## Runtime Contract

A real broker adapter must:

- expose an accurate `capabilities()` matrix before orders are submitted
- fail closed when disconnected or when broker credentials are missing
- map unknown broker order states to `LiveOrderStatus::Error`
- emit execution reports and positions through the registered callbacks
- avoid logging raw credentials or broker secrets

The live engine can instantiate statically registered broker plugins by using
`broker_type` equal to the plugin name. Dynamic libraries can be loaded through
the existing plugin registry before the live engine is constructed.
