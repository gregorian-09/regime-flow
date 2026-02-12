#!/usr/bin/env python3
import json
import sys
from collections import defaultdict


def first_json_line(lines):
    for line in lines:
        s = line.strip()
        if s.startswith("{") or s.startswith("["):
            return s
    return ""


def parse_sections(path):
    with open(path, "r", encoding="utf-8") as f:
        raw = f.read()
    sections = defaultdict(list)
    current = None
    for line in raw.splitlines():
        if line.strip() in {"assets:", "bars:", "trades:", "snapshot:"}:
            current = line.strip()[:-1]
            continue
        if current is None:
            continue
        sections[current].append(line)
    out = {}
    for key in ("assets", "bars", "trades", "snapshot"):
        payload = first_json_line(sections.get(key, []))
        if not payload:
            out[key] = None
            continue
        out[key] = json.loads(payload)
    return out


def main():
    if len(sys.argv) != 2:
        print("Usage: validate_alpaca_fetch.py <path>")
        return 2
    data = parse_sections(sys.argv[1])
    present = [k for k, v in data.items() if v is not None]
    print("sections_present:", present)

    assets = data.get("assets")
    if assets is None:
        print("assets: not present")
    else:
        print("assets_count:", len(assets))

    bars = data.get("bars")
    if bars is None:
        print("bars: not present")
    else:
        bar_map = bars.get("bars", {}) if isinstance(bars, dict) else {}
        print("bar_symbols:", list(bar_map.keys()))
        for sym, arr in bar_map.items():
            timestamps = [b.get("t") for b in arr if isinstance(b, dict)]
            print(f"bars[{sym}] count:", len(arr))
            print(f"bars[{sym}] first_ts:", timestamps[0] if timestamps else None)
            print(f"bars[{sym}] last_ts:", timestamps[-1] if timestamps else None)
            missing = [b for b in arr if not all(k in b for k in ("t", "o", "h", "l", "c", "v"))]
            print(f"bars[{sym}] missing_fields:", len(missing))

    trades = data.get("trades")
    if trades is None:
        print("trades: not present")
    else:
        trade_map = trades.get("trades", {}) if isinstance(trades, dict) else {}
        print("trade_symbols:", list(trade_map.keys()))
        next_page = trades.get("next_page_token") if isinstance(trades, dict) else None
        print("trades_next_page_token:", next_page)
        for sym, arr in trade_map.items():
            timestamps = [t.get("t") for t in arr if isinstance(t, dict)]
            print(f"trades[{sym}] count:", len(arr))
            print(f"trades[{sym}] first_ts:", timestamps[0] if timestamps else None)
            print(f"trades[{sym}] last_ts:", timestamps[-1] if timestamps else None)
            missing = [t for t in arr if not all(k in t for k in ("t", "p", "s"))]
            print(f"trades[{sym}] missing_fields:", len(missing))

    snapshot = data.get("snapshot")
    if snapshot is None:
        print("snapshot: not present")
    else:
        print("snapshot_keys:", sorted(snapshot.keys()))
        print("snapshot_symbol:", snapshot.get("symbol"))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
