# RegimeFlow

[![CI](https://github.com/gregorian-09/regime-flow/actions/workflows/ci.yml/badge.svg)](https://github.com/gregorian-09/regime-flow/actions/workflows/ci.yml)
[![Docs](https://github.com/gregorian-09/regime-flow/actions/workflows/docs.yml/badge.svg)](https://github.com/gregorian-09/regime-flow/actions/workflows/docs.yml)
[![Release](https://img.shields.io/github/v/release/gregorian-09/regime-flow)](https://github.com/gregorian-09/regime-flow/releases)
[![License](https://img.shields.io/github/license/gregorian-09/regime-flow)](LICENSE)
[![Python](https://img.shields.io/badge/python-3.9%2B-blue)](python/pyproject.toml)

RegimeFlow is a regime-aware quantitative trading stack for backtesting, live execution, and research automation. It combines a C++20 engine, Python bindings, regime detection workflows, live broker adapters, risk controls, and a dynamic plugin system across Linux, Windows, and macOS.

Documentation: https://gregorian-09.github.io/regime-flow/

## Choose Your Path

| If you want to... | Start here |
| --- | --- |
| Install the Python package and run a backtest quickly | [`docs/getting-started/quick-install.md`](docs/getting-started/quick-install.md) |
| Build RegimeFlow from source | [`docs/getting-started/installation.md`](docs/getting-started/installation.md) |
| Run the first source-based backtest | [`docs/getting-started/quickstart.md`](docs/getting-started/quickstart.md) |
| Use the Python API and CLI | [`python/README.md`](python/README.md) and [`docs/python/overview.md`](docs/python/overview.md) |
| Inspect live-trading boundaries and broker support | [`docs/live/overview.md`](docs/live/overview.md) and [`docs/live/brokers.md`](docs/live/brokers.md) |
| Consume RegimeFlow from CMake or vcpkg | [`docs/getting-started/quick-install.md`](docs/getting-started/quick-install.md) |
| Contribute code or plugins | [`CONTRIBUTING.md`](CONTRIBUTING.md) |

## Features

- HMM-based and plugin-driven regime detection, including transformer-oriented plugin examples.
- Event-driven backtesting with walk-forward optimization, slippage, transaction-cost modeling, and risk overlays.
- Live adapters for Alpaca, Binance, and Interactive Brokers when built with `ENABLE_IBAPI=ON`.
- Dynamic plugins for strategies, regime detectors, risk managers, and execution models across `.so`, `.dylib`, and `.dll` targets.
- Python bindings and a Python CLI for research workflows, reporting, and dashboard generation.
- Packaging for wheels, Linux `.deb`/`.rpm` artifacts, and vcpkg overlay/custom-registry consumption.

## Platform Support

| Platform | Compiler | Status |
| --- | --- | --- |
| Linux x86_64 | GCC 10+, Clang | CI |
| Windows x64 | MSVC 2022 | CI |
| macOS arm64 | Apple Clang | CI |
| macOS x86_64 | Apple Clang | CI |

The canonical install and support matrix lives in [`docs/getting-started/installation.md`](docs/getting-started/installation.md).

## Installation

### Python wheels

```bash
pip install regimeflow
regimeflow-backtest --help
```

### Source build

```bash
cmake -S . -B build
cmake --build build --target all
ctest --test-dir build --output-on-failure
```

### CMake or vcpkg consumer

```cmake
find_package(RegimeFlow CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE RegimeFlow::regimeflow_engine)
```

Detailed install paths, support tiers, and feature flags are in [`docs/getting-started/installation.md`](docs/getting-started/installation.md) and [`docs/getting-started/quick-install.md`](docs/getting-started/quick-install.md).

## Quick Example

```python
import regimeflow as rf

cfg = rf.BacktestConfig.from_yaml("examples/backtest_basic/config.yaml")
engine = rf.BacktestEngine(cfg)
results = engine.run("moving_average_cross")

print(rf.analysis.performance_summary(results))
rf.analysis.write_report_html(results, "report.html")
```

## Running Tests

```bash
ctest --test-dir build --output-on-failure
pytest python/tests
```

## Repository Layout

- [`docs/`](docs/) user guides, tutorials, architecture notes, API reference, and reports
- [`examples/`](examples/) end-to-end configs, Python workflows, and plugin examples
- [`include/`](include/) public C++ headers
- [`src/`](src/) C++ libraries and CLI tools
- [`python/`](python/) Python package, bindings, and Python-specific docs
- [`tests/`](tests/) C++ and Python tests
- [`packaging/`](packaging/) Debian, RPM, and vcpkg consumer assets
- [`ports/`](ports/) vcpkg overlay/custom registry port definition

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).

## License

Released under the MIT License. See [`LICENSE`](LICENSE).
