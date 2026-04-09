# Getting Started

This section is the onboarding hub. Use it to choose the right install and runtime path before you dive into the deeper guides.

## Choose Your Path

| If you want to... | Start here |
| --- | --- |
| Install the Python package and run a backtest quickly | [Quick Install](quick-install.md) |
| Build RegimeFlow from source | [Installation](installation.md) |
| Run the first source-based backtest | [Quickstart (Backtest)](quickstart.md) |
| Use RegimeFlow from Python notebooks or scripts | [Python Overview](../python/overview.md) |
| Inspect live brokers and operational boundaries | [Live Overview](../live/overview.md) |
| Consume RegimeFlow from a separate CMake project | [Quick Install](quick-install.md) |
| Contribute code or plugins | https://github.com/gregorian-09/regime-flow/blob/main/CONTRIBUTING.md |

## Recommended Reading Order

### Python-first research users

1. [Quick Install](quick-install.md)
2. [Python Overview](../python/overview.md)
3. [Python Workflow](../python/workflow.md)
4. [Python Usage](../tutorials/python-usage.md)

### Source-build users

1. [Installation](installation.md)
2. [Quickstart (Backtest)](quickstart.md)
3. [Backtesting](../guide/backtesting.md)
4. [Configuration](../reference/configuration.md)

### Live-trading users

1. [Installation](installation.md)
2. [Live Overview](../live/overview.md)
3. [Brokers](../live/brokers.md)
4. [Production Readiness](../live/production-readiness.md)

### CMake and packaging consumers

1. [Quick Install](quick-install.md)
2. [Installation](installation.md)
3. [Configuration](../reference/configuration.md)

## Support Tiers

| Path | Support level | Notes |
| --- | --- | --- |
| Python wheels | Recommended | Fastest path for backtests and Python research. |
| Source build | Recommended | Full engine, tests, plugins, and optional integrations. |
| vcpkg overlay/custom registry | Supported | Best path for external CMake consumers. |
| Linux `.deb` / `.rpm` artifacts | Release artifact | Suitable for deployment installs; tied to release packaging quality. |
| Homebrew | Experimental | Verify the formula version before relying on it. |

## Before You Go Further

- [Installation](installation.md) is the source of truth for prerequisites, feature flags, and supported platforms.
- [Troubleshooting](troubleshooting.md) covers common build, wheel, and environment problems.
- [Project Layout](project-layout.md) explains the repo shape once you know which path you are on.
