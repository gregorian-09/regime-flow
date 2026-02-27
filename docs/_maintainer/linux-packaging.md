# Linux Packaging (Deb/RPM)

This project includes scaffolding to produce `.deb` and `.rpm` artifacts.

## Debian (deb)

Files:

- `packaging/deb/DEBIAN/control`
- `packaging/deb/README.md`

Typical flow:

1. Build and install into a staging prefix.
2. Copy the files into a package root with `DEBIAN/control`.
3. Build with `dpkg-deb --build`.

## RPM

Files:

- `packaging/rpm/regimeflow.spec`
- `packaging/rpm/README.md`

Typical flow:

- Use `rpmbuild -ba` to build the RPM.

## CI Integration

The release workflow publishes signed apt/yum repositories to GitHub Pages under:

- `https://gregorian-09.github.io/regime-flow/apt/`
- `https://gregorian-09.github.io/regime-flow/yum/`

The workflow builds `.deb`/`.rpm`, generates repo metadata, signs it, and pushes to the `gh-pages` branch.

## Secrets Required

- `GPG_PRIVATE_KEY`
- `GPG_PASSPHRASE`
- `GPG_PUBLIC_KEY`
