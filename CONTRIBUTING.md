# Contributing

## Scope

RegimeFlow is a mixed C++ and Python repository with optional live-trading integrations and packaging targets. Keep changes small, testable, and explicit about feature flags.

## Prerequisites

- CMake `3.20+`
- C++20 compiler
- Python `3.9+`
- `vcpkg` if you want a consistent native dependency path across platforms

## Build

Core build:

```bash
cmake -S . -B build
cmake --build build --target all
```

Recommended dependency-managed build:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --target all
```

Lean build when optional integrations are not available:

```bash
cmake -S . -B build \
  -DREGIMEFLOW_FETCH_DEPS=ON \
  -DENABLE_IBAPI=OFF \
  -DENABLE_ZMQ=OFF \
  -DENABLE_REDIS=OFF \
  -DENABLE_KAFKA=OFF \
  -DENABLE_POSTGRES=OFF
cmake --build build --target all
```

## Tests

C++ test suite:

```bash
ctest --test-dir build --output-on-failure
```

Python tests:

```bash
pytest python/tests
```

Sanitizer build:

```bash
cmake -S . -B build-clang-debug \
  -DENABLE_SANITIZERS=ON \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang-debug --target all
ctest --test-dir build-clang-debug --output-on-failure
```

clang-tidy build:

```bash
cmake -S . -B build-tidy -DENABLE_CLANG_TIDY=ON
cmake --build build-tidy --target all
```

## Plugin Development

Start from the examples in:

- `examples/plugins/custom_regime/`
- `examples/plugins/transformer_regime/`
- `docs/reference/plugin-api.md`
- `docs/reference/plugins.md`

Keep plugin ABI boundaries stable. On Windows that means testing dynamic loading, not just static registration.

## Pull Requests

- Describe the user-visible effect and the feature flags involved.
- Include the exact build and test commands you ran.
- Update `README.md`, `python/README.md`, or docs pages when the public surface changes.
- Update `CHANGELOG.md` only for user-visible behavior, packaging, or release-facing changes.
- Do not bury public API changes inside CI-only commits.
