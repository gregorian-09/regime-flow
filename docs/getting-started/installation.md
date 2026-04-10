# Installation

This page is the source of truth for prerequisites, supported install paths, optional dependencies, and platform notes.

## Support Matrix

| Surface | Support level | Notes |
| --- | --- | --- |
| Python wheels | Recommended | Python `3.9+`; fastest route for quant research users. |
| Source build | Recommended | C++20 core, plugins, tests, and optional live integrations. |
| vcpkg overlay/custom registry | Supported | Intended for CMake consumers on Windows, Linux, and macOS. |
| Linux `.deb` / `.rpm` artifacts | Release artifact | Built in release CI; useful for deployment-style installs. |
| Homebrew | Experimental | Verify formula freshness against the current release tag. |

## Minimum Requirements

- CMake `3.20+`
- C++20 compiler
- Python `3.9+` for wheels, bindings, and the Python CLI
- `vcpkg` recommended when you want a consistent native dependency path

## Platform Notes

| Platform | Compiler | Notes |
| --- | --- | --- |
| Linux x86_64 | GCC 10+, Clang | Primary source-build and wheel target. |
| Windows x64 | MSVC 2022 | CI-supported; prefer PowerShell examples where relevant. |
| macOS arm64 | Apple Clang | CI-supported. |
| macOS x86_64 | Apple Clang | CI-supported. |

## Optional Native Dependencies

Enable these only if your workflow needs them:

- `ENABLE_OPENSSL=ON` for TLS and secure WebSocket support
- `ENABLE_CURL=ON` for HTTP data sources and clients
- `ENABLE_POSTGRES=ON` for PostgreSQL-backed data sources
- `ENABLE_ZMQ=ON` for ZeroMQ adapters
- `ENABLE_REDIS=ON` for Redis adapters
- `ENABLE_KAFKA=ON` for Kafka adapters
- `ENABLE_IBAPI=ON` for Interactive Brokers support
- `REGIMEFLOW_FETCH_DEPS=ON` to let RegimeFlow fetch lightweight missing dependencies in supported builds

## Recommended Source Build

### Unix-like shells

```bash
git clone https://github.com/gregorian-09/regime-flow.git
cd regime-flow

git clone https://github.com/microsoft/vcpkg.git vcpkg
./vcpkg/bootstrap-vcpkg.sh -disableMetrics

cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --target all
ctest --test-dir build --output-on-failure
```

### Windows PowerShell

```powershell
git clone https://github.com/gregorian-09/regime-flow.git
Set-Location regime-flow

git clone https://github.com/microsoft/vcpkg.git vcpkg
.\vcpkg\bootstrap-vcpkg.bat -disableMetrics

cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release --target ALL_BUILD
ctest --test-dir build -C Release --output-on-failure
```

## Lean Build With Optional Integrations Disabled

Use this when you want the core engine and Python bindings without the live-integration dependency surface.

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

## Python Bindings From Source

### Unix-like shells

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
export PYTHONPATH=python:build/lib
```

### Windows PowerShell

```powershell
py -3 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -e .
$env:PYTHONPATH = "python;build\lib"
```

## Verification

- C++ tests: `ctest --test-dir build --output-on-failure`
- Python tests: `pytest python/tests`
- Python package smoke: `python -c "import regimeflow; print(regimeflow.__version__)"`

## Related Pages

- [Quick Install](quick-install.md)
- [Quickstart (Backtest)](quickstart.md)
- [Troubleshooting](troubleshooting.md)
- [Environment And Flags](../reference/environment-and-flags.md)
