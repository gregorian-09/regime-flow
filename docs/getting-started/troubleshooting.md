# Troubleshooting

## Python Wheel Vs Source Checkout Confusion

If `pip install regimeflow` works but your source checkout does not, you are likely mixing a published wheel with a local editable install.

Use one path at a time:

- wheel install: `pip install regimeflow`
- source checkout: `pip install -e python` plus `PYTHONPATH` / PowerShell equivalent when needed

## Missing Optional Dependencies

If CMake cannot find OpenSSL, PostgreSQL, ZeroMQ, Redis, Kafka, or IB API support, disable the feature explicitly:

```bash
cmake -S . -B build \
  -DENABLE_IBAPI=OFF \
  -DENABLE_ZMQ=OFF \
  -DENABLE_REDIS=OFF \
  -DENABLE_KAFKA=OFF \
  -DENABLE_POSTGRES=OFF \
  -DENABLE_OPENSSL=OFF
```

Or enable `-DREGIMEFLOW_FETCH_DEPS=ON` where supported.

## vcpkg Toolchain Not Applied

If dependencies are not resolving through vcpkg, check that you passed the toolchain file:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

On Windows PowerShell:

```powershell
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

## Windows Environment Differences

Common Unix commands in the docs map to PowerShell as follows:

| Unix-like shell | PowerShell |
| --- | --- |
| `. .venv/bin/activate` | `.\.venv\Scripts\Activate.ps1` |
| `export PYTHONPATH=python:build/lib` | `$env:PYTHONPATH = "python;build\lib"` |
| `./build/bin/regimeflow_live` | `build\bin\regimeflow_live.exe` |

## Plugin Loading Fails

If a custom plugin is not discovered:

- confirm the plugin binary was built for the same platform and configuration as RegimeFlow
- verify `plugins.search_paths` and `plugins.load` entries in your config
- test with one of the examples in [Plugin API](../reference/plugin-api.md) or [Plugins](../reference/plugins.md)

## Tests Fail After A Successful Build

- C++ tests: run `ctest --test-dir build --output-on-failure`
- Python tests: run `pytest python/tests`
- If you changed feature flags, rebuild from a clean build directory to avoid stale config

## Next Steps

- [Installation](installation.md)
- [Quick Install](quick-install.md)
- [Quickstart (Backtest)](quickstart.md)
- [Environment And Flags](../reference/environment-and-flags.md)
