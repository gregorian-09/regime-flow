#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any


def _compute_percentile(values: list[float], percentile: float) -> float:
    if not values:
        return 0.0
    sorted_values = sorted(values)
    if len(sorted_values) == 1:
        return sorted_values[0]
    rank = max(0.0, min(100.0, percentile)) / 100.0 * (len(sorted_values) - 1)
    lower = int(rank)
    upper = min(len(sorted_values) - 1, lower + 1)
    frac = rank - lower
    return sorted_values[lower] * (1.0 - frac) + sorted_values[upper] * frac


def _safe_float(value: Any) -> float:
    try:
        return float(value)
    except Exception:
        return 0.0


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarize live dashboard telemetry JSONL output.")
    parser.add_argument(
        "path",
        nargs="?",
        default=os.environ.get("DASHBOARD_TELEMETRY_FILE", "logs/live_dashboard_telemetry.jsonl"),
        help="Path to telemetry JSONL file (default: DASHBOARD_TELEMETRY_FILE or logs/live_dashboard_telemetry.jsonl).",
    )
    parser.add_argument("--tail", type=int, default=0, help="Only include last N telemetry rows.")
    parser.add_argument("--profile", default="", help="Filter to one profile (balanced, low_latency, high_history).")
    return parser.parse_args()


def _load_rows(path: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as handle:
        for line in handle:
            text = line.strip()
            if not text:
                continue
            try:
                row = json.loads(text)
            except Exception:
                continue
            if not isinstance(row, dict):
                continue
            if row.get("event") != "live_dashboard_telemetry":
                continue
            rows.append(row)
    return rows


def _print_group(title: str, rows: list[dict[str, Any]]) -> None:
    patch_p50 = [_safe_float(row.get("replay_patch_p50_ms")) for row in rows]
    patch_p95 = [_safe_float(row.get("replay_patch_p95_ms")) for row in rows]
    extend_p50 = [_safe_float(row.get("replay_extend_p50_ms")) for row in rows]
    extend_p95 = [_safe_float(row.get("replay_extend_p95_ms")) for row in rows]
    hit_rates = [_safe_float(row.get("overlay_cache_hit_rate_pct")) for row in rows]
    latest = rows[-1]

    print(title)
    print(f"  rows: {len(rows)}")
    print(f"  latest_ts: {latest.get('ts', 'n/a')}")
    print(f"  latest_speed/window: {latest.get('speed', 'n/a')}/{latest.get('zoom_bars', 'n/a')}")
    print(
        "  patch p50/p95 (snapshot p50 over time): "
        f"{_compute_percentile(patch_p50, 50):.2f}/{_compute_percentile(patch_p95, 50):.2f} ms"
    )
    print(
        "  extend p50/p95 (snapshot p50 over time): "
        f"{_compute_percentile(extend_p50, 50):.2f}/{_compute_percentile(extend_p95, 50):.2f} ms"
    )
    print(
        "  overlay cache hit-rate avg/p95: "
        f"{_compute_percentile(hit_rates, 50):.1f}%/{_compute_percentile(hit_rates, 95):.1f}%"
    )


def main() -> int:
    args = _parse_args()
    path = Path(args.path)
    if not path.exists():
        print(f"Telemetry file not found: {path}")
        return 1

    rows = _load_rows(path)
    if args.profile:
        wanted = args.profile.strip().lower()
        rows = [row for row in rows if str(row.get("profile", "")).strip().lower() == wanted]

    if args.tail and args.tail > 0:
        rows = rows[-args.tail :]

    if not rows:
        print(f"No telemetry rows found in {path}")
        return 1

    print(f"Telemetry Source: {path}")
    _print_group("All Profiles", rows)
    profiles = sorted({str(row.get("profile", "")).strip() for row in rows if row.get("profile")})
    for profile in profiles:
        profile_rows = [row for row in rows if str(row.get("profile", "")).strip() == profile]
        if profile_rows:
            _print_group(f"Profile={profile}", profile_rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
