#!/usr/bin/env python3
"""Dependency-free license metadata checks for release gating."""
from __future__ import annotations

from pathlib import Path
import re
import sys
import tomllib

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    errors: list[str] = []
    license_files = [ROOT / "LICENSE", ROOT / "LICENSE.md"]
    if not any(path.is_file() for path in license_files):
        errors.append("missing project LICENSE or LICENSE.md")

    pyproject = ROOT / "pyproject.toml"
    if pyproject.is_file():
        data = tomllib.loads(pyproject.read_text(encoding="utf-8"))
        project = data.get("project", {})
        license_value = project.get("license")
        if not license_value:
            errors.append("pyproject.toml missing project.license metadata")
        elif isinstance(license_value, str) and not license_value.strip():
            errors.append("pyproject.toml project.license is empty")
    else:
        errors.append("missing pyproject.toml")

    vcpkg = ROOT / "vcpkg.json"
    if vcpkg.is_file():
        text = vcpkg.read_text(encoding="utf-8")
        if '"license"' not in text:
            errors.append("vcpkg.json missing license field")
    else:
        errors.append("missing vcpkg.json")

    ib_license = ROOT / "third_party" / "ibapi" / "LICENSE.md"
    if not ib_license.is_file():
        errors.append("vendored IB API missing LICENSE.md")

    vendor = ROOT / "third_party" / "ibapi" / "VENDOR.md"
    if vendor.is_file():
        vendor_text = vendor.read_text(encoding="utf-8")
        for label in ("Version", "URL", "SHA256", "License"):
            if not re.search(rf"(?im)^[-* ].*{label}\b", vendor_text):
                errors.append(f"IB VENDOR.md missing {label} metadata")
    else:
        errors.append("vendored IB API missing VENDOR.md")

    if errors:
        print("license checks failed:", file=sys.stderr)
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("License checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
