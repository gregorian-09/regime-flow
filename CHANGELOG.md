# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Expanded the public README with prerequisites, installation paths, platform support, a working example, and links to the published docs site.
- Added `CONTRIBUTING.md` with contributor build, test, and plugin-development guidance.
- Removed `progress.md` from the repository root to keep the public repo surface focused on user and contributor documentation.

## [1.0.9] - 2026-04-10

### Changed
- Aligned the project, Python package, and Linux package metadata with the `1.0.9` release series.
- Standardized release packaging around tag-derived versioning so release artifacts follow the published tag.

### Fixed
- Fixed Windows release wheel builds by correcting Windows system-header include order in the live engine.
- Made Linux package publication tolerate missing or misconfigured signing secrets by falling back to unsigned repository metadata when no signing key is available in CI.

## [1.0.1] - 2026-02-20

### Fixed
- Built Python wheels from the `python/` package directory in the release job.
- Resolved a Windows stack overflow in live engine integration tests by allocating the engine on the heap.
- Fixed Windows dynamic plugin test lookup for `regimeflow_test_plugin.dll`.
- Ensured cibuildwheel uses a compatible CMake policy minimum when fetching dependencies.

## [1.0.0] - 2026-02-19

### Added
- Windows support for dynamic plugin loading, plus docs updates for plugin platform support.

### Changed
- Skipped Windows Debug tests in CI to keep pipelines fast while keeping Release as the gating test run.
- Triggered CI on version tags (`v*`) so tag pushes can create releases.
