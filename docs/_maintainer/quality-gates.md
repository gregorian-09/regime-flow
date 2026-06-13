# Quality Gates

This is the maintainer checklist for native-code safety and release confidence.

## Required CI Gates

- `supply-chain`: license, secret, vendored checksum, pinned-action, vulnerability-wrapper, and SBOM checks.
- Linux/macOS/Windows C++ build jobs: compile, test, and `cppcheck` static analysis.
- Linux analysis job: clang-tidy analyzer build.
- Linux sanitizer job: ASAN/UBSAN test run.
- Linux targeted Valgrind: leak/ownership checks for memory-sensitive tests.
- Python tests: root-level editable install and Python test suite.
- Publish workflow: release metadata validation, wheel/sdist build, `twine check`, PyPI publishing, Linux package generation.

## Local Triage Order

1. Reproduce with the normal Debug or Release C++ build.
2. Run the focused unit test that owns the code path.
3. Run `cppcheck` on the compile database.
4. Run clang-tidy if the failure involves ownership, nullability, or lifetime.
5. Run ASAN/UBSAN for undefined behavior or out-of-bounds access.
6. Run Valgrind on Linux for leak or allocator ownership questions.

## Current Scope Decisions

- Valgrind is Linux-only.
- Windows Debug tests are still intentionally not the primary full test gate because they are slow on GitHub-hosted runners.
- Linux package repository publishing must use `gh-pages/packages` with `keep_files: true`; do not publish package metadata at the root of `gh-pages` because that can overwrite the documentation site.
- PyPI publishing should use Trusted Publishing/OIDC, not long-lived API tokens.
