#!/usr/bin/env python3
"""Validate that the plugin SDK template keeps the required ABI exports documented and present."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
TEMPLATE = ROOT / "examples" / "plugins" / "template" / "strategy_template.cpp"
README = ROOT / "examples" / "plugins" / "template" / "README.md"
REQUIRED = [
    "create_plugin",
    "destroy_plugin",
    "plugin_type",
    "plugin_name",
    "regimeflow_abi_version",
]


def main() -> int:
    source = TEMPLATE.read_text(encoding="utf-8")
    readme = README.read_text(encoding="utf-8")
    missing = [name for name in REQUIRED if name not in source or name not in readme]
    if missing:
        for name in missing:
            print(f"missing plugin template export reference: {name}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
