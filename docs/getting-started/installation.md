# Installation

This page covers a clean, production-style install for local development and research. RegimeFlow builds with CMake and C++20, and optionally produces Python bindings.

## Requirements

- **C++20 compiler**: GCC or Clang.
- **CMake 3.20+**.
- **Python 3.12+** if you want bindings and the Python CLI.
- Optional dependencies depending on features:
  - OpenSSL for TLS.
  - libcurl for HTTP data sources.
  - PostgreSQL client libs for database data sources.
  - ZeroMQ, Redis, Kafka, IB API for live integrations.

## Build (C++ Core)

```bash
cmake -S . -B build
cmake --build build --target all
```

Build with GCC or Clang explicitly:

```bash
cmake -S . -B build-gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build-gcc --target all

cmake -S . -B build-clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang --target all
```

## Build With Python Bindings

Bindings are enabled by default via `BUILD_PYTHON_BINDINGS`.

```bash
cmake -S . -B build -DBUILD_PYTHON_BINDINGS=ON
cmake --build build --target all
```

The compiled module is written to `build/lib` as `_core.*.so` and is used by the Python package in `python/`.

## Optional Build Flags

These are defined in `CMakeLists.txt`:

- `-DBUILD_TESTS=ON` to build tests.
- `-DBUILD_PYTHON_BINDINGS=ON` for Python bindings.
- `-DENABLE_OPENSSL=ON` for TLS and WSS support.
- `-DENABLE_CURL=ON` for HTTP data sources.
- `-DENABLE_POSTGRES=ON` for Postgres data sources.
- `-DENABLE_ZMQ=ON`, `-DENABLE_REDIS=ON`, `-DENABLE_KAFKA=ON` for message queue adapters.
- `-DENABLE_IBAPI=ON` for Interactive Brokers adapter.

## Python Environment

Create a virtual environment and install the Python package:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e python
```

To run the bindings from the repo without installing, add the module path:

```bash
export PYTHONPATH=python:build/lib
```

## Verify

Run the tests for the chosen build directory:

```bash
ctest --test-dir build --output-on-failure
```

## Next Steps

- `getting-started/quickstart.md`
- `python/overview.md`
- `guide/backtesting.md`
