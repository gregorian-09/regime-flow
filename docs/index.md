# RegimeFlow Docs

RegimeFlow is a regime-aware quantitative trading stack for Python research, C++ backtesting, live broker execution, and packaging across Linux, Windows, and macOS.

## Choose Your Path

| If you want to... | Read this next |
| --- | --- |
| Run something in 5 minutes | [Quick Install](getting-started/quick-install.md) |
| Build from source | [Installation](getting-started/installation.md) |
| Run a first source-based backtest | [Quickstart (Backtest)](getting-started/quickstart.md) |
| Work from Python | [Python Overview](python/overview.md) |
| Evaluate live trading readiness | [Live Overview](live/overview.md) |
| Consume RegimeFlow from CMake or vcpkg | [Quick Install](getting-started/quick-install.md) |
| Understand architecture and event flow | [Explanation Overview](explanation/overview.md) |

## What Works From Wheels Vs Source

| Surface | Status | Notes |
| --- | --- | --- |
| Python wheels | Recommended | Fastest path for research users on supported platforms. |
| Source build | Recommended | Full C++ engine, plugins, tests, and optional integrations. |
| vcpkg overlay/custom registry | Supported | Best fit for CMake consumers. |
| Linux `.deb` / `.rpm` artifacts | Release artifact | Good for deployment-oriented installs; still tied to release packaging. |
| Homebrew | Experimental | Do not assume version parity unless the formula has been updated for the current release. |

The canonical support matrix and prerequisites live in [Installation](getting-started/installation.md).

## First 5 Minutes

1. If you want the quickest working path, use [Quick Install](getting-started/quick-install.md) and install the Python wheel.
2. If you need source builds or plugins, go to [Installation](getting-started/installation.md).
3. If you already have a checkout and want a runnable example, use [Quickstart (Backtest)](getting-started/quickstart.md).

## Live Trading Boundary

RegimeFlow includes live adapters, but live support is not presented as blanket production validation.

Read these before treating a live configuration as deployable:
- [Live Overview](live/overview.md)
- [Brokers](live/brokers.md)
- [Production Readiness](live/production-readiness.md)
- [Resilience](live/resilience.md)

## Documentation Map

- [Getting Started](getting-started/overview.md): installation, quick paths, troubleshooting, and repo layout
- [Quant Guide](guide/backtesting.md): workflows for backtesting, regime detection, risk, execution, and walk-forward analysis
- [Python](python/overview.md): Python package, CLI, strategy patterns, and research workflow
- [Reference](reference/configuration.md): exact config keys, flags, plugin API, and public symbol coverage
- [Explanation](explanation/overview.md): architecture, event flow, execution semantics, and deeper engineering notes
- [Tutorials](tutorials/examples.md): examples and end-to-end workflows
- [Reports](reports/intraday_strategy_tradecheck.md): generated reports and validation artifacts
