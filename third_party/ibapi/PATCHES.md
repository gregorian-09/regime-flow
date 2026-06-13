# Interactive Brokers API Local Patch Notes

## Current State

No direct source patches are applied inside the vendored IB API C++ files.

RegimeFlow integration changes live outside the vendor tree:

- IB adapter behavior: `src/live/ib_adapter.cpp`
- IB adapter interface: `include/regimeflow/live/ib_adapter.h`
- IB API build and protobuf compatibility gates: `src/CMakeLists.txt`
- IB behavior tests: `tests/unit/test_broker_adapter_capabilities.cpp`

## Vendor Tree Policy

Keep `third_party/ibapi` as close to the upstream C++ client payload as practical, while pruning unused upstream payloads. If a direct vendor patch becomes unavoidable, document it here with:

- affected files
- reason for patch
- upstream issue or release note, if available
- whether the patch should be dropped on the next IBKR update

## Current Integration-Specific Constraints

- Bundled generated protobuf stubs are expected to match protobuf `3.21.12`.
- Source `.proto` files are intentionally not vendored.
- Java/Python clients, samples, and binary JARs are intentionally not vendored.
- Remote plaintext IB hosts are rejected by RegimeFlow adapter policy unless explicitly allowed in RegimeFlow configuration.
