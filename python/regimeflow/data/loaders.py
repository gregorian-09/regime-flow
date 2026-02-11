from __future__ import annotations

from typing import Iterable, Optional, Sequence

import pandas as pd

from .. import Tick, Timestamp
from .pandas_utils import dataframe_to_bars


def _normalize_timestamp_column(df: pd.DataFrame, column: str) -> pd.DataFrame:
    if column not in df.columns:
        raise ValueError(f"CSV must contain '{column}' column")
    df = df.copy()
    df[column] = pd.to_datetime(df[column], utc=True, errors="coerce")
    return df


def load_csv_bars(
    path: str,
    symbol: Optional[str] = None,
    timestamp_col: str = "timestamp",
    tz: Optional[str] = None,
    utc: bool = True,
) -> Sequence[object]:
    """Load bars from CSV and return a list of Bar objects.

    The CSV should include: timestamp, open, high, low, close, volume.
    """
    df = pd.read_csv(path)
    df = _normalize_timestamp_column(df, timestamp_col)
    if tz:
        df[timestamp_col] = df[timestamp_col].dt.tz_convert(tz)
    if utc:
        df[timestamp_col] = df[timestamp_col].dt.tz_convert("UTC")
    if symbol is not None and "symbol" not in df.columns:
        df["symbol"] = symbol
    df = df.rename(columns={timestamp_col: "timestamp"})
    if "timestamp" in df.columns:
        df = df.set_index("timestamp")
    return dataframe_to_bars(df)


def load_csv_ticks(
    path: str,
    symbol: Optional[str] = None,
    timestamp_col: str = "timestamp",
    tz: Optional[str] = None,
    utc: bool = True,
) -> Sequence[object]:
    """Load ticks from CSV and return a list of Tick objects.

    The CSV should include: timestamp, price, quantity.
    """
    df = pd.read_csv(path)
    df = _normalize_timestamp_column(df, timestamp_col)
    if tz:
        df[timestamp_col] = df[timestamp_col].dt.tz_convert(tz)
    if utc:
        df[timestamp_col] = df[timestamp_col].dt.tz_convert("UTC")
    if symbol is not None and "symbol" not in df.columns:
        df["symbol"] = symbol
    df = df.rename(columns={timestamp_col: "timestamp"})
    ticks = []
    for _, row in df.iterrows():
        tick = Tick()
        tick.timestamp = Timestamp.from_datetime(row["timestamp"])
        tick.price = float(row["price"])
        tick.quantity = float(row["quantity"])
        if "symbol" in row and row["symbol"] is not None:
            tick.symbol = row["symbol"]
        ticks.append(tick)
    return ticks


def load_csv_dataframe(path: str, timestamp_col: str = "timestamp") -> pd.DataFrame:
    df = pd.read_csv(path)
    return _normalize_timestamp_column(df, timestamp_col)


__all__ = [
    "load_csv_bars",
    "load_csv_ticks",
    "load_csv_dataframe",
]
