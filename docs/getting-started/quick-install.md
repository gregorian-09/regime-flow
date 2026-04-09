# Quick Install

This page is for the fastest working install paths. If you want prerequisites, feature flags, or source-build detail, go to [Installation](installation.md).

## Pick The Fastest Path

| Audience | Recommended path | Support level |
| --- | --- | --- |
| Python research user | `pip install regimeflow` | Recommended |
| CMake consumer | vcpkg overlay/custom registry | Supported |
| Linux deployment user | release `.deb` / `.rpm` artifacts | Release artifact |
| Homebrew user | tap formula | Experimental |

## Python Wheels

This is the fastest way to run a backtest.

```bash
pip install regimeflow
regimeflow-backtest --help
```

Alternative invocation if you prefer the module form:

```bash
python -m regimeflow.cli --help
```

For package details and workflow examples, see [Python Overview](../python/overview.md) and [Python Workflow](../python/workflow.md).

## vcpkg

Today the vcpkg path is an overlay port or custom registry path.

### Overlay port

```bash
git clone https://github.com/gregorian-09/regime-flow.git
vcpkg install regimeflow --overlay-ports=/path/to/regime-flow/ports
```

### Consumer CMake usage

```cmake
find_package(RegimeFlow CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE RegimeFlow::regimeflow_engine)
```

## Linux Release Artifacts

Linux releases publish `.deb` and `.rpm` artifacts. Treat them as release artifacts, not as a substitute for the source-build docs.

- Releases: `https://github.com/gregorian-09/regime-flow/releases`

### APT (Debian/Ubuntu)

```bash
curl -fsSL https://gregorian-09.github.io/regime-flow/apt/repo-signing-public.asc \
  | sudo gpg --dearmor -o /usr/share/keyrings/regimeflow.gpg

echo "deb [signed-by=/usr/share/keyrings/regimeflow.gpg] https://gregorian-09.github.io/regime-flow/apt/ ./" \
  | sudo tee /etc/apt/sources.list.d/regimeflow.list

sudo apt-get update
sudo apt-get install regimeflow
```

### YUM/DNF (Fedora/RHEL)

```bash
sudo tee /etc/yum.repos.d/regimeflow.repo > /dev/null <<'REPO'
[regimeflow]
name=regimeflow
baseurl=https://gregorian-09.github.io/regime-flow/yum/
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://gregorian-09.github.io/regime-flow/yum/repo-signing-public.asc
REPO

sudo dnf install regimeflow
```

## Homebrew

```bash
brew install regimeflow/regimeflow/regimeflow
```

Treat the Homebrew tap as experimental until you confirm that the formula version matches the current release.

## What To Read Next

- [Quickstart (Backtest)](quickstart.md)
- [Installation](installation.md)
- [Troubleshooting](troubleshooting.md)
