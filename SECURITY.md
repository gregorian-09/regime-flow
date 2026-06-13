# Security Policy

## Supported Versions

Security fixes are handled for the latest tagged release and the current `main` branch.
Older releases may receive fixes when the affected code path is still supported and the
patch can be applied safely.

## Reporting a Vulnerability

Please do not open a public issue for suspected vulnerabilities.

Report security issues through GitHub Security Advisories when available for this
repository. If advisories are unavailable, email the maintainer at
`gregorianrayne09@gmail.com` with:

- affected version or commit
- operating system and compiler or Python version
- reproduction steps or proof-of-concept details
- expected impact

The project aims to acknowledge reports within 7 days and provide a remediation plan
within 30 days for confirmed vulnerabilities.

## Scope

In scope:

- C++ core libraries and live-trading components
- Python bindings and package metadata
- CI/release automation that can affect distributed artifacts
- vendored or linked dependency integration issues

Out of scope:

- vulnerabilities in third-party services, brokers, exchanges, or market-data providers
- misconfigured user infrastructure
- issues requiring physical access to a user's machine


## Supply-Chain Artifacts

RegimeFlow includes a dependency-free SPDX 2.3 SBOM generator:

```bash
python3 tools/security/generate_sbom.py --output build/sbom/regimeflow.spdx.json
```

The generated document records the project version, Python dependencies, vcpkg dependencies,
and vendored Interactive Brokers API files with SHA256 checksums. CI runs this generator in
the supply-chain gate so release artifacts can be traced back to a deterministic dependency
inventory.

Release/package jobs can generate a checksum manifest for built artifacts:

```bash
python3 tools/security/generate_artifact_manifest.py \
  --artifact-dir dist \
  --output dist/SHA256SUMS
```

If `REGIMEFLOW_ARTIFACT_SIGNING_KEY` is available, the same tool can also write an
HMAC-SHA256 signature over the manifest:

```bash
python3 tools/security/generate_artifact_manifest.py \
  --artifact-dir dist \
  --output dist/SHA256SUMS \
  --signature-output dist/SHA256SUMS.hmac
```

Known-vulnerability scanning is wrapped by:

```bash
python3 tools/security/run_vulnerability_scan.py --allow-missing-tools
python3 tools/security/run_vulnerability_scan.py --require-tools
```

Use `--require-tools` in hardened release environments after installing scanners such as
`pip-audit` and `osv-scanner`; use `--allow-missing-tools` for local maintainer smoke checks.
