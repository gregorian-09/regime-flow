# RegimeFlow

RegimeFlow is a quantitative trading platform that combines regime detection, backtesting, and live execution across brokers.

## Start Here

If you are new to the codebase:
- Read the docs overview: `docs/explanation/overview.md`
- Follow the examples: `docs/tutorials/examples.md`
- Review the API reference: `docs/api/index.md`

## Repository Layout

- `docs/` Documentation (tutorials, how-to guides, reference, explanation).
- `examples/` End-to-end examples and configs.
- `include/` Public C++ headers.
- `src/` C++ implementation.
- `python/` Python bindings and utilities.
- `tests/` C++ and Python tests.
- `tools/` Utility scripts.

## Build

```bash
cmake -S . -B build
cmake --build build --target all
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

## Docs Site

The documentation site is built with MkDocs:

```bash
mkdocs build
```

## Entry Points

- C++ backtest: `src/engine/backtest_engine.cpp`
- Strategy registration: `src/strategy/strategies/register_builtin.cpp`
- Live CLI: `src/tools/live_main.cpp`
- Python bindings: `python/bindings.cpp`
