from __future__ import annotations

from typing import Any, Dict, Iterable, Mapping, Optional, Union

import pandas as pd


def _compute_drawdown(equity: pd.Series) -> pd.Series:
    peak = equity.cummax()
    return equity / peak - 1.0


def _normalize_equity(equity: Union[pd.DataFrame, pd.Series, Any]) -> pd.DataFrame:
    if hasattr(equity, "equity_curve"):
        equity = equity.equity_curve()
    if isinstance(equity, pd.Series):
        equity = equity.to_frame(name="equity")
    if not isinstance(equity, pd.DataFrame):
        raise TypeError("equity must be a DataFrame, Series, or BacktestResults")
    if "equity" not in equity.columns:
        raise ValueError("equity DataFrame must include an 'equity' column")
    return equity


def _normalize_table(value: Optional[Union[pd.DataFrame, Iterable[Mapping[str, Any]]]]) -> pd.DataFrame:
    if value is None:
        return pd.DataFrame()
    if isinstance(value, pd.DataFrame):
        return value
    return pd.DataFrame(list(value))


def _normalize_regime_state(value: Optional[Union[Mapping[str, Any], Any]]) -> Dict[str, Any]:
    if value is None:
        return {}
    if isinstance(value, Mapping):
        return dict(value)
    out = {}
    for key in ("regime", "confidence", "probabilities"):
        if hasattr(value, key):
            out[key] = getattr(value, key)
    return out


def _normalize_timestamp(value: Any) -> Any:
    if value is None:
        return None
    if hasattr(value, "to_datetime"):
        try:
            return value.to_datetime()
        except Exception:
            pass
    if hasattr(value, "isoformat"):
        try:
            return pd.to_datetime(value)
        except Exception:
            pass
    if hasattr(value, "microseconds"):
        try:
            return pd.to_datetime(value.microseconds(), unit="us")
        except Exception:
            pass
    return value


def _normalize_equity_curve(value: Any) -> pd.DataFrame:
    if value is None:
        return pd.DataFrame(columns=["equity"])
    if isinstance(value, pd.DataFrame):
        return value
    records = []
    for item in value:
        if isinstance(item, Mapping):
            ts = item.get("timestamp")
            equity = item.get("equity")
        else:
            ts = getattr(item, "timestamp", None)
            equity = getattr(item, "equity", None)
        records.append({"timestamp": _normalize_timestamp(ts), "equity": equity})
    df = pd.DataFrame(records)
    if "timestamp" in df.columns and not df["timestamp"].isna().all():
        df = df.set_index("timestamp")
    return df


def create_live_dashboard(
    equity: Union[pd.DataFrame, pd.Series, Any],
    positions: Optional[Union[pd.DataFrame, Iterable[Mapping[str, Any]]]] = None,
    orders: Optional[Union[pd.DataFrame, Iterable[Mapping[str, Any]]]] = None,
    regime_state: Optional[Union[Mapping[str, Any], Any]] = None,
    alerts: Optional[Iterable[str]] = None,
    ) -> Dict[str, Any]:
    equity_df = _normalize_equity(equity)
    drawdown = _compute_drawdown(equity_df["equity"])
    positions_df = _normalize_table(positions)
    orders_df = _normalize_table(orders)
    regime = _normalize_regime_state(regime_state)
    alert_list = list(alerts) if alerts is not None else []

    try:
        import plotly.graph_objects as go
        from plotly.subplots import make_subplots

        fig = make_subplots(rows=2, cols=1, shared_xaxes=True, vertical_spacing=0.1)
        fig.add_trace(go.Scatter(x=equity_df.index, y=equity_df["equity"], name="Equity"),
                      row=1, col=1)
        fig.add_trace(go.Scatter(x=equity_df.index, y=drawdown, name="Drawdown"),
                      row=2, col=1)
        fig.update_yaxes(title_text="Equity", row=1, col=1)
        fig.update_yaxes(title_text="Drawdown", row=2, col=1)
        fig.update_layout(title="Live Trading Dashboard", height=700)

        return {
            "figure": fig,
            "equity": equity_df,
            "drawdown": drawdown,
            "positions": positions_df,
            "orders": orders_df,
            "regime_state": regime,
            "alerts": alert_list,
        }
    except Exception:
        pass

    try:
        import matplotlib.pyplot as plt

        fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
        axes[0].plot(equity_df.index, equity_df["equity"])
        axes[0].set_title("Equity Curve (Live)")
        axes[0].set_ylabel("Equity")

        axes[1].fill_between(drawdown.index, drawdown, 0.0, alpha=0.4, color="red")
        axes[1].set_title("Drawdown")
        axes[1].set_ylabel("Drawdown")
        fig.tight_layout()

        return {
            "figure": fig,
            "equity": equity_df,
            "drawdown": drawdown,
            "positions": positions_df,
            "orders": orders_df,
            "regime_state": regime,
            "alerts": alert_list,
        }
    except Exception as exc:
        raise ImportError(
            "Plotting requires plotly or matplotlib. Install with `regimeflow[viz]`."
        ) from exc


