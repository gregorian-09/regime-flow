# vcpkg Port (Local)

This port is intended for local testing or as a starting point before upstreaming to the official vcpkg registry.

## Steps

1. Replace `REPLACE_WITH_SOURCE_SHA512` with the SHA512 of the source tarball.
2. Test with:

```bash
vcpkg install regimeflow --overlay-ports=ports
```

## Notes

- The port disables tests and Python bindings by default.
- Update `v1.0.1` and checksums when releasing.
