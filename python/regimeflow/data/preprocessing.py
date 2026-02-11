from __future__ import annotations

from typing import Optional

import pandas as pd


def normalize_timezone(df: pd.DataFrame, timestamp_col: str = "timestamp", tz: Optional[str] = None) -> pd.DataFrame:
    if timestamp_col not in df.columns:
        raise ValueError(f"DataFrame must include '{timestamp_col}' column")
    out = df.copy()
    out[timestamp_col] = pd.to_datetime(out[timestamp_col], errors="coerce")
    if tz is not None:
        out[timestamp_col] = out[timestamp_col].dt.tz_localize(tz, ambiguous="infer", nonexistent="shift_forward")
    out[timestamp_col] = out[timestamp_col].dt.tz_convert("UTC")
    return out


def fill_missing_time_bars(
    df: pd.DataFrame,
    freq: str,
    timestamp_col: str = "timestamp",
    price_cols: Optional[list[str]] = None,
) -> pd.DataFrame:
    if timestamp_col not in df.columns:
        raise ValueError(f"DataFrame must include '{timestamp_col}' column")
    price_cols = price_cols or ["open", "high", "low", "close"]
    out = df.copy()
    out[timestamp_col] = pd.to_datetime(out[timestamp_col], errors="coerce")
    out = out.sort_values(timestamp_col)
    out = out.set_index(timestamp_col)
    out = out.asfreq(freq)
    if "close" in out.columns:
        out["close"] = out["close"].ffill()
    for col in price_cols:
        if col in out.columns:
            out[col] = out[col].fillna(out["close"])
    if "volume" in out.columns:
        out["volume"] = out["volume"].fillna(0)
    out = out.reset_index()
    return out


__all__ = ["normalize_timezone", "fill_missing_time_bars"]
