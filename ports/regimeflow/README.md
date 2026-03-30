# vcpkg Port (Overlay)

This port is intended for local testing, CI validation, and custom-registry work
before upstreaming to the official vcpkg registry.

## Current Install Path

```bash
vcpkg install regimeflow --overlay-ports=/path/to/regime-flow/ports
```

## Consumer Integration

```cmake
find_package(RegimeFlow CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE
    RegimeFlow::regimeflow_engine
    RegimeFlow::regimeflow_strategy)
```

## Features

Default features:

- `curl`
- `openssl`

Optional features:

- `postgres`

## Release Maintenance

1. Update `REF` to the release tag.
2. Update the `SHA512` source hash.
3. Validate the port with the sample consumer in CI on Windows, Linux, and macOS.
4. Keep `ports/regimeflow/vcpkg.json` features aligned with the CMake option surface.

## Notes

- The port disables tests and Python bindings by default.
- The port disables IB, ZeroMQ, Redis, and Kafka integrations for deterministic cross-platform packaging.
- The current public path is overlay-port usage. Publishing to a custom registry or the official vcpkg registry is the next step.
