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

    def table_component(df: pd.DataFrame, max_rows: int = 500) -> Any:
        if df.empty:
            return html.Div("No data available.")
        return dash_table.DataTable(
            columns=[{"name": col, "id": col} for col in df.columns],
            data=df.head(max_rows).to_dict("records"),
            page_size=min(20, len(df)),
            style_table={"overflowX": "auto"},
            style_cell={"fontFamily": "Arial, sans-serif", "fontSize": 12},
        )

    equity_fig = go.Figure()
    if not equity_df.empty and "equity" in equity_df.columns:
        equity_fig.add_trace(go.Scatter(x=equity_df.index, y=equity_df["equity"], name="Equity"))
    if not drawdown.empty:
        equity_fig.add_trace(go.Scatter(x=drawdown.index, y=drawdown, name="Drawdown", yaxis="y2"))
    equity_fig.update_layout(
        title="Equity & Drawdown",
        yaxis=dict(title="Equity"),
        yaxis2=dict(title="Drawdown", overlaying="y", side="right"),
        height=500,
        legend=dict(orientation="h"),
    )

    regime_fig = go.Figure()
    if not regime_perf_df.empty and "regime" in regime_perf_df.columns:
        regime_fig.add_trace(
            go.Bar(x=regime_perf_df["regime"], y=regime_perf_df.get("return", 0.0), name="Return")
        )
        regime_fig.update_layout(title="Regime Performance", height=400)

    transition_fig = go.Figure()
    if not transition_matrix.empty:
        transition_fig = go.Figure(
            data=go.Heatmap(
                z=transition_matrix.values,
                x=transition_matrix.columns,
                y=transition_matrix.index,
                colorscale="Viridis",
            )
        )
        transition_fig.update_layout(title="Regime Transition Avg Return", height=400)

    diagnostics_payload = report if report else {"status": "No report data available"}

    app = dash.Dash(__name__)
    app.layout = html.Div(
        [
            html.H2(title),
            dcc.Tabs(
                [
                    dcc.Tab(
                        label="Equity",
                        children=[dcc.Graph(figure=equity_fig), table_component(perf_df)],
                    ),
                    dcc.Tab(
                        label="Regimes",
                        children=[
                            dcc.Graph(figure=regime_fig),
                            dcc.Graph(figure=transition_fig),
                            table_component(regime_perf_df),
                        ],
                    ),
                    dcc.Tab(
                        label="Attribution",
                        children=[
                            html.H4("Regime Attribution"),
                            table_component(regime_metrics_df),
                            html.H4("Performance Summary"),
                            table_component(summary_df),
                        ],
                    ),
                    dcc.Tab(
                        label="Risk",
                        children=[
                            html.H4("Performance Summary"),
                            table_component(summary_df),
                            html.H4("Risk Metrics"),
                            table_component(perf_df),
                        ],
                    ),
                    dcc.Tab(
                        label="Trades",
                        children=[table_component(trades_df)],
                    ),
                    dcc.Tab(
                        label="Diagnostics",
                        children=[html.Pre(str(diagnostics_payload))],
                    ),
                ]
            ),
        ],
        style={"padding": "20px"},
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
