# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Fixed
- Removed the unnecessary `regimeflow_plugins -> regimeflow_common` binary link so Windows shared vcpkg consumer builds no longer fail waiting for `regimeflow_common.lib`.
- Switched the Windows vcpkg consumer smoke test to `x64-windows-static` so CI validates package consumption without the unstable DLL/import-library packaging path.
- Switched the Windows vcpkg consumer smoke test to `x64-windows-static-md` and matched the consumer CRT setting so MSVC no longer rejects the packaged static libraries with runtime-library mismatches.
- Fixed Windows secret hygiene tests by closing audit log files before deletion and by generating helper scripts plus command invocation that work correctly with Windows batch files and quoted paths.
- Updated the Publish workflow to pin `softprops/action-gh-release` to a valid upstream release commit so Linux package publication can start correctly.
- Reduced wheel and Linux package publish builds to the core packaging feature set so Publish no longer fails on optional IBAPI, ZeroMQ, Redis, Kafka, PostgreSQL, curl, or OpenSSL dependencies that are not provisioned in the release runners.
- Made the publish package version resolution robust for manual workflow_dispatch runs by falling back to the project version when the workflow is not running on a `v*` tag.
- Fixed the manual publish version fallback to parse `CMakeLists.txt` reliably, so Linux package metadata no longer ends up with an empty version field on workflow_dispatch runs.
- Corrected the `publish.yml` version-resolution script indentation so GitHub Actions can parse and register the workflow again after the manual-dispatch fallback change.
- Replaced the publish workflow's Bash heredoc version parser with a single-line Python command because Bash here-document terminators cannot be space-indented inside the YAML `run` block.
- Changed the Publish workflow to run only on `v*` tag pushes, so release packaging and publication always derive their version from the release tag instead of a branch or manual-dispatch fallback.
- Taught the Ubuntu-based RPM packaging job to use `rpmbuild --nodeps`, because its build toolchain is provisioned by `apt` and therefore not visible to the RPM database used for `BuildRequires` checks.
- Added an explicit `git archive` source tarball for the RPM publish job so `rpmbuild` has the `Source0` payload that `%autosetup` expects in `rpmbuild/SOURCES`.
- Replaced Fedora-specific `%cmake` RPM macros with plain `cmake` commands in the spec so the Ubuntu-based publish job can build the RPM package without distro macro packages.
- Fixed the RPM `%files` manifest to match the actual install layout on Ubuntu runners, packaging static libraries from `%{_libdir}` instead of nonexistent `/usr/bin` and `/usr/lib64` paths.
- Opted all GitHub Actions workflows into Node 24 for JavaScript-based actions so runner deprecation warnings about Node 20 no longer appear.
- Simplified the vcpkg consumer smoke app to link the exported engine target only, avoiding a missing `RegimeFlow::regimeflow_strategy` package target on Windows.
- Removed the unused POSIX-only `poll.h` include from the Redis queue adapter so Windows MSVC builds compile cleanly.
- Simplified the vcpkg consumer smoke executable to exercise the exported common target only, avoiding missing transitive engine/strategy symbols in consumer builds.
- Defined `NOMINMAX` before `windows.h` in the strategy tester tool so MSVC no longer breaks on `std::min` and `std::max`.
- Loaded the packaged Python extension from its actual wheel/build location instead of assuming a top-level `_core` module.
- Renamed the pure-Python package bootstrap module so the compiled `_core` extension keeps the module name expected by Python's extension loader.
- Matched the dynamic plugin test's `destroy_plugin` ABI to the registry callback type so sanitizer builds no longer trip on an invalid function-pointer call.
- Let vcpkg consumer CI build the overlay port from the current workspace instead of the last published tag.
- Raised the Intel macOS wheel deployment target to 15.0 to match the hosted runner's OpenSSL binaries during delocate repair.
- Avoided re-importing the native Python extension under a second module name in the native binding tests.
- Exported Boost websocket requirements from `regimeflow_data` via Boost imported targets so consumer installs no longer leak build-tree include paths.
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
- Stopped Python native binding tests from assuming a CMake build-tree `_core` artifact exists when the package is installed editable in CI.
- Removed the Windows wheel vcpkg toolchain override so cibuildwheel can resolve Python development components correctly.
- Broke the shared-library link cycle between `regimeflow_execution` and `regimeflow_engine` and forced `regimeflow_plugins` to build after `regimeflow_common` on Windows shared builds.
- Enabled CMake's Windows auto-export path for shared builds so vcpkg Windows consumer builds consistently generate the import libraries their dependent DLLs link against.
- Applied Windows auto-export explicitly per library target so vcpkg shared consumers do not rely on a global CMake variable being propagated into subdirectory targets.

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
