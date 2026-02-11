from __future__ import annotations

from typing import Any

from .reporting import report_html


def display_report(results: Any) -> None:
    try:
        from IPython.display import HTML, display
    except Exception as exc:
        raise ImportError("display_report requires IPython in a Jupyter environment") from exc
    display(HTML(report_html(results)))


def display_equity(results: Any) -> None:
    try:
        from IPython.display import display
    except Exception as exc:
        raise ImportError("display_equity requires IPython in a Jupyter environment") from exc
    try:
        from regimeflow.visualization import plot_results
    except Exception as exc:
        raise ImportError("display_equity requires regimeflow.visualization") from exc
    payload = plot_results(results)
    fig = payload.get("figure")
    if fig is not None and hasattr(fig, "show"):
        fig.show()
    else:
        display(fig)


__all__ = ["display_report", "display_equity"]
