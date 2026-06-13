#!/usr/bin/env python3
"""Lightweight repository secret scanner for CI and maintainer preflight checks."""
from __future__ import annotations

import math
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
EXCLUDED_DIRS = {
    ".git",
    ".venv",
    "build",
    "build-gcc",
    "build-clang",
    "dist",
    "wheelhouse",
    "vcpkg",
    "vcpkg_installed",
    "__pycache__",
}
EXCLUDED_SUFFIXES = {
    ".a",
    ".dll",
    ".dylib",
    ".exe",
    ".lib",
    ".o",
    ".obj",
    ".png",
    ".jpg",
    ".jpeg",
    ".gif",
    ".pdf",
    ".so",
    ".zip",
    ".gz",
}
ALLOWLIST_FRAGMENTS = {
    "example",
    "placeholder",
    "dummy",
    "redacted",
    "changeme",
    "your_",
    "test",
    "secret_key = \"secret\"",
    "api_key = \"key\"",
    "PYPI_API_TOKEN",
    "GPG_PRIVATE_KEY",
    "REGIMEFLOW_ARTIFACT_SIGNING_KEY",
}
SECRET_PATTERNS = [
    re.compile(r"(?i)(api[_-]?key|secret[_-]?key|access[_-]?token|auth[_-]?token|password)\s*[:=]\s*['\"]([^'\"]{12,})['\"]"),
    re.compile(r"gh[pousr]_[A-Za-z0-9_]{36,}"),
    re.compile(r"AKIA[0-9A-Z]{16}"),
    re.compile(r"-----BEGIN (RSA |EC |OPENSSH |PGP )?PRIVATE KEY-----"),
]


def entropy(value: str) -> float:
    if not value:
        return 0.0
    counts = {char: value.count(char) for char in set(value)}
    return -sum((count / len(value)) * math.log2(count / len(value)) for count in counts.values())


def should_scan(path: Path) -> bool:
    rel_parts = path.relative_to(ROOT).parts
    if any(part in EXCLUDED_DIRS for part in rel_parts):
        return False
    if path.suffix.lower() in EXCLUDED_SUFFIXES:
        return False
    return path.is_file()


def allowed(line: str) -> bool:
    lowered = line.lower()
    return any(fragment.lower() in lowered for fragment in ALLOWLIST_FRAGMENTS)


def main() -> int:
    findings: list[str] = []
    for path in sorted(ROOT.rglob("*")):
        if not should_scan(path):
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for line_number, line in enumerate(text.splitlines(), start=1):
            if allowed(line):
                continue
            for pattern in SECRET_PATTERNS:
                match = pattern.search(line)
                if not match:
                    continue
                candidate = match.group(match.lastindex or 0)
                if len(candidate) >= 20 and entropy(candidate) < 3.0:
                    continue
                findings.append(f"{path.relative_to(ROOT)}:{line_number}: possible secret")
    if findings:
        print("secret scan failed:", file=sys.stderr)
        print("\n".join(findings), file=sys.stderr)
        return 1
    print("Secret scan passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
