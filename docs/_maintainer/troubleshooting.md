# Troubleshooting

This page lists common errors and how to resolve them.

## Python Import Errors

**Symptom**: `ModuleNotFoundError: No module named 'regimeflow'`.

- Ensure the virtual environment is active.
- Install the package: `pip install -e python`.
- Or set `PYTHONPATH=python:build/lib` if running directly from the repo.

## Missing `_core` Module

**Symptom**: `ImportError: cannot import name '_core'`.

- Make sure bindings were built: `-DBUILD_PYTHON_BINDINGS=ON`.
- Confirm `_core.*.so` exists in `build/lib`.

## Data Source Errors

**Symptom**: `Failed to create data source`.

- Ensure `data.type` is set.
- Verify paths and file patterns.
- Use `reference/configuration.md` and `guide/data-sources.md` to validate config keys.

## Strategy Not Found

**Symptom**: `Failed to create strategy: <name>`.

- Confirm the strategy is registered.
- Check for typos in `strategy.name`.
- Verify plugin search paths and load list.

## Alpaca Live Config Errors

**Symptom**: `Missing Alpaca API key/secret`.

- Set `ALPACA_API_KEY` and `ALPACA_API_SECRET`.
- Ensure `ALPACA_PAPER_BASE_URL` is set or provided in `live.broker_config`.

## CMake Build Fails

- Confirm CMake 3.20+ and a C++20 compiler.
- Disable missing optional dependencies with flags such as `-DENABLE_IBAPI=OFF`.
