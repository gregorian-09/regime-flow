from __future__ import annotations

from .reporting import report_html
from .._types import SupportsEquityCurve, SupportsReportJson


def display_report(results: SupportsReportJson) -> None:
    try:
        from IPython.display import HTML, display
    except ImportError as exc:
        raise ImportError("display_report requires IPython in a Jupyter environment") from exc
    display(HTML(report_html(results)))


def display_equity(results: SupportsEquityCurve) -> None:
    try:
        from IPython.display import display
    except ImportError as exc:
        raise ImportError("display_equity requires IPython in a Jupyter environment") from exc
    try:
        from regimeflow.visualization import plot_results
    except ImportError as exc:
        raise ImportError("display_equity requires regimeflow.visualization") from exc
    payload = plot_results(results)
    fig = payload.get("figure")
    if fig is not None and hasattr(fig, "show"):
        fig.show()
    else:
        display(fig)


__all__ = ["display_report", "display_equity"]
