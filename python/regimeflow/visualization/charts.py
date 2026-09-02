from __future__ import annotations

from typing import TYPE_CHECKING

import pandas as pd

if TYPE_CHECKING:
    from .. import BacktestResults


def _compute_drawdown(equity: pd.Series) -> pd.Series:
    peak = equity.cummax()
    return equity / peak - 1.0


def _plot_with_plotly(results: "BacktestResults") -> dict[str, object]:
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


def _plot_with_matplotlib(results: "BacktestResults") -> dict[str, object]:
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


def plot_results(results: "BacktestResults") -> dict[str, object]:
    try:
        import plotly  # noqa: F401 - availability probe for the optional backend
    except ImportError:
        try:
            import matplotlib  # noqa: F401 - availability probe for the fallback backend
        except ImportError as exc:
            raise ImportError(
                "Plotting requires plotly or matplotlib. Install with `regimeflow[viz]`."
            ) from exc
        return _plot_with_matplotlib(results)
    return _plot_with_plotly(results)


def create_dashboard(results: "BacktestResults", interactive: bool = True) -> dict[str, object]:
    if interactive:
        try:
            import dash  # noqa: F401 - availability probe for the optional frontend
        except ImportError:
            pass
        else:
            from .dashboard_app import create_dash_app

            return create_dash_app(results)

    from .dashboard import create_strategy_tester_dashboard

    return create_strategy_tester_dashboard(results)


__all__ = ["plot_results", "create_dashboard"]
