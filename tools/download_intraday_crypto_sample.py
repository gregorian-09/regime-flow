#!/usr/bin/env python3
"""Download small intraday crypto samples and convert them to RegimeFlow CSV format."""
from __future__ import annotations

import argparse
import csv
import datetime as dt
import pathlib
import urllib.request
import zipfile

DEFAULT_BTC_URL = (
    "https://data.binance.vision/data/spot/monthly/klines/BTCUSDT/1m/"
    "BTCUSDT-1m-2024-01.zip"
)
DEFAULT_ETH_URL = (
    "https://data.binance.vision/data/spot/monthly/klines/ETHUSDT/1m/"
    "ETHUSDT-1m-2024-01.zip"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Download intraday BTCUSDT/ETHUSDT sample CSVs")
    parser.add_argument("--url", default=DEFAULT_BTC_URL)
    parser.add_argument("--output", default="examples/python_custom_regime_ensemble/data_intraday/BTCUSDT.csv")
    parser.add_argument("--multi", action="store_true", help="Download BTCUSDT and ETHUSDT samples")
    parser.add_argument("--max-rows", type=int, default=10000)
    return parser.parse_args()


def format_timestamp(raw: str) -> str:
    ts = int(raw) / 1000.0
    return dt.datetime.fromtimestamp(ts, dt.UTC).strftime("%Y-%m-%d %H:%M:%S")


def fetch(url: str, out: pathlib.Path, max_rows: int) -> None:
    tmp_path = out.with_suffix(".raw")
    urllib.request.urlretrieve(url, tmp_path)  # noqa: S310

    rows_written = 0
    try:
        with zipfile.ZipFile(tmp_path) as zf:
            name = zf.namelist()[0]
            with zf.open(name, "r") as infile, out.open("w", encoding="utf-8", newline="") as outfile:
                reader = csv.reader((line.decode("utf-8") for line in infile))
                writer = csv.writer(outfile)
                writer.writerow(["timestamp", "open", "high", "low", "close", "volume"])

                for row in reader:
                    if len(row) < 6:
                        continue
                    writer.writerow([format_timestamp(row[0]), row[1], row[2], row[3], row[4], row[5]])
                    rows_written += 1
                    if rows_written >= max_rows:
                        break
    finally:
        tmp_path.unlink(missing_ok=True)

    print(f"Wrote {rows_written} rows to {out}")


def main() -> None:
    args = parse_args()
    output_path = pathlib.Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    if args.multi:
        fetch(DEFAULT_BTC_URL, output_path.parent / "BTCUSDT.csv", args.max_rows)
        fetch(DEFAULT_ETH_URL, output_path.parent / "ETHUSDT.csv", args.max_rows)
    else:
        fetch(args.url, output_path, args.max_rows)


if __name__ == "__main__":
    main()
