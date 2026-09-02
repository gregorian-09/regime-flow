from __future__ import annotations

import numpy as np

from .._types import SupportsEquityCurve, SupportsTrades


def equity_to_numpy(results: SupportsEquityCurve) -> tuple[np.ndarray, np.ndarray]:
    if not hasattr(results, "equity_curve"):
        raise AttributeError("results must provide equity_curve()")
    df = results.equity_curve()
    times = df.index.to_numpy()
    equity = df["equity"].to_numpy(dtype=float)
    return times, equity


def trades_to_numpy(results: SupportsTrades) -> np.ndarray:
    if not hasattr(results, "trades"):
        raise AttributeError("results must provide trades()")
    df = results.trades()
    return df.to_numpy()


__all__ = ["equity_to_numpy", "trades_to_numpy"]
