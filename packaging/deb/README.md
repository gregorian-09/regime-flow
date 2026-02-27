# Debian Package Scaffold

This folder contains a minimal Debian package control file. It is a scaffold for building `.deb` artifacts.

## Build (example)

1. Build and install RegimeFlow into a staging directory:

```bash
cmake -S . -B build
cmake --build build
cmake --install build --prefix /tmp/regimeflow
```

2. Create a package root:

```bash
mkdir -p /tmp/regimeflow-deb
cp -r /tmp/regimeflow/* /tmp/regimeflow-deb/
mkdir -p /tmp/regimeflow-deb/DEBIAN
cp packaging/deb/DEBIAN/control /tmp/regimeflow-deb/DEBIAN/control
```

3. Build the package:

```bash
dpkg-deb --build /tmp/regimeflow-deb
```

## Notes

- Update version and dependencies per release.
- For production, integrate this into CI and publish to a repo.
