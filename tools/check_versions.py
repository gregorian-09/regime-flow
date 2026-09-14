#!/usr/bin/env python3
"""Validate that release metadata agrees with the release tag version."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require_match(label: str, path: str, pattern: str, expected: str) -> None:
    text = read(path)
    match = re.search(pattern, text, flags=re.MULTILINE)
    if match is None:
        raise AssertionError(f"{label}: pattern not found in {path}: {pattern}")
    actual = match.group(1)
    if actual != expected:
        raise AssertionError(f"{label}: {path} has {actual!r}, expected {expected!r}")


def require_contains(label: str, path: str, needle: str) -> None:
    if needle not in read(path):
        raise AssertionError(f"{label}: {path} does not contain {needle!r}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="Release version, with or without a leading v")
    args = parser.parse_args()

    version = args.version.removeprefix("v")
    if not re.fullmatch(r"\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?", version):
        raise SystemExit(f"Invalid semantic version: {args.version!r}")

    checks = [
        ("Python package", "pyproject.toml", r'^version\s*=\s*"([^"]+)"'),
        ("CMake project", "CMakeLists.txt", r"^project\(RegimeFlow VERSION\s+([^\s)]+)"),
        ("Python runtime", "python/regimeflow/__init__.py", r'^__version__\s*=\s*"([^"]+)"'),
        ("vcpkg manifest", "vcpkg.json", r'^\s*"version-string"\s*:\s*"([^"]+)"'),
        ("overlay port manifest", "ports/regimeflow/vcpkg.json", r'^\s*"version"\s*:\s*"([^"]+)"'),
        ("Debian control", "packaging/deb/DEBIAN/control", r"^Version:\s*(\S+)"),
        ("RPM spec", "packaging/rpm/regimeflow.spec", r"^Version:\s*(\S+)"),
    ]
    errors: list[str] = []
    for label, path, pattern in checks:
        try:
            require_match(label, path, pattern, version)
        except AssertionError as exc:
            errors.append(str(exc))

    portfile = read("ports/regimeflow/portfile.cmake")
    release_tag_ref = re.search(rf"^\s*REF v{re.escape(version)}\s*$", portfile, re.MULTILINE)
    release_source_ref = re.search(
        rf"^\s*# Release source: v{re.escape(version)}\s*$.*?"
        rf"^\s*REF [0-9a-f]{{40}}\s*$.*?"
        rf"^\s*SHA512 [0-9a-f]{{128}}\s*$",
        portfile,
        re.MULTILINE | re.DOTALL,
    )
    if release_tag_ref is None and release_source_ref is None:
        errors.append(
            "vcpkg port source: ports/regimeflow/portfile.cmake does not pin "
            f"v{version} or a labeled immutable source commit"
        )

    if f"## [{version}]" not in read("CHANGELOG.md"):
        errors.append(f"changelog: CHANGELOG.md does not contain '## [{version}]'")

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print(f"Release metadata is consistent for v{version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
