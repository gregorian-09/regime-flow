#!/usr/bin/env python3
"""Generate release artifact checksums and optional HMAC signatures.

This is intentionally dependency-free so release jobs can produce a verification
manifest even before optional external signing tools are installed.
"""

from __future__ import annotations

import argparse
import hashlib
import hmac
import os
from pathlib import Path
import sys


DEFAULT_PATTERNS = ("*.whl", "*.tar.gz", "*.zip", "*.deb", "*.rpm")


def iter_artifacts(root: Path, patterns: tuple[str, ...]) -> list[Path]:
    files: list[Path] = []
    for pattern in patterns:
        files.extend(path for path in root.rglob(pattern) if path.is_file())
    return sorted(set(files), key=lambda p: p.relative_to(root).as_posix())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--pattern",
        action="append",
        dest="patterns",
        help="Artifact glob pattern. May be repeated. Defaults to common package artifacts.",
    )
    parser.add_argument(
        "--signature-output",
        type=Path,
        help="Optional output path for an HMAC-SHA256 signature over the checksum manifest.",
    )
    parser.add_argument(
        "--signing-key-env",
        default="REGIMEFLOW_ARTIFACT_SIGNING_KEY",
        help="Environment variable containing the optional HMAC signing key.",
    )
    args = parser.parse_args()

    artifact_dir = args.artifact_dir.resolve()
    if not artifact_dir.is_dir():
        print(f"artifact directory does not exist: {artifact_dir}", file=sys.stderr)
        return 2

    patterns = tuple(args.patterns) if args.patterns else DEFAULT_PATTERNS
    artifacts = iter_artifacts(artifact_dir, patterns)
    if not artifacts:
        print(f"no artifacts found in {artifact_dir}", file=sys.stderr)
        return 1

    args.output.parent.mkdir(parents=True, exist_ok=True)
    lines = [f"{sha256(path)}  {path.relative_to(artifact_dir).as_posix()}" for path in artifacts]
    manifest = "\n".join(lines) + "\n"
    args.output.write_text(manifest, encoding="utf-8")

    key = os.environ.get(args.signing_key_env)
    if args.signature_output:
        if not key:
            print(f"{args.signing_key_env} is not set; cannot write signature", file=sys.stderr)
            return 3
        signature = hmac.new(key.encode("utf-8"), manifest.encode("utf-8"), hashlib.sha256).hexdigest()
        args.signature_output.parent.mkdir(parents=True, exist_ok=True)
        args.signature_output.write_text(signature + "\n", encoding="utf-8")

    print(f"wrote {len(artifacts)} artifact checksum(s) to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
