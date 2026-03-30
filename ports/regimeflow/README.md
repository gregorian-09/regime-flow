# vcpkg Port (Overlay)

This port is intended for local testing, CI validation, and custom-registry work
before upstreaming to the official vcpkg registry.

## Current Install Path

```bash
vcpkg install regimeflow --overlay-ports=/path/to/regime-flow/ports
```

## Git Registry Path

This repository now also contains the vcpkg versions metadata under `versions/`.
That means you can consume it as a git registry instead of only as an overlay.

Example consumer `vcpkg-configuration.json`:

```json
{
  "default-registry": {
    "kind": "builtin",
    "baseline": "<builtin-baseline>"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/gregorian-09/regime-flow",
      "baseline": "<registry-baseline-commit>",
      "packages": [ "regimeflow" ]
    }
  ]
}
```

Then users can install with:

```bash
vcpkg install regimeflow
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
3. Update `versions/baseline.json`.
4. Update `versions/r-/regimeflow.json` with the new port `git-tree`.
5. Validate the port with the sample consumer in CI on Windows, Linux, and macOS.
6. Keep `ports/regimeflow/vcpkg.json` features aligned with the CMake option surface.

## Notes

- The port disables tests and Python bindings by default.
- The port disables IB, ZeroMQ, Redis, and Kafka integrations for deterministic cross-platform packaging.
- The current public path is overlay-port usage. Publishing to a custom registry or the official vcpkg registry is the next step.
