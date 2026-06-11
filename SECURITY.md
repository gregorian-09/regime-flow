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
