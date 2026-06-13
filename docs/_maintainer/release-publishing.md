# Release Publishing

RegimeFlow uses a tag-based release model. A tag named `vX.Y.Z` is the source of truth for public distribution.

## Publishing Strategy

The release pipeline has one owner: `.github/workflows/publish.yml`.

- `ci.yml` validates code, tests, static analysis, and sanitizers.
- `wheels.yml` is a manual preflight for building Python wheels without publishing.
- `publish.yml` runs only on `v*` tag pushes and publishes public artifacts.
- Linux package repository metadata is published under `gh-pages/packages` so it does not overwrite the documentation site.

Do not add a second release job to `ci.yml`; it creates ambiguity about which workflow owns public artifacts.

## Release Gate

Before publishing, `publish.yml` runs `tools/check_versions.py` against the tag version. The workflow fails if any of these disagree with the tag:

- `pyproject.toml`
- `CMakeLists.txt`
- `python/regimeflow/__init__.py`
- `vcpkg.json`
- `ports/regimeflow/vcpkg.json`
- `ports/regimeflow/portfile.cmake`
- `packaging/deb/DEBIAN/control`
- `packaging/rpm/regimeflow.spec`
- `CHANGELOG.md`

Run the same check locally before tagging:

```bash
python3 tools/check_versions.py v1.0.11
```

## PyPI Publishing

PyPI publishing should use Trusted Publishing with GitHub OIDC, not a long-lived API token.

One-time PyPI project configuration:

- Owner: `gregorian-09`
- Repository: `regime-flow`
- Workflow file: `publish.yml`
- Environment: `pypi`
- Package name: `regimeflow`

The GitHub job named `Publish to PyPI` uses the `pypi` environment and has `id-token: write` permission. If Trusted Publishing is not configured on PyPI, the job will fail before upload.

## Wheel Coverage

The publish and manual wheel workflows build CPython wheels for:

- `cp39`
- `cp310`
- `cp311`
- `cp312`
- `cp313`
- `cp314`

The workflow excludes PyPy and musllinux wheels until they are tested explicitly.

## Release Flow

1. Ensure CI is green on the commit you intend to release.
2. Update version metadata and `CHANGELOG.md`.
3. Run `python3 tools/check_versions.py vX.Y.Z` locally.
4. Optionally run the `Wheels` workflow manually as a preflight.
5. Push the tag:

```bash
git tag vX.Y.Z
git push origin vX.Y.Z
```

6. `publish.yml` builds wheels, builds an sdist, validates distributions with `twine check`, uploads artifacts to the GitHub Release, publishes to PyPI, builds `.deb` and `.rpm` packages, and publishes package repository metadata under GitHub Pages.

## Verification

After PyPI publication:

```bash
python -m pip install --upgrade regimeflow
python -c "import regimeflow; print(regimeflow.__version__)"
```

For a clean install test, use a fresh virtual environment or temporary container.

## vcpkg And Homebrew

This repository contains local packaging definitions for vcpkg and Homebrew-style distribution, but RegimeFlow should not be submitted to the Microsoft curated vcpkg registry until it satisfies the registry maturity expectations.

For each release:

- update `ports/regimeflow/portfile.cmake` to the new `REF`
- update `ports/regimeflow/vcpkg.json`
- update `vcpkg.json`
- update package checksums if the package manager requires them
- keep official registry submission separate from the release tag