def dashboard_snapshot_to_live_dashboard(snapshot: Mapping[str, Any]) -> Dict[str, Any]:
    equity = _normalize_equity_curve(snapshot.get("equity_curve"))
    positions = snapshot.get("positions")
    orders = snapshot.get("open_orders")
    regime_state = snapshot.get("current_regime")
    alerts = snapshot.get("alerts")
    return create_live_dashboard(
        equity=equity,
        positions=positions,
        orders=orders,
        regime_state=regime_state,
        alerts=alerts,
    )


def _load_report_json(results_or_report: Any) -> Dict[str, Any]:
    if results_or_report is None:
        return {}
    if isinstance(results_or_report, Mapping):
        if "report_json" in results_or_report:
            raw = results_or_report["report_json"]
        else:
            raw = results_or_report.get("report")
    elif hasattr(results_or_report, "report_json"):
        raw = results_or_report.report_json()
    else:
        raw = None
    if not raw:
        return {}
    try:
        import json

        if isinstance(raw, str):
            return json.loads(raw)
        if isinstance(raw, bytes):
            return json.loads(raw.decode("utf-8"))
    except Exception:
        return {}
    return {}


def _normalize_trades(results_or_snapshot: Any) -> pd.DataFrame:
    if results_or_snapshot is None:
        return pd.DataFrame()
    if isinstance(results_or_snapshot, Mapping) and "trades" in results_or_snapshot:
        return _normalize_table(results_or_snapshot["trades"])
    if hasattr(results_or_snapshot, "trades"):
        try:
            trades = results_or_snapshot.trades()
            return trades if isinstance(trades, pd.DataFrame) else _normalize_table(trades)
        except Exception:
            return pd.DataFrame()
    return pd.DataFrame()


def _normalize_regime_metrics(results_or_snapshot: Any) -> pd.DataFrame:
    if results_or_snapshot is None:
        return pd.DataFrame()
    if isinstance(results_or_snapshot, Mapping) and "regime_metrics" in results_or_snapshot:
        data = results_or_snapshot["regime_metrics"]
    elif hasattr(results_or_snapshot, "regime_metrics"):
        try:
            data = results_or_snapshot.regime_metrics()
        except Exception:
            return pd.DataFrame()
    else:
        return pd.DataFrame()
    if isinstance(data, Mapping):
        rows = []
        for regime, metrics in data.items():
            row = {"regime": regime}
            if isinstance(metrics, Mapping):
                row.update(metrics)
            rows.append(row)
        return pd.DataFrame(rows)
    return _normalize_table(data)


def _normalize_equity_input(results_or_snapshot: Any) -> pd.DataFrame:
    if results_or_snapshot is None:
        return pd.DataFrame(columns=["equity"])
    if isinstance(results_or_snapshot, Mapping):
        if "equity_curve" in results_or_snapshot:
            return _normalize_equity_curve(results_or_snapshot["equity_curve"])
    if hasattr(results_or_snapshot, "equity_curve"):
        try:
            return _normalize_equity(results_or_snapshot.equity_curve())
        except Exception:
            return pd.DataFrame(columns=["equity"])
    return pd.DataFrame(columns=["equity"])


