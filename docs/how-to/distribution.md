# Distribution And Deployment

This guide explains how to **ship RegimeFlow across Linux, macOS, and Windows** without asking users to
compile C++ or set up large local toolchains.

## Recommended Strategy (Cross‑Platform)

1. **Publish Python wheels to PyPI** (primary install path)
2. **Publish a conda‑forge package** (scientific stack users)
3. **Attach prebuilt CLI binaries to GitHub Releases** (C++ users who want a standalone CLI)
4. Keep a **source build** path as a fallback for unusual environments

## 1) PyPI Wheels (Primary)

Wheels are the fastest path for most users:

```bash
pip install regimeflow
```

We build wheels using GitHub Actions + `cibuildwheel`, which produces `manylinux` (Linux), macOS, and
Windows wheels with bundled native libraries.

### CI workflow (already in this repo)

- `.github/workflows/wheels.yml` builds wheels for Linux/macOS/Windows.
- Wheels are uploaded as CI artifacts and can be attached to releases.

## 2) conda‑forge (Quant / Research users)

Conda‑forge is a standard distribution channel for compiled scientific packages. A feedstock
contains the recipe and CI configuration. Once the recipe is merged into `staged-recipes`, a feedstock
is created and CI builds the packages for all platforms.

Recommended for users already on conda/mamba:

```bash
conda install -c conda-forge regimeflow
```

## 3) GitHub Releases (C++ CLI)

For users who want the native CLI without Python, publish prebuilt binaries in GitHub Releases
(e.g., `regimeflow_live`, `regimeflow_alpaca_fetch`). This avoids local build steps.

## 4) Source Builds (Fallback)

Keep a documented fallback build for users on unsupported platforms.

```bash
pip install -e python
```

## Suggested Release Flow

1. Update version in `python/pyproject.toml`.
2. Tag a release: `git tag vX.Y.Z && git push origin vX.Y.Z`.
3. CI builds wheels and publishes artifacts.
4. Publish to PyPI and conda‑forge.

