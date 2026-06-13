# Developer Quality Gates

RegimeFlow uses several native-code gates to keep the C++ engine safe and portable. These checks are part of CI and should also be run locally before changing concurrency, memory management, mmap storage, or live-trading code.

## Static Analysis

CI runs `cppcheck` on Linux, macOS, and Windows C++ build jobs using the generated CMake compile database.

Local command after configuring with `CMAKE_EXPORT_COMPILE_COMMANDS=ON`:

```bash
cppcheck --project=build/compile_commands.json \
  --std=c++20 \
  --enable=warning,portability \
  --inline-suppr \
  --suppress=missingIncludeSystem \
  --suppress=unmatchedSuppression \
  --suppress=invalidPointerCast \
  --suppress=preprocessorErrorDirective \
  --error-exitcode=2 \
  --file-filter='include/*' \
  --file-filter='src/*'
```

The mmap code intentionally uses layout casts around validated file headers, so `invalidPointerCast` is suppressed in CI. Do not use that suppression as permission to add unchecked casts elsewhere.

## clang-tidy

Linux CI also has a clang-tidy build for analyzer-style checks.

```bash
cmake -S . -B build-tidy \
  -G Ninja \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DENABLE_CLANG_TIDY=ON
cmake --build build-tidy
```

The project keeps clang-tidy target-scoped through CMake instead of applying global compiler flags to third-party dependencies.

## Sanitizers

The sanitizer job uses Clang with AddressSanitizer and UndefinedBehaviorSanitizer.

```bash
cmake -S . -B build-asan \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_PYTHON_BINDINGS=OFF \
  -DREGIMEFLOW_FETCH_DEPS=ON \
  -DENABLE_WERROR=ON \
  -DENABLE_SANITIZERS=ON
cmake --build build-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-asan --output-on-failure
```

Sanitizer flags are applied through project targets, not global `add_compile_options`, so vendored and fetched third-party code is not forced into the same instrumentation policy.

## Valgrind

Valgrind is used only on Linux. Windows and macOS CI use static analysis and sanitizer-style coverage where supported, but they do not run Valgrind.

CI runs a targeted Valgrind pass against memory-sensitive tests:

```bash
valgrind --leak-check=full \
  --show-leak-kinds=definite,indirect \
  --errors-for-leak-kinds=definite,indirect \
  --error-exitcode=9 \
  ./build/bin/regimeflow_tests \
  --gtest_filter='MonotonicArena.*:MmapWriter.*:EventBus.*:ReplayJournal.*:LiveOrderManager.*:PluginRegistry.*'
```

Use the full test suite under sanitizers first. Use Valgrind when investigating allocator ownership, mmap writer/reader behavior, plugin loading, replay journaling, or event dispatch lifetimes.

## Version And Publishing Gate

Release publication is blocked if tag metadata drifts. Before pushing a release tag, run:

```bash
python3 tools/check_versions.py vX.Y.Z
```

The check validates Python, CMake, vcpkg, Debian, RPM, portfile, and changelog versions against the tag.
