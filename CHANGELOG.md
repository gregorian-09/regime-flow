# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-02-19
### Added
- Windows support for dynamic plugin loading, plus docs updates for plugin platform support.

### Changed
- Skip Windows Debug tests in CI to keep pipelines fast; Release remains the gating test run.
- Trigger CI on version tags (`v*`) so tag pushes can create releases.
- Build Python wheels from the `python/` package directory in the release job.
