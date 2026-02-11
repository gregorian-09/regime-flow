from __future__ import annotations

from typing import Any, Callable, Mapping

from .dashboard import (
    create_interactive_dashboard,
    _compute_drawdown,
    _normalize_equity_curve,
    _normalize_table,
    _normalize_regime_state,
)


def create_dash_app(results: Any):
    return create_interactive_dashboard(results)


def create_live_dash_app(
    snapshot_provider: Callable[[], Mapping[str, Any]],
    title: str = "RegimeFlow Live Dashboard",
    interval_ms: int = 1000,
):
    try:
        import dash
        from dash import dash_table, dcc, html
        import plotly.graph_objects as go
    except Exception as exc:
        raise ImportError("Dash is required for live dashboards. Install with `regimeflow[viz]`.") from exc

    app = dash.Dash(__name__)

    app.layout = html.Div(
        [
            html.H2(title),
            dcc.Graph(id="equity-graph"),
            dcc.Graph(id="drawdown-graph"),
            html.H4("Positions"),
            dash_table.DataTable(id="positions-table"),
            html.H4("Open Orders"),
            dash_table.DataTable(id="orders-table"),
            html.H4("Regime"),
            html.Div(id="regime-state"),
            html.H4("Alerts"),
            html.Pre(id="alerts"),
            dcc.Interval(id="refresh-interval", interval=interval_ms, n_intervals=0),
        ],
        style={"padding": "20px"},
    )

    @app.callback(
        [
            dash.Output("equity-graph", "figure"),
            dash.Output("drawdown-graph", "figure"),
            dash.Output("positions-table", "data"),
            dash.Output("positions-table", "columns"),
            dash.Output("orders-table", "data"),
            dash.Output("orders-table", "columns"),
            dash.Output("regime-state", "children"),
            dash.Output("alerts", "children"),
        ],
        [dash.Input("refresh-interval", "n_intervals")],
    )
    def refresh(_):
        snapshot = snapshot_provider()
        equity_df = _normalize_equity_curve(snapshot.get("equity_curve"))
        drawdown = _compute_drawdown(equity_df["equity"]) if "equity" in equity_df else equity_df

        fig_equity = go.Figure()
        if not equity_df.empty and "equity" in equity_df.columns:
            fig_equity.add_trace(
                go.Scatter(x=equity_df.index, y=equity_df["equity"], name="Equity")
            )
        fig_equity.update_layout(height=350, title="Equity")

        fig_drawdown = go.Figure()
        if drawdown is not None and not drawdown.empty:
            fig_drawdown.add_trace(
                go.Scatter(x=drawdown.index, y=drawdown, name="Drawdown")
            )
        fig_drawdown.update_layout(height=300, title="Drawdown")

        positions_df = _normalize_table(snapshot.get("positions"))
        orders_df = _normalize_table(snapshot.get("open_orders"))
        regime = _normalize_regime_state(snapshot.get("current_regime"))

        positions_data = positions_df.to_dict("records")
        positions_cols = [{"name": c, "id": c} for c in positions_df.columns]
        orders_data = orders_df.to_dict("records")
        orders_cols = [{"name": c, "id": c} for c in orders_df.columns]
        alerts = "\n".join(snapshot.get("alerts", []))
        regime_text = str(regime)

        return (
            fig_equity,
            fig_drawdown,
            positions_data,
            positions_cols,
            orders_data,
            orders_cols,
            regime_text,
            alerts,
        )

    return app


def launch_dashboard(results: Any, host: str = "127.0.0.1", port: int = 8050, debug: bool = False) -> None:
    app = create_dash_app(results)
    app.run_server(host=host, port=port, debug=debug)


def launch_live_dashboard(snapshot_provider: Callable[[], Mapping[str, Any]],
                          host: str = "127.0.0.1",
                          port: int = 8050,
                          debug: bool = False,
                          interval_ms: int = 1000) -> None:
    app = create_live_dash_app(snapshot_provider, interval_ms=interval_ms)
    app.run_server(host=host, port=port, debug=debug)


def export_dashboard_html(results: Any, path: str) -> str:
    import plotly.io as pio
    import plotly.graph_objects as go

    equity = results.equity_curve()
    drawdown = equity["equity"] / equity["equity"].cummax() - 1.0

    fig = go.Figure()
    fig.add_trace(go.Scatter(x=equity.index, y=equity["equity"], name="Equity"))
    fig.add_trace(go.Scatter(x=equity.index, y=drawdown, name="Drawdown", yaxis="y2"))
    fig.update_layout(
        title="RegimeFlow Dashboard",
        yaxis=dict(title="Equity"),
        yaxis2=dict(title="Drawdown", overlaying="y", side="right"),
    )
    html = pio.to_html(fig, full_html=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(html)
    return path


__all__ = [
    "create_dash_app",
    "launch_dashboard",
    "create_live_dash_app",
    "launch_live_dashboard",
    "export_dashboard_html",
]
