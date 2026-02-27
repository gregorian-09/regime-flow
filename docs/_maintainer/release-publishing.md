# Release Publishing

This project is already configured for trusted publishing to PyPI in `.github/workflows/publish.yml`.

## What This Means

- No API token is required.
- GitHub Actions will obtain a short-lived publishing token from PyPI at release time.

## One-Time PyPI Setup

On PyPI, create a **trusted publisher** for the project:

- **Repository**: `regimeflow/regimeflow` (update if different)
- **Workflow**: `Publish`
- **Environment**: `pypi`
- **Package name**: `regimeflow`

Once the trusted publisher is configured, publishing happens on GitHub releases.

## Release Flow

1. Tag and create a GitHub Release (or use `workflow_dispatch`).
2. The `Publish` workflow builds wheels and sdist.
3. Artifacts are pushed to PyPI.

## Verify

After release:

```bash
pip install regimeflow
python -c "import regimeflow; print(regimeflow.__version__)"
```

## Notes

- The workflow currently builds `cp39` through `cp312` wheels.
- If the repo or package name changes, update both `pyproject.toml` and the PyPI trusted publisher settings.

## Packaging (Homebrew + vcpkg)

Homebrew and vcpkg definitions live in this repo and are updated per release:

- `Formula/regimeflow.rb` uses the GitHub release tarball URL and SHA256.
- `ports/regimeflow/portfile.cmake` uses the GitHub release tarball URL and SHA512.

### Update Steps

1. Download the new release tarball.
2. Compute SHA256 and SHA512.
3. Update:
   - `Formula/regimeflow.rb` (url + sha256)
   - `ports/regimeflow/portfile.cmake` (REF + SHA512)
4. Commit and tag with the release.
