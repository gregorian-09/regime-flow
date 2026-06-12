#!/usr/bin/env python3
"""Generate a lightweight SPDX 2.3 SBOM for RegimeFlow without external dependencies."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = ROOT / "build" / "sbom" / "regimeflow.spdx.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def project_version() -> str:
    text = read_text(ROOT / "pyproject.toml")
    match = re.search(r'^version\s*=\s*"([^"]+)"', text, re.MULTILINE)
    if match:
        return match.group(1)
    manifest = ROOT / "vcpkg.json"
    if manifest.is_file():
        return json.loads(manifest.read_text(encoding="utf-8")).get("version-string", "NOASSERTION")
    return "NOASSERTION"


def git_commit() -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True, stderr=subprocess.DEVNULL
        ).strip()
    except Exception:
        return "NOASSERTION"


def spdx_id(name: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9.-]", "-", name)
    safe = re.sub(r"-+", "-", safe).strip("-")
    return f"SPDXRef-{safe or 'package'}"


def parse_python_dependencies() -> list[str]:
    text = read_text(ROOT / "pyproject.toml")
    match = re.search(r'^dependencies\s*=\s*\[(.*?)\]', text, re.MULTILINE | re.DOTALL)
    if not match:
        return []
    deps: list[str] = []
    for dep in re.findall(r'"([^"]+)"', match.group(1)):
        deps.append(dep)
    return deps


def parse_vcpkg_dependencies() -> list[str]:
    manifest = ROOT / "vcpkg.json"
    if not manifest.is_file():
        return []
    data = json.loads(manifest.read_text(encoding="utf-8"))
    deps: list[str] = []
    for dep in data.get("dependencies", []):
        if isinstance(dep, str):
            deps.append(dep)
        elif isinstance(dep, dict) and "name" in dep:
            deps.append(str(dep["name"]))
    return deps


def ibapi_version() -> str:
    version_file = ROOT / "third_party" / "ibapi" / "IBJts" / "API_VersionNum.txt"
    text = read_text(version_file).strip()
    return text or "NOASSERTION"


def ibapi_files() -> list[dict[str, Any]]:
    root = ROOT / "third_party" / "ibapi"
    if not root.exists():
        return []
    files: list[dict[str, Any]] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(ROOT).as_posix()
        files.append(
            {
                "fileName": rel,
                "SPDXID": spdx_id("File-" + rel),
                "checksums": [{"algorithm": "SHA256", "checksumValue": sha256(path)}],
                "licenseConcluded": "NOASSERTION",
                "licenseInfoInFiles": ["NOASSERTION"],
                "copyrightText": "NOASSERTION",
            }
        )
    return files


def package(name: str, version: str = "NOASSERTION", supplier: str = "NOASSERTION") -> dict[str, Any]:
    return {
        "name": name,
        "SPDXID": spdx_id("Package-" + name),
        "versionInfo": version,
        "downloadLocation": "NOASSERTION",
        "filesAnalyzed": False,
        "supplier": supplier,
        "licenseConcluded": "NOASSERTION",
        "licenseDeclared": "NOASSERTION",
        "copyrightText": "NOASSERTION",
    }


def build_sbom() -> dict[str, Any]:
    version = project_version()
    created = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    document_id = "SPDXRef-DOCUMENT"
    root_id = spdx_id("Package-regimeflow")
    packages: list[dict[str, Any]] = [
        {
            **package("regimeflow", version, "Person: Gregorian Rayne"),
            "SPDXID": root_id,
            "homepage": "https://github.com/gregorian-09/regime-flow",
            "externalRefs": [
                {
                    "referenceCategory": "OTHER",
                    "referenceType": "commit",
                    "referenceLocator": git_commit(),
                }
            ],
        }
    ]
    relationships: list[dict[str, str]] = [
        {"spdxElementId": document_id, "relationshipType": "DESCRIBES", "relatedSpdxElement": root_id}
    ]

    for dep in parse_python_dependencies():
        name = dep.split(";", 1)[0].strip()
        pkg = package(f"pypi:{name}")
        packages.append(pkg)
        relationships.append(
            {"spdxElementId": root_id, "relationshipType": "DEPENDS_ON", "relatedSpdxElement": pkg["SPDXID"]}
        )

    for dep in parse_vcpkg_dependencies():
        pkg = package(f"vcpkg:{dep}")
        packages.append(pkg)
        relationships.append(
            {"spdxElementId": root_id, "relationshipType": "DEPENDS_ON", "relatedSpdxElement": pkg["SPDXID"]}
        )

    ib_pkg = package("interactive-brokers-api-vendored", ibapi_version(), "Organization: Interactive Brokers")
    packages.append(ib_pkg)
    relationships.append(
        {"spdxElementId": root_id, "relationshipType": "CONTAINS", "relatedSpdxElement": ib_pkg["SPDXID"]}
    )

    files = ibapi_files()
    for file_entry in files:
        relationships.append(
            {
                "spdxElementId": ib_pkg["SPDXID"],
                "relationshipType": "CONTAINS",
                "relatedSpdxElement": file_entry["SPDXID"],
            }
        )

    return {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": document_id,
        "name": f"regimeflow-{version}",
        "documentNamespace": f"https://github.com/gregorian-09/regime-flow/sbom/{version}/{git_commit()}",
        "creationInfo": {
            "created": created,
            "creators": ["Tool: tools/security/generate_sbom.py"],
        },
        "packages": packages,
        "files": files,
        "relationships": relationships,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="output SPDX JSON path")
    args = parser.parse_args()

    sbom = build_sbom()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(sbom, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
