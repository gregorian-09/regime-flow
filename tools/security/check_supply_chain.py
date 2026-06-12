#!/usr/bin/env python3
"""Repository supply-chain guardrails for vendored code and workflow actions."""
from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IBAPI_ROOT = ROOT / "third_party" / "ibapi"
DISALLOWED_IBAPI_SUFFIXES = {
    ".jar",
    ".class",
    ".java",
    ".py",
    ".proto",
    ".dll",
    ".exe",
    ".dylib",
    ".so",
}
REQUIRED_IBAPI_FILES = {
    "third_party/ibapi/LICENSE.md",
    "third_party/ibapi/VENDOR.md",
    "third_party/ibapi/PATCHES.md",
    "third_party/ibapi/SHA256SUMS",
    "third_party/ibapi/IBJts/API_VersionNum.txt",
}
PINNED_ACTION_RE = re.compile(r"^[^\s@]+@[0-9a-fA-F]{40}$")
USES_RE = re.compile(r"^\s*uses:\s*([^\s#]+)", re.MULTILINE)
REQUIRED_PLUGIN_EXPORTS = {
    "create_plugin",
    "destroy_plugin",
    "plugin_type",
    "plugin_name",
    "regimeflow_abi_version",
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def check_required_files(errors: list[str]) -> None:
    for entry in sorted(REQUIRED_IBAPI_FILES):
        if not (ROOT / entry).is_file():
            errors.append(f"missing required vendor metadata: {entry}")
    if not (ROOT / "SECURITY.md").is_file():
        errors.append("missing SECURITY.md")
    if not (ROOT / "python" / "regimeflow" / "py.typed").is_file():
        errors.append("missing python/regimeflow/py.typed")


def check_ibapi_scope(errors: list[str]) -> None:
    if not IBAPI_ROOT.exists():
        errors.append("third_party/ibapi is missing")
        return
    for path in IBAPI_ROOT.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() in DISALLOWED_IBAPI_SUFFIXES:
            errors.append(f"disallowed vendored IB API payload: {rel(path)}")


def read_checksum_manifest(errors: list[str]) -> dict[str, str]:
    manifest_path = IBAPI_ROOT / "SHA256SUMS"
    if not manifest_path.is_file():
        return {}
    entries: dict[str, str] = {}
    for line_number, line in enumerate(manifest_path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        try:
            digest, path = line.split(maxsplit=1)
        except ValueError:
            errors.append(f"invalid checksum line {line_number}: {line!r}")
            continue
        path = path.strip()
        if not re.fullmatch(r"[0-9a-fA-F]{64}", digest):
            errors.append(f"invalid checksum digest on line {line_number}: {digest}")
            continue
        entries[path] = digest.lower()
    return entries


def check_ibapi_checksums(errors: list[str]) -> None:
    entries = read_checksum_manifest(errors)
    if not entries:
        return

    actual_files = {
        rel(path)
        for path in IBAPI_ROOT.rglob("*")
        if path.is_file() and rel(path) != "third_party/ibapi/SHA256SUMS"
    }
    expected_files = set(entries)

    for missing in sorted(expected_files - actual_files):
        errors.append(f"checksum manifest references missing file: {missing}")
    for extra in sorted(actual_files - expected_files):
        errors.append(f"vendored file missing from checksum manifest: {extra}")

    for entry, expected in sorted(entries.items()):
        path = ROOT / entry
        if not path.is_file():
            continue
        actual = sha256(path)
        if actual != expected:
            errors.append(f"checksum mismatch for {entry}: expected {expected}, got {actual}")


def check_workflow_actions(errors: list[str]) -> None:
    workflow_dir = ROOT / ".github" / "workflows"
    if not workflow_dir.exists():
        return
    for workflow in sorted(workflow_dir.glob("*.yml")) + sorted(workflow_dir.glob("*.yaml")):
        text = workflow.read_text(encoding="utf-8")
        if "FORCE_JAVASCRIPT_ACTIONS_TO_NODE24" in text:
            errors.append(f"non-standard Node override in {rel(workflow)}")
        for match in USES_RE.finditer(text):
            value = match.group(1).strip().strip('"\'')
            if value.startswith("./") or value.startswith("docker://"):
                continue
            if not PINNED_ACTION_RE.fullmatch(value):
                errors.append(f"unpinned GitHub Action reference in {rel(workflow)}: {value}")



def check_plugin_template(errors: list[str]) -> None:
    source = ROOT / "examples" / "plugins" / "template" / "strategy_template.cpp"
    readme = ROOT / "examples" / "plugins" / "template" / "README.md"
    if not source.is_file():
        errors.append("missing plugin SDK template source")
        return
    if not readme.is_file():
        errors.append("missing plugin SDK template README")
        return
    source_text = source.read_text(encoding="utf-8")
    readme_text = readme.read_text(encoding="utf-8")
    for export in sorted(REQUIRED_PLUGIN_EXPORTS):
        if export not in source_text or export not in readme_text:
            errors.append(f"plugin SDK template missing required export reference: {export}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-workflows", action="store_true", help="skip GitHub Actions pin checks")
    args = parser.parse_args()

    errors: list[str] = []
    check_required_files(errors)
    check_ibapi_scope(errors)
    check_ibapi_checksums(errors)
    check_plugin_template(errors)
    if not args.skip_workflows:
        check_workflow_actions(errors)

    if errors:
        print("Supply-chain checks failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("Supply-chain checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
