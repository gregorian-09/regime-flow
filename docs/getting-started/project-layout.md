# Project Layout

This repo follows a clean split between core engine code, bindings, examples, and documentation.

- `include/` Public C++ headers.
- `src/` C++ implementations.
- `python/` Python bindings and CLI.
- `examples/` End-to-end configs and demo runs.
- `tests/` C++ and Python tests.
- `docs/` Documentation content and diagrams.
- `tools/` Utility scripts.
- `cmake/` CMake config and package templates.

Key entry points in code:

- `src/engine/backtest_engine.cpp` backtest runtime.
- `src/engine/engine_factory.cpp` config-driven engine creation.
- `src/data/data_source_factory.cpp` data source configuration mapping.
- `src/tools/live_main.cpp` live CLI parsing and bootstrapping.
- `python/bindings.cpp` Python API and BacktestConfig mapping.
- `python/regimeflow/cli.py` Python CLI.
