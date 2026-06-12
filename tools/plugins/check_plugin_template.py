#!/usr/bin/env python3
"""Validate that plugin SDK templates keep required ABI exports documented and present."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
REQUIRED_EXPORTS = [
    "create_plugin",
    "destroy_plugin",
    "plugin_type",
    "plugin_name",
    "regimeflow_abi_version",
]
TEMPLATES = {
    "strategy": (ROOT / "examples" / "plugins" / "template" / "strategy_template.cpp", "strategy"),
    "regime_detector": (
        ROOT / "examples" / "plugins" / "regime_detector_template" / "regime_detector_template.cpp",
        "regime_detector",
    ),
    "risk_manager": (
        ROOT / "examples" / "plugins" / "risk_manager_template" / "risk_manager_template.cpp",
        "risk_manager",
    ),
    "data_source": (
        ROOT / "examples" / "plugins" / "data_source_template" / "data_source_template.cpp",
        "data_source",
    ),
    "metrics": (
        ROOT / "examples" / "plugins" / "metrics_template" / "metrics_template.cpp",
        "metrics",
    ),
}


def main() -> int:
    failed = False
    for name, (source, plugin_type) in TEMPLATES.items():
        readme = source.with_name("README.md")
        if not source.is_file():
            print(f"missing plugin template source: {source.relative_to(ROOT)}", file=sys.stderr)
            failed = True
            continue
        if not readme.is_file():
            print(f"missing plugin template README: {readme.relative_to(ROOT)}", file=sys.stderr)
            failed = True
            continue
        source_text = source.read_text(encoding="utf-8")
        readme_text = readme.read_text(encoding="utf-8")
        for export in REQUIRED_EXPORTS:
            if export not in source_text or export not in readme_text:
                print(f"{name} template missing export reference: {export}", file=sys.stderr)
                failed = True
        if plugin_type not in source_text or plugin_type not in readme_text:
            print(f"{name} template missing plugin_type value: {plugin_type}", file=sys.stderr)
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
