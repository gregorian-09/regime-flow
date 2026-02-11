from __future__ import annotations

from typing import Any, Dict, TYPE_CHECKING

import pandas as pd

if TYPE_CHECKING:
    from .. import BacktestResults


def _compute_drawdown(equity: pd.Series) -> pd.Series:
    peak = equity.cummax()
    return equity / peak - 1.0


def _plot_with_plotly(results: "BacktestResults") -> Dict[str, Any]:
    import plotly.graph_objects as go
    from plotly.subplots import make_subplots

    equity = results.equity_curve()
    drawdown = _compute_drawdown(equity["equity"])

    fig = make_subplots(rows=2, cols=1, shared_xaxes=True, vertical_spacing=0.1)
    fig.add_trace(go.Scatter(x=equity.index, y=equity["equity"], name="Equity"), row=1, col=1)
    fig.add_trace(go.Scatter(x=equity.index, y=drawdown, name="Drawdown"), row=2, col=1)
    fig.update_yaxes(title_text="Equity", row=1, col=1)
    fig.update_yaxes(title_text="Drawdown", row=2, col=1)
    fig.update_layout(title="Backtest Results", height=700)

    return {"figure": fig, "equity": equity, "drawdown": drawdown}


def _plot_with_matplotlib(results: "BacktestResults") -> Dict[str, Any]:
    import matplotlib.pyplot as plt

    equity = results.equity_curve()
    drawdown = _compute_drawdown(equity["equity"])

    fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    axes[0].plot(equity.index, equity["equity"])
    axes[0].set_title("Equity Curve")
    axes[0].set_ylabel("Equity")

    axes[1].fill_between(drawdown.index, drawdown, 0.0, alpha=0.4, color="red")
    axes[1].set_title("Drawdown")
    axes[1].set_ylabel("Drawdown")
    fig.tight_layout()

    return {"figure": fig, "equity": equity, "drawdown": drawdown}


def plot_results(results: "BacktestResults") -> Dict[str, Any]:
    try:
        return _plot_with_plotly(results)
    except Exception:
        pass
    try:
        return _plot_with_matplotlib(results)
    except Exception as exc:
        raise ImportError(
            "Plotting requires plotly or matplotlib. Install with `regimeflow[viz]`."
        ) from exc


def create_dashboard(results: "BacktestResults", interactive: bool = True) -> Dict[str, Any]:
    if interactive:
        try:
            from .dashboard_app import create_dash_app

            return create_dash_app(results)
        except Exception:
            pass
    return plot_results(results)


__all__ = ["plot_results", "create_dashboard"]
