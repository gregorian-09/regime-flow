# Interactive Brokers API Vendor Manifest

## Current Vendor State

- Upstream project: Interactive Brokers Trader Workstation API
- Upstream landing page: https://interactivebrokers.github.io/
- Upstream URL: https://interactivebrokers.github.io/
- Vendored API version: `10.42.01`
- Archive SHA256: unavailable for the initial import; current vendored files are pinned in `third_party/ibapi/SHA256SUMS` and all future updates must record the reviewed archive SHA256.
- License: Interactive Brokers API license, vendored at `third_party/ibapi/LICENSE.md`
- Version marker: `third_party/ibapi/IBJts/API_VersionNum.txt`
- Vendored scope: C++ client sources and generated C++ protobuf stubs required by RegimeFlow's optional IB adapter
- Excluded scope: Java client, Python client, sample applications, binary JARs, source `.proto` files, and other unused payloads
- Required protobuf runtime for bundled stubs: `3.21.12`
- Protobuf compatibility gate: `src/CMakeLists.txt`

## Required Files

Every IB API vendor update must keep these files current:

- `third_party/ibapi/LICENSE.md`
- `third_party/ibapi/VENDOR.md`
- `third_party/ibapi/PATCHES.md`
- `third_party/ibapi/SHA256SUMS`

## Update Rules

IB API updates must be reviewed manually. Do not blindly overwrite the tree.

Required update flow:

1. Download the official IBKR API archive from Interactive Brokers.
2. Record the archive URL and SHA256 in the update commit or PR description.
3. Run `tools/vendor/update_ibapi.sh --archive PATH --expected-sha256 SHA256 --apply`.
4. Confirm the vendored scope still excludes unused clients, samples, binaries, and source `.proto` files.
5. Confirm generated protobuf stubs still match the pinned protobuf runtime in `src/CMakeLists.txt`.
6. Run native builds and `BrokerAdapterCapabilities.*` tests.
7. Update this manifest, `PATCHES.md`, and `SHA256SUMS` in the same commit.

## Security Notes

The IB TWS API is a broker protocol dependency, not just a build dependency. Updates can change order fields, status names, Decimal/BID math linkage, protobuf generated files, and socket protocol behavior. Treat every update as behavior-affecting until tests prove otherwise.
