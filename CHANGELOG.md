# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Fixed
- Loaded the packaged Python extension from its actual wheel/build location instead of assuming a top-level `_core` module.
- Matched the dynamic plugin test's `destroy_plugin` ABI to the registry callback type so sanitizer builds no longer trip on an invalid function-pointer call.
- Let vcpkg consumer CI build the overlay port from the current workspace instead of the last published tag.
- Raised the Intel macOS wheel deployment target to 15.0 to match the hosted runner's OpenSSL binaries during delocate repair.
- Made `python/regimeflow/_core.py` importable on Python 3.9 wheel test environments by postponing annotation evaluation.
- Fixed fetched Protobuf linkage for bundled IBAPI builds so macOS arm64 links against the fetched library instead of a stale imported target.
- Suppressed third-party hiredis C99 flexible-array warnings in sanitizer builds without weakening project-wide warning settings.
- Made `Result` error returns explicit across optional feature paths so GCC, Clang, and MSVC builds do not rely on invalid implicit conversions.
- Reworked `ConfigValue` storage to avoid recursive container completeness issues on older libstdc++ toolchains used in Linux wheel builds.
- Added a `source_location` fallback for older Linux wheel toolchains that do not ship the C++20 header yet.
- Set the default macOS deployment target to 10.15 for native and wheel builds so plugin registry filesystem code compiles consistently.
- Fixed the vcpkg package dependency metadata so Boost.Asio/Beast headers are available when WebSocket support is enabled.
- Removed a `constexpr std::vector` use that broke GCC 10 in the manylinux wheel build.
- Align standalone wheel workflows with CI so cibuildwheel disables IBAPI and fetches dependencies on all platforms.
- Remove invalid manifest-mode `vcpkg install protobuf:x64-windows` from wheel workflows.
- Reduce Python wheel builds to the bindings-only feature set and stop pinning disabled `protobuf@21` on macOS CI.
- Added the missing member-template disambiguator in live config flattening so GCC 10 wheel builds accept `ConfigValue::get_if`.
- Require Protobuf 3.21.12-compatible headers for the bundled IB API sources and fetch that version when dependency fetching is enabled.
- Build macOS wheels on native Intel and Apple Silicon runners instead of cross-linking arm64 Homebrew OpenSSL into x86_64 wheels.
- Switched the Intel macOS wheel job to the explicit `macos-15-intel` runner label so GitHub schedules the native x86_64 wheel build reliably.
- Fixed the wheel/publish sdist artifact path so release workflows upload the generated source tarball from `python/dist`.
- Matched the arm64 macOS wheel deployment target to the OpenSSL bottle baseline on `macos-14`, avoiding delocate failures during wheel repair.
- Made `src/live/mq_adapter.cpp` include process-id headers independently of the Redis feature flag so macOS consumer builds compile without `REGIMEFLOW_USE_REDIS`.

## [1.0.1] - 2026-02-20
### Fixed
- Build Python wheels from the `python/` package directory in the release job.
- Resolve Windows stack overflow in live engine integration tests by allocating the engine on the heap.
- Fix Windows dynamic plugin test lookup for `regimeflow_test_plugin.dll`.
- Ensure cibuildwheel uses a compatible CMake policy minimum when fetching dependencies.

## [1.0.0] - 2026-02-19
### Added
- Windows support for dynamic plugin loading, plus docs updates for plugin platform support.

### Changed
- Skip Windows Debug tests in CI to keep pipelines fast; Release remains the gating test run.
- Trigger CI on version tags (`v*`) so tag pushes can create releases.
