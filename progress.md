# Documentation Rewrite Progress

Date: 2026-02-26

## Completed

- Audited code paths for config, data sources, execution, risk, regime, live engine, Python bindings, and CLI.
- Reworked MkDocs information architecture to a Spring-style structure with versioning scaffold (mike).
- Added new quant-focused docs and references.
- Fixed malformed code block in `docs/tutorials/examples.md`.
- Expanded `docs/tutorials/python-usage.md` into a full workflow tutorial.
- Updated example configs to use correct risk structure under `limits`.
- Rewrote and aligned explanation pages to the new style and added diagrams across all explanation pages.
- Expanded reference pages and cleaned legacy links.
- Added maintainer docs and moved them under `docs/_maintainer` with exclusion from public build.
- Switched Mermaid assets to local files for offline docs builds.
- Vendored KaTeX assets locally and re-enabled math rendering.
- Removed obsolete `docs/how-to/*` pages and repaired links in API docs.
- Added quick install guidance and removed legacy `docs/README.md`.
- Added Homebrew formula and vcpkg port scaffolding.
- Filled in release checksums for Homebrew and vcpkg using tag `v1.0.1`.
- Added Debian and RPM packaging scaffolds.
- Added GitHub Actions job to build and upload .deb/.rpm on release.
- Added signed apt/yum repo publication to GitHub Pages and user install instructions.
- Updated signing workflow to read public key from `GPG_PUBLIC_KEY` secret.
- MkDocs build succeeds with `_maintainer` excluded.

## Packaging Added

- `Formula/regimeflow.rb`
- `ports/regimeflow/vcpkg.json`
- `ports/regimeflow/portfile.cmake`
- `ports/regimeflow/usage`
- `ports/regimeflow/README.md`
- `packaging/deb/DEBIAN/control`
- `packaging/deb/README.md`
- `packaging/rpm/regimeflow.spec`
- `packaging/rpm/README.md`
- `.github/workflows/publish.yml` (linux packages + signed apt/yum repos)

## Updated Example Configs (Risk Structure)

- `examples/backtest_basic/config.yaml`
- `examples/backtest_intraday_pairs.yaml`
- `examples/backtest_intraday_harmonic.yaml`
- `examples/python_custom_regime_ensemble/config.yaml`
- `examples/python_custom_regime_ensemble/config_intraday_multi.yaml`
- `examples/python_engine_regime/config.yaml`
- `examples/python_engine_regime/config_custom_plugin.yaml`
- `examples/custom_regime_ensemble/config.yaml`
- `examples/transformer_regime_ensemble/config.yaml`
- `examples/transformer_regime_ensemble/config_torchscript.yaml`
- `examples/live_paper_alpaca/config.yaml`
- `examples/live_paper_ib/config.yaml`

## Remaining (Optional)

- Decide whether to keep legacy reference pages visible in nav.

## In Progress

- Implementing parity checker CLI + Python research session.
- Adding live performance drift tracking with file outputs and optional Postgres sink.
