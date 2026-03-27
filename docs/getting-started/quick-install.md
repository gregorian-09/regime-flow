# Quick Install

If you want to run RegimeFlow quickly without a full developer setup, use these install paths.

## Python (Fastest)

```bash
pip install regimeflow
python -c "import regimeflow; print(regimeflow.__version__)"
```

This installs prebuilt wheels for supported platforms. It is the fastest path to a working backtest.

## CLI Entry Point

After install, the CLI is available as:

```bash
regimeflow-backtest --config path/to/config.yaml --strategy moving_average_cross
```

## Prebuilt Binaries (Optional)

If you prefer native binaries, use GitHub Releases. Each release includes platform‑specific artifacts and checksums.

- Download: `https://github.com/gregorian-09/regime-flow/releases`
- Linux packages: `.deb` and `.rpm` artifacts are attached to each release.

## Homebrew (macOS)

```bash
brew install regimeflow/regimeflow/regimeflow
```

## vcpkg (Windows)

```powershell
vcpkg install regimeflow
```

## System Packages

- **Linux**: `.deb` and `.rpm` artifacts are provided via GitHub Releases.

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

## Next Steps

- `getting-started/quickstart.md`
- `python/overview.md`
