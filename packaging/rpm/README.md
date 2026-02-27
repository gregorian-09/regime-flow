# RPM Package Scaffold

This folder contains a minimal RPM spec for building `.rpm` artifacts.

## Build (example)

```bash
rpmbuild -ba packaging/rpm/regimeflow.spec
```

## Notes

- Update version and dependencies per release.
- For production, integrate in CI and publish to a repo.