def _report_section(report: Dict[str, Any], key: str) -> pd.DataFrame:
    if not report:
        return pd.DataFrame()
    section = report.get(key, {})
    if isinstance(section, Mapping):
        return pd.DataFrame([section])
    if isinstance(section, list):
        return pd.DataFrame(section)
    return pd.DataFrame()


def _build_transition_matrix(transitions: pd.DataFrame) -> pd.DataFrame:
    if transitions.empty or "from" not in transitions.columns or "to" not in transitions.columns:
        return pd.DataFrame()
    return transitions.pivot_table(index="from", columns="to", values="avg_return", fill_value=0.0)


def create_interactive_dashboard(
    results_or_snapshot: Any,
    title: str = "RegimeFlow Dashboard",
    run: bool = False,
    host: str = "127.0.0.1",
    port: int = 8050,
    debug: bool = False,
) -> Any:
    try:
        import dash
        from dash import dash_table, dcc, html
        import plotly.graph_objects as go
    except Exception as exc:
        raise ImportError(
            "Interactive dashboard requires dash and plotly. Install with `regimeflow[viz]`."
        ) from exc

    report = _load_report_json(results_or_snapshot)
    equity_df = _normalize_equity_input(results_or_snapshot)
    trades_df = _normalize_trades(results_or_snapshot)
    drawdown = _compute_drawdown(equity_df["equity"]) if "equity" in equity_df else pd.Series()

    perf_df = _report_section(report, "performance")
    summary_df = _report_section(report, "performance_summary")
    regime_perf_df = _report_section(report, "regime_performance")
    transitions_df = _report_section(report, "transitions")
    transition_matrix = _build_transition_matrix(transitions_df)
    regime_metrics_df = _normalize_regime_metrics(results_or_snapshot)

    theme = {
        "bg": "#0b0f1a",
        "panel": "#121827",
        "panel_alt": "#0f1524",
        "text": "#e7ecf2",
        "muted": "#9aa4b2",
        "accent": "#4cc9f0",
        "accent2": "#f72585",
        "grid": "rgba(255,255,255,0.06)",
    }

    font_stack = "\"Space Grotesk\", \"IBM Plex Sans\", \"Segoe UI\", sans-serif"

    def _card(children: Any, title: Optional[str] = None) -> Any:
        header = html.Div(
            title,
            style={
                "fontSize": "14px",
                "textTransform": "uppercase",
                "letterSpacing": "0.08em",
                "color": theme["muted"],
                "marginBottom": "8px",
            },
        ) if title else None
        return html.Div(
            [header, children] if header else [children],
            style={
                "background": theme["panel"],
                "border": "1px solid rgba(255,255,255,0.06)",
                "boxShadow": "0 12px 30px rgba(0,0,0,0.35)",
                "borderRadius": "16px",
                "padding": "16px",
                "marginBottom": "16px",
            },
        )

    def table_component(df: pd.DataFrame, max_rows: int = 500) -> Any:
        if df.empty:
            return html.Div("No data available.", style={"color": theme["muted"]})
        return dash_table.DataTable(
            columns=[{"name": col, "id": col} for col in df.columns],
            data=df.head(max_rows).to_dict("records"),
            page_size=min(20, len(df)),
            style_table={"overflowX": "auto"},
            style_header={
                "backgroundColor": theme["panel_alt"],
                "color": theme["text"],
                "fontWeight": "600",
                "border": f"1px solid {theme['grid']}",
            },
            style_cell={
                "backgroundColor": theme["panel"],
                "color": theme["text"],
                "fontFamily": font_stack,
                "fontSize": 12,
                "border": f"1px solid {theme['grid']}",
                "padding": "8px",
                "textAlign": "left",
            },
            style_data_conditional=[
                {"if": {"row_index": "odd"}, "backgroundColor": "#0f1424"},
            ],
        )

    equity_fig = go.Figure()
    if not equity_df.empty and "equity" in equity_df.columns:
        equity_fig.add_trace(go.Scatter(
            x=equity_df.index,
            y=equity_df["equity"],
            name="Equity",
            line=dict(color=theme["accent"], width=2),
        ))
    if not drawdown.empty:
        equity_fig.add_trace(go.Scatter(
            x=drawdown.index,
            y=drawdown,
            name="Drawdown",
            yaxis="y2",
            line=dict(color=theme["accent2"], width=1),
            fill="tozeroy",
            opacity=0.18,
        ))
    equity_fig.update_layout(
        title="Equity & Drawdown",
        yaxis=dict(title="Equity"),
        yaxis2=dict(title="Drawdown", overlaying="y", side="right", rangemode="tozero"),
        height=460,
        legend=dict(orientation="h", y=1.02),
        paper_bgcolor=theme["panel"],
        plot_bgcolor=theme["panel"],
        font=dict(family=font_stack, color=theme["text"]),
        margin=dict(l=40, r=40, t=50, b=40),
    )
    equity_fig.update_xaxes(gridcolor=theme["grid"])
    equity_fig.update_yaxes(gridcolor=theme["grid"])

    regime_fig = go.Figure()
    if not regime_perf_df.empty and "regime" in regime_perf_df.columns:
        regime_fig.add_trace(
            go.Bar(
                x=regime_perf_df["regime"],
                y=regime_perf_df.get("return", 0.0),
                name="Return",
                marker_color=theme["accent"],
            )
        )
        regime_fig.update_layout(
            title="Regime Performance",
            height=360,
            paper_bgcolor=theme["panel"],
            plot_bgcolor=theme["panel"],
            font=dict(family=font_stack, color=theme["text"]),
            margin=dict(l=40, r=40, t=50, b=40),
        )
        regime_fig.update_xaxes(gridcolor=theme["grid"])
        regime_fig.update_yaxes(gridcolor=theme["grid"])

    transition_fig = go.Figure()
    if not transition_matrix.empty:
        transition_fig = go.Figure(
            data=go.Heatmap(
                z=transition_matrix.values,
                x=transition_matrix.columns,
                y=transition_matrix.index,
                colorscale="Turbo",
            )
        )
        transition_fig.update_layout(
            title="Regime Transition Avg Return",
            height=360,
            paper_bgcolor=theme["panel"],
            plot_bgcolor=theme["panel"],
            font=dict(family=font_stack, color=theme["text"]),
            margin=dict(l=40, r=40, t=50, b=40),
        )

    if report:
        try:
            import json
            diagnostics_payload = json.dumps(report, indent=2, sort_keys=True)
        except Exception:
            diagnostics_payload = str(report)
    else:
        diagnostics_payload = "No report data available"

    def _metric_card(label: str, value: Any) -> Any:
        return html.Div(
            [
                html.Div(label, style={"color": theme["muted"], "fontSize": "11px"}),
                html.Div(str(value), style={"fontSize": "18px", "fontWeight": "600"}),
            ],
            style={
                "background": theme["panel_alt"],
                "border": "1px solid rgba(255,255,255,0.06)",
                "borderRadius": "12px",
                "padding": "10px 12px",
                "minWidth": "140px",
            },
        )

    summary_metrics = {}
    if not summary_df.empty:
        summary_metrics = summary_df.iloc[0].to_dict()

    def _summary_kv(label: str, value: Any) -> Any:
        return html.Div(
            [
                html.Div(label, style={"color": theme["muted"], "fontSize": "11px"}),
                html.Div(str(value), style={"fontSize": "13px", "fontWeight": "500"}),
            ],
            style={"marginBottom": "6px"},
        )

    summary_left = html.Div(
        [
            _summary_kv("Total Return", summary_metrics.get("total_return", "n/a")),
            _summary_kv("CAGR", summary_metrics.get("cagr", "n/a")),
            _summary_kv("Sharpe Ratio", summary_metrics.get("sharpe_ratio", "n/a")),
            _summary_kv("Max Drawdown", summary_metrics.get("max_drawdown", "n/a")),
        ],
        style={"flex": "1"},
    )
    summary_right = html.Div(
        [
            _summary_kv("Best Day", summary_metrics.get("best_day", "n/a")),
            _summary_kv("Best Day Date", summary_metrics.get("best_day_date", "n/a")),
            _summary_kv("Worst Day", summary_metrics.get("worst_day", "n/a")),
            _summary_kv("Worst Day Date", summary_metrics.get("worst_day_date", "n/a")),
            _summary_kv("Best Month Date", summary_metrics.get("best_month_date", "n/a")),
            _summary_kv("Worst Month Date", summary_metrics.get("worst_month_date", "n/a")),
        ],
        style={"flex": "1"},
    )

    app = dash.Dash(__name__)
    app.layout = html.Div(
        [
            html.Div(
                [
                    html.Div(
                        [
                            html.H2(title, style={"margin": "0 0 6px 0"}),
                            html.Div(
                                "Regime-aware backtest diagnostics",
                                style={"color": theme["muted"], "fontSize": "13px"},
                            ),
                        ]
                    ),
                    html.Div(
                        "RegimeFlow",
                        style={
                            "fontWeight": "600",
                            "color": theme["accent"],
                            "letterSpacing": "0.12em",
                            "textTransform": "uppercase",
                            "fontSize": "12px",
                        },
                    ),
                ],
                style={
                    "display": "flex",
                    "justifyContent": "space-between",
                    "alignItems": "center",
                    "marginBottom": "18px",
                },
            ),
            dcc.Tabs(
                [
                    dcc.Tab(
                        label="Equity",
                        children=[
                            html.Div(
                                [
                                    _metric_card("Total Return", summary_metrics.get("total_return", "n/a")),
                                    _metric_card("Sharpe Ratio", summary_metrics.get("sharpe_ratio", "n/a")),
                                    _metric_card("Max Drawdown", summary_metrics.get("max_drawdown", "n/a")),
                                    _metric_card("Total Trades", summary_metrics.get("total_trades", "n/a")),
                                    _metric_card("Closed Trades", summary_metrics.get("closed_trades", "n/a")),
                                    _metric_card("Open Trades", summary_metrics.get("open_trades", "n/a")),
                                    _metric_card("Open U-PnL", summary_metrics.get("open_trades_unrealized_pnl", "n/a")),
                                ],
                                style={
                                    "display": "flex",
                                    "gap": "12px",
                                    "flexWrap": "wrap",
                                    "marginBottom": "12px",
                                },
                            ),
                            _card(dcc.Graph(figure=equity_fig, config={"displayModeBar": False})),
                            _card(table_component(perf_df), title="Performance Metrics"),
                        ],
                    ),
                    dcc.Tab(
                        label="Regimes",
                        children=[
                            _card(dcc.Graph(figure=regime_fig, config={"displayModeBar": False})),
                            _card(dcc.Graph(figure=transition_fig, config={"displayModeBar": False})),
                            _card(table_component(regime_perf_df), title="Regime Performance Table"),
                        ],
                    ),
                    dcc.Tab(
                        label="Attribution",
                        children=[
                            _card(table_component(regime_metrics_df), title="Regime Attribution"),
                            _card(table_component(summary_df), title="Performance Summary"),
                        ],
                    ),
                    dcc.Tab(
                        label="Risk",
                        children=[
                            _card(table_component(summary_df), title="Performance Summary"),
                            _card(table_component(perf_df), title="Risk Metrics"),
                            _card(
                                html.Div(
                                    [summary_left, summary_right],
                                    style={"display": "flex", "gap": "16px"},
                                ),
                                title="Best/Worst Dates",
                            ),
                        ],
                    ),
                    dcc.Tab(
                        label="Trades",
                        children=[_card(table_component(trades_df), title="Trades")],
                    ),
                    dcc.Tab(
                        label="Diagnostics",
                        children=[
                            _card(
                                html.Pre(
                                    diagnostics_payload,
                                    style={
                                        "color": theme["text"],
                                        "whiteSpace": "pre-wrap",
                                        "fontSize": "12px",
                                    },
                                ),
                                title="Raw Report Payload",
                            )
                        ],
                    ),
                ]
            ),
        ],
        style={
            "padding": "24px 28px",
            "background": "radial-gradient(circle at 10% 10%, #1b2340 0%, #0b0f1a 45%, #090d18 100%)",
            "minHeight": "100vh",
            "color": theme["text"],
            "fontFamily": font_stack,
        },
    )

    if run:
        app.run_server(host=host, port=port, debug=debug)
    return app


__all__ = [
    "create_live_dashboard",
    "dashboard_snapshot_to_live_dashboard",
    "_normalize_equity_curve",
    "create_interactive_dashboard",
]
