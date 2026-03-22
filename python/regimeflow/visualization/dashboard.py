from __future__ import annotations

from functools import lru_cache
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping, Optional, Union

import pandas as pd


BLOOMBERG_THEME: Dict[str, str] = {
    "bg": "#0B0F14",
    "panel": "#111821",
    "panel_alt": "#16202B",
    "border": "#233242",
    "text": "#D7E1EA",
    "muted": "#8FA1B3",
    "accent": "#39A0ED",
    "positive": "#37C978",
    "negative": "#FF9F43",
    "warning": "#FFD166",
    "grid": "rgba(143, 161, 179, 0.14)",
}

MT5_STYLE_TABS = ("Setup", "Report", "Graph", "Trades", "Orders", "Venues", "Optimization", "Journal")
FONT_STACK = '"IBM Plex Sans", "Space Grotesk", "Segoe UI", sans-serif'


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
    for key in ("regime", "name", "confidence", "probabilities"):
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


@lru_cache(maxsize=16)
def _load_market_bars_cached(
    data_directory: str,
    file_pattern: str,
    symbol: str,
    start_date: str,
    end_date: str,
) -> pd.DataFrame:
    path = Path(data_directory) / file_pattern.format(symbol=symbol)
    if not path.exists():
        return pd.DataFrame()

    try:
        bars = pd.read_csv(path)
    except Exception:
        return pd.DataFrame()

    required = {"timestamp", "open", "high", "low", "close"}
    if not required.issubset(bars.columns):
        return pd.DataFrame()

    bars = bars.copy()
    bars["timestamp"] = pd.to_datetime(bars["timestamp"], errors="coerce")
    bars = bars.dropna(subset=["timestamp"]).sort_values("timestamp")

    if start_date:
        bars = bars[bars["timestamp"] >= pd.to_datetime(start_date, errors="coerce")]
    if end_date:
        bars = bars[bars["timestamp"] < pd.to_datetime(end_date, errors="coerce") + pd.Timedelta(days=1)]
    return bars.reset_index(drop=True)


def _load_market_bars(setup: Mapping[str, Any]) -> pd.DataFrame:
    if setup.get("data_source") != "csv":
        return pd.DataFrame()
    data_directory = setup.get("data_directory")
    file_pattern = setup.get("file_pattern")
    symbols = setup.get("symbols") or []
    if not data_directory or not file_pattern or not symbols:
        return pd.DataFrame()
    return _load_market_bars_cached(
        str(data_directory),
        str(file_pattern),
        str(symbols[0]),
        str(setup.get("start_date", "")),
        str(setup.get("end_date", "")),
    )


def _is_walkforward_results(value: Any) -> bool:
    return hasattr(value, "windows") and hasattr(value, "stitched_oos_results")


def _extract_base_results(value: Any) -> Any:
    if _is_walkforward_results(value):
        return value.stitched_oos_results
    return value


def _load_dashboard_snapshot(results_or_snapshot: Any) -> Dict[str, Any]:
    if results_or_snapshot is None:
        return {}
    if isinstance(results_or_snapshot, Mapping):
        return dict(results_or_snapshot)
    base_results = _extract_base_results(results_or_snapshot)
    if hasattr(base_results, "dashboard_snapshot"):
        try:
            snapshot = base_results.dashboard_snapshot()
            if isinstance(snapshot, Mapping):
                return dict(snapshot)
        except Exception:
            return {}
    return {}


def _snapshot_section(snapshot: Mapping[str, Any], key: str) -> Dict[str, Any]:
    value = snapshot.get(key, {})
    return dict(value) if isinstance(value, Mapping) else {}


def _load_report_json(results_or_report: Any) -> Dict[str, Any]:
    if results_or_report is None:
        return {}
    if isinstance(results_or_report, Mapping):
        if "report_json" in results_or_report:
            raw = results_or_report["report_json"]
        else:
            raw = results_or_report.get("report")
    else:
        base_results = _extract_base_results(results_or_report)
        if hasattr(base_results, "report_json"):
            raw = base_results.report_json()
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
    base_results = _extract_base_results(results_or_snapshot)
    if hasattr(base_results, "trades"):
        try:
            trades = base_results.trades()
            return trades if isinstance(trades, pd.DataFrame) else _normalize_table(trades)
        except Exception:
            return pd.DataFrame()
    return pd.DataFrame()


def _normalize_tester_report(results_or_snapshot: Any) -> Dict[str, Any]:
    if results_or_snapshot is None:
        return {}
    if isinstance(results_or_snapshot, Mapping) and "tester_report" in results_or_snapshot:
        value = results_or_snapshot["tester_report"]
        return dict(value) if isinstance(value, Mapping) else {}
    base_results = _extract_base_results(results_or_snapshot)
    if hasattr(base_results, "tester_report"):
        try:
            value = base_results.tester_report()
            return dict(value) if isinstance(value, Mapping) else {}
        except Exception:
            return {}
    return {}


def _normalize_tester_journal(results_or_snapshot: Any) -> pd.DataFrame:
    if results_or_snapshot is None:
        return pd.DataFrame()
    if isinstance(results_or_snapshot, Mapping) and "tester_journal" in results_or_snapshot:
        return _normalize_table(results_or_snapshot["tester_journal"])
    base_results = _extract_base_results(results_or_snapshot)
    if hasattr(base_results, "tester_journal"):
        try:
            value = base_results.tester_journal()
            return value if isinstance(value, pd.DataFrame) else _normalize_table(value)
        except Exception:
            return pd.DataFrame()
    return pd.DataFrame()


def _normalize_regime_metrics(results_or_snapshot: Any) -> pd.DataFrame:
    if results_or_snapshot is None:
        return pd.DataFrame()
    if isinstance(results_or_snapshot, Mapping) and "regime_metrics" in results_or_snapshot:
        data = results_or_snapshot["regime_metrics"]
    else:
        base_results = _extract_base_results(results_or_snapshot)
        if hasattr(base_results, "regime_metrics"):
            try:
                data = base_results.regime_metrics()
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


def _normalize_optimization_payload(results_or_snapshot: Any) -> Dict[str, Any]:
    if not _is_walkforward_results(results_or_snapshot):
        return {
            "enabled": False,
            "summary": {},
            "windows": pd.DataFrame(),
            "params": pd.DataFrame(),
            "stability": pd.DataFrame(),
        }

    results = results_or_snapshot
    window_rows = []
    for index, window in enumerate(getattr(results, "windows", []), start=1):
        row = {
            "window": index,
            "in_sample_start": window.in_sample_range[0],
            "in_sample_end": window.in_sample_range[1],
            "out_of_sample_start": window.out_of_sample_range[0],
            "out_of_sample_end": window.out_of_sample_range[1],
            "is_fitness": window.is_fitness,
            "oos_fitness": window.oos_fitness,
            "efficiency_ratio": window.efficiency_ratio,
            "oos_total_return": getattr(window.oos_results, "total_return", None),
            "oos_max_drawdown": getattr(window.oos_results, "max_drawdown", None),
            "oos_sharpe": getattr(window.oos_results, "sharpe_ratio", None),
            "optimal_params": ", ".join(f"{k}={v}" for k, v in window.optimal_params.items()),
        }
        for key, value in window.optimal_params.items():
            row[f"param_{key}"] = value
        window_rows.append(row)

    param_frame = pd.DataFrame()
    param_evolution = getattr(results, "param_evolution", {})
    if isinstance(param_evolution, Mapping) and param_evolution:
        series_by_param = {
            name: pd.Series(list(values), dtype="float64") for name, values in param_evolution.items()
        }
        param_frame = pd.DataFrame(series_by_param)
        param_frame.index = pd.RangeIndex(start=1, stop=len(param_frame.index) + 1, name="window")

    stability = pd.DataFrame(
        [
            {"parameter": name, "stability_score": score}
            for name, score in getattr(results, "param_stability_score", {}).items()
        ]
    )

    summary = {
        "avg_is_sharpe": getattr(results, "avg_is_sharpe", None),
        "avg_oos_sharpe": getattr(results, "avg_oos_sharpe", None),
        "overall_oos_sharpe": getattr(results, "overall_oos_sharpe", None),
        "avg_efficiency_ratio": getattr(results, "avg_efficiency_ratio", None),
        "potential_overfit": getattr(results, "potential_overfit", None),
        "overfit_diagnosis": getattr(results, "overfit_diagnosis", ""),
        "regime_consistency_score": getattr(results, "regime_consistency_score", None),
    }

    return {
        "enabled": True,
        "summary": summary,
        "windows": pd.DataFrame(window_rows),
        "params": param_frame,
        "stability": stability,
    }


def _normalize_equity_input(results_or_snapshot: Any) -> pd.DataFrame:
    if results_or_snapshot is None:
        return pd.DataFrame(columns=["equity"])
    if isinstance(results_or_snapshot, Mapping):
        if "account_curve" in results_or_snapshot:
            return _normalize_equity_curve(results_or_snapshot["account_curve"])
        if "equity_curve" in results_or_snapshot:
            return _normalize_equity_curve(results_or_snapshot["equity_curve"])
    if hasattr(results_or_snapshot, "account_curve"):
        try:
            return _normalize_equity(results_or_snapshot.account_curve())
        except Exception:
            pass
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


def _format_dashboard_value(value: Any) -> str:
    if value is None:
        return "n/a"
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, int):
        return f"{value}"
    if isinstance(value, float):
        return f"{value:.6g}"
    return str(value)


def _build_report_schema(
    headline: Mapping[str, Any],
    account: Mapping[str, Any],
    execution: Mapping[str, Any],
    report: Mapping[str, Any],
) -> Dict[str, pd.DataFrame]:
    summary_rows = [{"metric": key, "value": _format_dashboard_value(value)} for key, value in headline.items()]
    account_rows = [{"metric": key, "value": _format_dashboard_value(value)} for key, value in account.items()]
    execution_rows = [{"metric": key, "value": _format_dashboard_value(value)} for key, value in execution.items()]

    performance_summary = report.get("performance_summary", {})
    performance_stats = report.get("performance", {})
    report_rows = [{"metric": key, "value": _format_dashboard_value(value)} for key, value in performance_summary.items()]
    statistics_rows = [{"metric": key, "value": _format_dashboard_value(value)} for key, value in performance_stats.items()]

    return {
        "headline": pd.DataFrame(summary_rows),
        "account": pd.DataFrame(account_rows),
        "execution": pd.DataFrame(execution_rows),
        "summary": pd.DataFrame(report_rows),
        "statistics": pd.DataFrame(statistics_rows),
    }


def _build_journal_entries(
    snapshot: Mapping[str, Any],
    trades: pd.DataFrame,
    alerts: Iterable[str],
) -> pd.DataFrame:
    rows = []
    base_time = snapshot.get("timestamp", "n/a")
    for message in alerts:
        rows.append(
            {
                "time": base_time,
                "source": "engine",
                "event": "alert",
                "message": str(message),
            }
        )

    if not trades.empty:
        for _, trade in trades.tail(25).iterrows():
            timestamp = trade.get("timestamp", base_time)
            venue = trade.get("venue", "unassigned")
            side = trade.get("side", "trade")
            quantity = trade.get("quantity", "n/a")
            price = trade.get("price", "n/a")
            rows.append(
                {
                    "time": timestamp,
                    "source": venue,
                    "event": "fill",
                    "message": f"{side} {quantity} @ {price}",
                }
            )

    if not rows:
        rows.append(
            {
                "time": base_time,
                "source": "engine",
                "event": "status",
                "message": "No alerts or fills recorded",
            }
        )

    return pd.DataFrame(rows)


def _normalize_strategy_tester_payload(results_or_snapshot: Any) -> Dict[str, Any]:
    snapshot = _load_dashboard_snapshot(results_or_snapshot)
    report = _load_report_json(results_or_snapshot)
    tester_report = _normalize_tester_report(results_or_snapshot)

    account_curve = _normalize_equity_curve(snapshot.get("account_curve", snapshot.get("equity_curve")))
    if account_curve.empty:
        account_curve = _normalize_equity_input(results_or_snapshot)

    positions = _normalize_table(snapshot.get("positions"))
    orders = _normalize_table(snapshot.get("open_orders", snapshot.get("orders")))
    recent_fills = _normalize_table(snapshot.get("trades"))
    if recent_fills.empty:
        recent_fills = _normalize_table(snapshot.get("recent_fills"))
    if recent_fills.empty:
        recent_fills = _normalize_trades(results_or_snapshot)
    venue_summary = _normalize_table(snapshot.get("venue_summary"))
    quote_summary = _normalize_table(snapshot.get("quote_summary"))
    regime = _normalize_regime_state(snapshot.get("regime", snapshot.get("current_regime")))

    headline = _snapshot_section(snapshot, "headline")
    account = _snapshot_section(snapshot, "account")
    execution = _snapshot_section(snapshot, "execution")
    optimization = _normalize_optimization_payload(results_or_snapshot)

    if not headline and not account_curve.empty and "equity" in account_curve.columns:
        headline = {
            "equity": float(account_curve["equity"].iloc[-1]),
            "total_return": float(account_curve["equity"].iloc[-1] / account_curve["equity"].iloc[0] - 1.0)
            if len(account_curve.index) > 1 and float(account_curve["equity"].iloc[0]) != 0.0
            else 0.0,
        }

    setup = _snapshot_section(snapshot, "setup")
    setup.setdefault("timestamp", snapshot.get("timestamp"))
    setup.setdefault("tick_mode", "snapshot")
    setup.setdefault("bar_mode", "n/a")
    setup.setdefault("regime", regime.get("regime", regime.get("name", "n/a")))
    setup.setdefault("confidence", regime.get("confidence", None))
    setup.setdefault("fills", execution.get("fill_count", len(recent_fills.index)))
    setup.setdefault("open_orders", execution.get("open_order_count", len(orders.index)))

    report_sections = _build_report_schema(headline, account, execution, report)
    if tester_report:
        for key in ("headline", "account", "execution", "summary", "statistics"):
            section = tester_report.get(key)
            if isinstance(section, Mapping):
                rows = [{"metric": section_key, "value": _format_dashboard_value(section_value)}
                        for section_key, section_value in section.items()]
                report_sections[key] = pd.DataFrame(rows)
    journal = _normalize_tester_journal(results_or_snapshot)
    if journal.empty:
        journal = _build_journal_entries(snapshot, recent_fills, snapshot.get("alerts", []))
    elif "event" in journal.columns and "category" in journal.columns:
        preferred_columns = ["time", "source", "category", "event", "message"]
        extra_columns = [column for column in journal.columns if column not in preferred_columns]
        journal = journal[[column for column in preferred_columns if column in journal.columns] + extra_columns]

    market_bars = _load_market_bars(setup)

    return {
        "snapshot": snapshot,
        "report": report,
        "tester_report": tester_report,
        "report_sections": report_sections,
        "theme": dict(BLOOMBERG_THEME),
        "tabs": [tab for tab in MT5_STYLE_TABS if optimization["enabled"] or tab != "Optimization"],
        "headline": headline,
        "account": account,
        "execution": execution,
        "regime": regime,
        "setup": setup,
        "account_curve": account_curve,
        "market_bars": market_bars,
        "positions": positions,
        "orders": orders,
        "quote_summary": quote_summary,
        "recent_fills": recent_fills,
        "venue_summary": venue_summary,
        "optimization": optimization,
        "alerts": list(snapshot.get("alerts", [])),
        "journal": journal,
    }


def create_live_runtime_payload(results_or_snapshot: Any) -> Dict[str, Any]:
    snapshot = _load_dashboard_snapshot(results_or_snapshot)
    account_curve = _normalize_equity_curve(snapshot.get("account_curve", snapshot.get("equity_curve")))
    positions = _normalize_table(snapshot.get("positions"))
    orders = _normalize_table(snapshot.get("open_orders", snapshot.get("orders")))
    recent_fills = _normalize_table(snapshot.get("trades"))
    if recent_fills.empty:
        recent_fills = _normalize_table(snapshot.get("recent_fills"))
    quote_summary = _normalize_table(snapshot.get("quote_summary"))
    regime = _normalize_regime_state(snapshot.get("regime", snapshot.get("current_regime")))
    headline = _snapshot_section(snapshot, "headline")
    account = _snapshot_section(snapshot, "account")
    execution = _snapshot_section(snapshot, "execution")
    setup = _snapshot_section(snapshot, "setup")
    journal = _normalize_tester_journal(results_or_snapshot)
    if journal.empty:
        journal = _build_journal_entries(snapshot, recent_fills, snapshot.get("alerts", []))
    market_bars = _load_market_bars(setup)
    drawdown = _compute_drawdown(account_curve["equity"]) if not account_curve.empty and "equity" in account_curve.columns else pd.Series(dtype=float)
    return {
        "snapshot": snapshot,
        "theme": dict(BLOOMBERG_THEME),
        "setup": setup,
        "headline": headline,
        "account": account,
        "execution": execution,
        "regime": regime,
        "account_curve": account_curve,
        "drawdown": drawdown,
        "market_bars": market_bars,
        "positions": positions,
        "orders": orders,
        "quote_summary": quote_summary,
        "recent_fills": recent_fills,
        "alerts": list(snapshot.get("alerts", [])),
        "journal": journal,
    }


def _build_live_equity_figure(payload: Dict[str, Any]) -> tuple[Any, Any]:
    import plotly.graph_objects as go

    theme = payload["theme"]
    equity_df = payload["account_curve"]
    drawdown = payload["drawdown"]
    x_axis = equity_df["timestamp"] if not equity_df.empty and "timestamp" in equity_df.columns else equity_df.index

    equity_fig = go.Figure()
    if not equity_df.empty and "equity" in equity_df.columns:
        equity_fig.add_trace(
            go.Scatter(
                x=x_axis,
                y=equity_df["equity"],
                mode="lines+markers",
                name="Equity",
                line=dict(color=theme["accent"], width=2),
                marker=dict(size=4, color=theme["accent"]),
            )
        )
    equity_fig.update_layout(
        title="Equity",
        height=340,
        paper_bgcolor=theme["panel"],
        plot_bgcolor=theme["panel"],
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=28, r=28, t=36, b=26),
        legend=dict(orientation="h", y=1.02, x=0.01),
        uirevision="live-equity",
        transition={"duration": 0},
        hovermode="x unified",
    )
    equity_fig.update_xaxes(gridcolor=theme["grid"], fixedrange=False)
    equity_fig.update_yaxes(gridcolor=theme["grid"], fixedrange=False)

    drawdown_fig = go.Figure()
    if not drawdown.empty:
        drawdown_x_axis = equity_df["timestamp"] if not equity_df.empty and "timestamp" in equity_df.columns else drawdown.index
        drawdown_fig.add_trace(
            go.Scatter(
                x=drawdown_x_axis,
                y=drawdown,
                mode="lines+markers",
                name="Drawdown",
                line=dict(color=theme["negative"], width=1),
                marker=dict(size=3, color=theme["negative"]),
                fill="tozeroy",
                fillcolor="rgba(255, 159, 67, 0.16)",
            )
        )
    drawdown_fig.update_layout(
        title="Drawdown",
        height=300,
        paper_bgcolor=theme["panel"],
        plot_bgcolor=theme["panel"],
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=28, r=28, t=36, b=26),
        legend=dict(orientation="h", y=1.02, x=0.01),
        uirevision="live-drawdown",
        transition={"duration": 0},
        hovermode="x unified",
    )
    drawdown_fig.update_xaxes(gridcolor=theme["grid"], fixedrange=False)
    drawdown_fig.update_yaxes(gridcolor=theme["grid"], fixedrange=False)
    return equity_fig, drawdown_fig


def _build_plotly_strategy_tester_figures(payload: Dict[str, Any]) -> Dict[str, Any]:
    import plotly.graph_objects as go
    from plotly.subplots import make_subplots

    theme = payload["theme"]
    equity_df = payload["account_curve"]
    drawdown = payload["drawdown"]
    venue_df = payload["venue_summary"]

    main_fig = make_subplots(
        rows=2,
        cols=1,
        shared_xaxes=True,
        vertical_spacing=0.08,
        row_heights=[0.72, 0.28],
    )
    if not equity_df.empty and "equity" in equity_df.columns:
        main_fig.add_trace(
            go.Scatter(
                x=equity_df.index,
                y=equity_df["equity"],
                mode="lines",
                name="Equity",
                line=dict(color=theme["accent"], width=2),
            ),
            row=1,
            col=1,
        )
    if not drawdown.empty:
        main_fig.add_trace(
            go.Scatter(
                x=drawdown.index,
                y=drawdown,
                mode="lines",
                name="Drawdown",
                line=dict(color=theme["negative"], width=1),
                fill="tozeroy",
                fillcolor="rgba(255, 159, 67, 0.16)",
            ),
            row=2,
            col=1,
        )
    main_fig.update_layout(
        title="Equity / Drawdown",
        height=720,
        paper_bgcolor=theme["panel"],
        plot_bgcolor=theme["panel"],
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=36, r=36, t=52, b=32),
        legend=dict(orientation="h", y=1.02, x=0.01),
    )
    main_fig.update_xaxes(gridcolor=theme["grid"])
    main_fig.update_yaxes(gridcolor=theme["grid"])

    venue_fig = go.Figure()
    if not venue_df.empty and "venue" in venue_df.columns:
        venue_fig.add_trace(
            go.Bar(
                x=venue_df["venue"],
                y=venue_df.get("total_cost", 0.0),
                marker_color=theme["warning"],
                name="Total Cost",
            )
        )
        if "avg_slippage_bps" in venue_df.columns:
            venue_fig.add_trace(
                go.Scatter(
                    x=venue_df["venue"],
                    y=venue_df["avg_slippage_bps"],
                    line=dict(color=theme["accent"], width=2),
                    mode="lines+markers",
                    name="Slippage (bps)",
                    yaxis="y2",
                )
            )
    venue_fig.update_layout(
        title="Venue Cost / Slippage",
        height=340,
        yaxis=dict(title="Cost"),
        yaxis2=dict(title="Slippage (bps)", overlaying="y", side="right"),
        paper_bgcolor=theme["panel"],
        plot_bgcolor=theme["panel"],
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=36, r=36, t=52, b=32),
    )
    venue_fig.update_xaxes(gridcolor=theme["grid"])
    venue_fig.update_yaxes(gridcolor=theme["grid"])

    return {"figure": main_fig, "venue_figure": venue_fig}


def _build_execution_replay_figure(
    payload: Dict[str, Any],
    end_index: Optional[int] = None,
    include_animation: bool = True,
    window_bars: int = 240,
    max_order_annotations: int = 8,
) -> Any:
    import plotly.graph_objects as go

    theme = payload["theme"]
    market_bars = payload["market_bars"]
    trades_df = payload["recent_fills"]
    orders_df = payload["orders"]
    quote_df = payload["quote_summary"]
    journal_df = payload["journal"]

    if market_bars.empty:
        return go.Figure()

    bars = market_bars.copy()
    bars["timestamp"] = pd.to_datetime(bars["timestamp"], errors="coerce")
    bars = bars.dropna(subset=["timestamp"]).reset_index(drop=True)
    if bars.empty:
        return go.Figure()
    window_bars = max(0, int(window_bars))

    current_timestamp = payload.get("snapshot", {}).get("timestamp")
    current_timestamp = pd.to_datetime(current_timestamp, errors="coerce") if current_timestamp is not None else pd.NaT

    trades = trades_df.copy()
    if not trades.empty and "timestamp" in trades.columns:
        trades["timestamp"] = pd.to_datetime(trades["timestamp"], errors="coerce")
        trades = trades.dropna(subset=["timestamp"])
    else:
        trades = pd.DataFrame(columns=["timestamp", "price", "side", "quantity"])

    if end_index is None:
        frame_count = min(180, max(24, len(bars)))
        step = max(1, len(bars) // frame_count)
        frame_end_indices = list(range(step, len(bars) + 1, step))
        if frame_end_indices[-1] != len(bars):
            frame_end_indices.append(len(bars))
        if pd.notna(current_timestamp):
            visible = bars["timestamp"] <= current_timestamp
            active_end = int(visible.sum())
            active_end = min(len(bars), max(1, active_end))
            frame_end_indices = [index for index in frame_end_indices if index <= active_end]
            if not frame_end_indices or frame_end_indices[-1] != active_end:
                frame_end_indices.append(active_end)
        else:
            active_end = frame_end_indices[min(3, len(frame_end_indices) - 1)]
    else:
        active_end = max(1, min(int(end_index), len(bars)))
        frame_end_indices = [active_end]

    def trade_points(side: str, cutoff: pd.Timestamp) -> pd.DataFrame:
        if trades.empty or "quantity" not in trades.columns:
            return pd.DataFrame(columns=["timestamp", "price", "quantity"])
        side_mask = trades["quantity"] > 0 if side == "buy" else trades["quantity"] < 0
        time_mask = trades["timestamp"] <= cutoff
        return trades.loc[side_mask & time_mask]

    def active_orders_at(cutoff: pd.Timestamp) -> pd.DataFrame:
        if journal_df.empty or "time" not in journal_df.columns or "event" not in journal_df.columns:
            return orders_df
        journal = journal_df.copy()
        journal["time"] = pd.to_datetime(journal["time"], errors="coerce")
        journal = journal.dropna(subset=["time"])
        journal = journal[journal["time"] <= cutoff]
        if journal.empty or "order_id" not in journal.columns:
            return orders_df
        active: dict[str, dict[str, Any]] = {}
        for _, row in journal.iterrows():
            event_name = str(row.get("event", "")).lower()
            order_id = str(row.get("order_id", "")).strip()
            if not order_id:
                continue
            if event_name == "order submitted":
                active[order_id] = row.to_dict()
            elif event_name in {"order cancelled", "order rejected", "order filled"}:
                active.pop(order_id, None)
        return pd.DataFrame(active.values()) if active else pd.DataFrame()

    def order_overlay(cutoff: pd.Timestamp, x0: pd.Timestamp, x1: pd.Timestamp, current_close: float) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
        active_orders = active_orders_at(cutoff)
        shapes: list[dict[str, Any]] = []
        annotations: list[dict[str, Any]] = []
        annotation_candidates: list[tuple[float, dict[str, Any]]] = []
        for _, order in active_orders.iterrows():
            limit_price = order.get("limit_price")
            stop_price = order.get("stop_price")
            side = str(order.get("side", "")).lower()
            side_color = theme["positive"] if "buy" in side else theme["negative"]
            if pd.notna(limit_price) and float(limit_price) > 0:
                limit_value = float(limit_price)
                shapes.append(
                    {
                        "type": "line",
                        "xref": "x",
                        "yref": "y",
                        "x0": x0,
                        "x1": x1,
                        "y0": limit_value,
                        "y1": limit_value,
                        "line": {"color": side_color, "dash": "dash", "width": 1},
                        "opacity": 0.75,
                    }
                )
                annotation_candidates.append(
                    (
                        abs(limit_value - current_close),
                        {
                            "x": x1,
                            "y": limit_value,
                            "xref": "x",
                            "yref": "y",
                            "text": f"LMT {order.get('side', '').upper()} #{order.get('order_id', '')}",
                            "showarrow": False,
                            "xanchor": "left",
                            "font": {"size": 10, "color": side_color},
                            "bgcolor": "rgba(11,15,20,0.8)",
                        },
                    )
                )
            if pd.notna(stop_price) and float(stop_price) > 0:
                stop_value = float(stop_price)
                shapes.append(
                    {
                        "type": "line",
                        "xref": "x",
                        "yref": "y",
                        "x0": x0,
                        "x1": x1,
                        "y0": stop_value,
                        "y1": stop_value,
                        "line": {"color": theme["warning"], "dash": "dot", "width": 1},
                        "opacity": 0.75,
                    }
                )
                annotation_candidates.append(
                    (
                        abs(stop_value - current_close),
                        {
                            "x": x1,
                            "y": stop_value,
                            "xref": "x",
                            "yref": "y",
                            "text": f"STP #{order.get('order_id', '')}",
                            "showarrow": False,
                            "xanchor": "left",
                            "font": {"size": 10, "color": theme["warning"]},
                            "bgcolor": "rgba(11,15,20,0.8)",
                        },
                    )
                )
        if not quote_df.empty and {"bid", "ask"}.issubset(quote_df.columns):
            current_quote = quote_df.iloc[0]
            if pd.notna(current_quote.get("bid")) and float(current_quote["bid"]) > 0:
                bid_value = float(current_quote["bid"])
                shapes.append(
                    {
                        "type": "line",
                        "xref": "x",
                        "yref": "y",
                        "x0": x0,
                        "x1": x1,
                        "y0": bid_value,
                        "y1": bid_value,
                        "line": {"color": "#4CB3FF", "dash": "dot", "width": 1},
                        "opacity": 0.7,
                    }
                )
                annotation_candidates.append(
                    (
                        abs(bid_value - current_close),
                        {
                            "x": x1,
                            "y": bid_value,
                            "xref": "x",
                            "yref": "y",
                            "text": f"Bid {bid_value:.2f}",
                            "showarrow": False,
                            "xanchor": "left",
                            "font": {"size": 10, "color": "#4CB3FF"},
                            "bgcolor": "rgba(11,15,20,0.8)",
                        },
                    )
                )
            if pd.notna(current_quote.get("ask")) and float(current_quote["ask"]) > 0:
                ask_value = float(current_quote["ask"])
                shapes.append(
                    {
                        "type": "line",
                        "xref": "x",
                        "yref": "y",
                        "x0": x0,
                        "x1": x1,
                        "y0": ask_value,
                        "y1": ask_value,
                        "line": {"color": "#FFD166", "dash": "dot", "width": 1},
                        "opacity": 0.7,
                    }
                )
                annotation_candidates.append(
                    (
                        abs(ask_value - current_close),
                        {
                            "x": x1,
                            "y": ask_value,
                            "xref": "x",
                            "yref": "y",
                            "text": f"Ask {ask_value:.2f}",
                            "showarrow": False,
                            "xanchor": "left",
                            "font": {"size": 10, "color": "#FFD166"},
                            "bgcolor": "rgba(11,15,20,0.8)",
                        },
                    )
                )
        annotation_candidates.sort(key=lambda item: item[0])
        annotations.extend(annotation for _, annotation in annotation_candidates[:max_order_annotations])
        return shapes, annotations

    def frame_data(end_idx: int) -> list[Any]:
        start_idx = 0 if window_bars == 0 else max(0, end_idx - window_bars)
        current = bars.iloc[start_idx:end_idx]
        cutoff = current["timestamp"].iloc[-1]
        window_start = current["timestamp"].iloc[0]
        buys = trade_points("buy", cutoff)
        buys = buys[buys["timestamp"] >= window_start]
        sells = trade_points("sell", cutoff)
        sells = sells[sells["timestamp"] >= window_start]
        traces: list[Any] = [
            go.Candlestick(
                x=current["timestamp"],
                open=current["open"],
                high=current["high"],
                low=current["low"],
                close=current["close"],
                name="Price",
                increasing_line_color=theme["positive"],
                increasing_fillcolor=theme["positive"],
                decreasing_line_color=theme["negative"],
                decreasing_fillcolor=theme["negative"],
            )
        ]
        traces.append(
            go.Scatter(
                x=buys["timestamp"] if not buys.empty else [],
                y=buys["price"] if not buys.empty else [],
                mode="markers",
                name="Buy Fills",
                marker=dict(symbol="triangle-up", size=11, color=theme["positive"], line=dict(width=1, color="#08110C")),
                customdata=buys.get("quantity") if not buys.empty else [],
                hovertemplate="Buy %{customdata}<br>%{x}<br>%{y:.2f}<extra></extra>",
            )
        )
        traces.append(
            go.Scatter(
                x=sells["timestamp"] if not sells.empty else [],
                y=sells["price"] if not sells.empty else [],
                mode="markers",
                name="Sell Fills",
                marker=dict(symbol="triangle-down", size=11, color=theme["negative"], line=dict(width=1, color="#140A04")),
                customdata=sells.get("quantity") if not sells.empty else [],
                hovertemplate="Sell %{customdata}<br>%{x}<br>%{y:.2f}<extra></extra>",
            )
        )
        return traces

    fig = go.Figure(data=frame_data(active_end))
    active_start = 0 if window_bars == 0 else max(0, active_end - window_bars)
    active_window = bars.iloc[active_start:active_end]
    active_cutoff = active_window["timestamp"].iloc[-1]
    active_shapes, active_annotations = order_overlay(
        active_cutoff,
        active_window["timestamp"].iloc[0],
        active_cutoff,
        float(active_window["close"].iloc[-1]),
    )
    fig.update_layout(
        title="Visual Mode",
        height=640,
        paper_bgcolor="#0F151C",
        plot_bgcolor="#0F151C",
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=34, r=34, t=40, b=36),
        uirevision="live-replay",
        transition={"duration": 0},
        xaxis=dict(
            gridcolor=theme["grid"],
            rangeslider=dict(visible=False),
            showspikes=True,
            spikemode="across",
            spikecolor=theme["accent"],
            fixedrange=False,
        ),
        yaxis=dict(gridcolor=theme["grid"], side="right", fixedrange=False),
        legend=dict(orientation="h", x=0.01, y=1.05, bgcolor="rgba(0,0,0,0)"),
        shapes=active_shapes,
        annotations=active_annotations,
        hovermode="x unified",
        dragmode="pan",
    )
    fig.update_xaxes(range=[active_window["timestamp"].iloc[0], active_window["timestamp"].iloc[-1]])
    if include_animation:
        frames = []
        for end_idx in frame_end_indices:
            frame_start = 0 if window_bars == 0 else max(0, end_idx - window_bars)
            frame_window = bars.iloc[frame_start:end_idx]
            cutoff = frame_window["timestamp"].iloc[-1]
            frame_shapes, frame_annotations = order_overlay(
                cutoff,
                frame_window["timestamp"].iloc[0],
                cutoff,
                float(frame_window["close"].iloc[-1]),
            )
            frames.append(
                go.Frame(
                    name=str(cutoff),
                    data=frame_data(end_idx),
                    layout={
                        "shapes": frame_shapes,
                        "annotations": frame_annotations,
                        "xaxis": {"range": [frame_window["timestamp"].iloc[0], frame_window["timestamp"].iloc[-1]]},
                    },
                )
            )
        fig.frames = frames
        fig.update_layout(
            updatemenus=[
                dict(
                    type="buttons",
                    direction="left",
                    x=0.01,
                    y=1.15,
                    bgcolor="#111821",
                    bordercolor=theme["border"],
                    buttons=[
                        dict(
                            label="Play",
                            method="animate",
                            args=[None, {"frame": {"duration": 90, "redraw": True}, "fromcurrent": True}],
                        ),
                        dict(
                            label="Pause",
                            method="animate",
                            args=[[None], {"frame": {"duration": 0, "redraw": False}, "mode": "immediate"}],
                        ),
                    ],
                )
            ],
            sliders=[
                dict(
                    active=max(0, len(frames) - 1),
                    x=0.14,
                    y=1.14,
                    len=0.84,
                    bgcolor="#0F151C",
                    currentvalue={"prefix": "Bar Time: ", "font": {"color": theme["muted"], "size": 11}},
                    steps=[
                        {
                            "label": bars.iloc[end_idx - 1]["timestamp"].strftime("%m-%d %H:%M"),
                            "method": "animate",
                            "args": [[str(bars.iloc[end_idx - 1]["timestamp"])], {"mode": "immediate", "frame": {"duration": 0, "redraw": True}}],
                        }
                        for end_idx in frame_end_indices
                    ],
                )
            ],
        )
    return fig


def create_strategy_tester_dashboard(
    results_or_snapshot: Any,
    title: str = "RegimeFlow Strategy Tester",
) -> Dict[str, Any]:
    payload = _normalize_strategy_tester_payload(results_or_snapshot)
    equity_df = payload["account_curve"]
    drawdown = _compute_drawdown(equity_df["equity"]) if "equity" in equity_df else pd.Series(dtype=float)
    payload["drawdown"] = drawdown
    payload["title"] = title

    try:
        payload.update(_build_plotly_strategy_tester_figures(payload))
        payload["replay_figure"] = _build_execution_replay_figure(payload)
    except Exception:
        try:
            import matplotlib.pyplot as plt

            fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
            if not equity_df.empty and "equity" in equity_df.columns:
                axes[0].plot(equity_df.index, equity_df["equity"], color=payload["theme"]["accent"])
            axes[0].set_title(title)
            axes[0].set_ylabel("Equity")
            if not drawdown.empty:
                axes[1].fill_between(drawdown.index, drawdown, 0.0, alpha=0.4, color=payload["theme"]["negative"])
            axes[1].set_title("Drawdown")
            axes[1].set_ylabel("Drawdown")
            fig.tight_layout()
            payload["figure"] = fig
            payload["replay_figure"] = fig
        except Exception as exc:
            raise ImportError(
                "Plotting requires plotly or matplotlib. Install with `regimeflow[viz]`."
            ) from exc

    return payload


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
        fig.add_trace(
            go.Scatter(x=equity_df.index, y=equity_df["equity"], name="Equity", line=dict(color=BLOOMBERG_THEME["accent"])),
            row=1,
            col=1,
        )
        fig.add_trace(
            go.Scatter(x=equity_df.index, y=drawdown, name="Drawdown", line=dict(color=BLOOMBERG_THEME["negative"])),
            row=2,
            col=1,
        )
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
    payload = _normalize_strategy_tester_payload(snapshot)
    return create_live_dashboard(
        equity=payload["account_curve"],
        positions=payload["positions"],
        orders=payload["orders"],
        regime_state=payload["regime"],
        alerts=payload["alerts"],
    )


def create_interactive_dashboard(
    results_or_snapshot: Any,
    title: str = "RegimeFlow Strategy Tester",
    run: bool = False,
    host: str = "127.0.0.1",
    port: int = 8050,
    debug: bool = False,
) -> Any:
    try:
        import dash
        from dash import dcc, html
        from dash.dash_table import DataTable
        import plotly.graph_objects as go
    except Exception as exc:
        raise ImportError(
            "Interactive dashboard requires dash and plotly. Install with `regimeflow[viz]`."
        ) from exc

    payload = create_strategy_tester_dashboard(results_or_snapshot, title=title)
    snapshot = dict(payload["snapshot"])
    report = payload["report"]
    report_sections = payload["report_sections"]
    theme = payload["theme"]
    equity_df = payload["account_curve"]
    drawdown = payload["drawdown"]
    market_bars_df = payload["market_bars"]
    positions_df = payload["positions"]
    orders_df = payload["orders"]
    trades_df = payload["recent_fills"]
    venue_df = payload["venue_summary"]
    optimization = payload["optimization"]
    optimization_summary = dict(optimization["summary"])
    optimization_windows_df = optimization["windows"]
    optimization_params_df = optimization["params"]
    optimization_stability_df = optimization["stability"]
    setup = dict(payload["setup"])
    setup_df = pd.DataFrame([payload["setup"]])
    headline = dict(payload["headline"])
    account = dict(payload["account"])
    execution = dict(payload["execution"])
    regime = dict(payload["regime"])
    alerts = payload["alerts"]
    journal_df = payload["journal"]
    replay_max_index = max(1, len(market_bars_df.index))
    replay_initial_index = min(replay_max_index, max(24, replay_max_index // 8))

    perf_df = _report_section(report, "performance")
    summary_df = _report_section(report, "performance_summary")
    regime_perf_df = _report_section(report, "regime_performance")
    transitions_df = _report_section(report, "transitions")
    transition_matrix = _build_transition_matrix(transitions_df)
    regime_metrics_df = _normalize_regime_metrics(results_or_snapshot)

    if not summary_df.empty:
        headline = {**summary_df.iloc[0].to_dict(), **headline}

    shell_card_style = {
        "background": "linear-gradient(180deg, rgba(22,32,43,0.98) 0%, rgba(14,21,29,0.98) 100%)",
        "border": f"1px solid {theme['border']}",
        "boxShadow": "0 16px 36px rgba(0,0,0,0.35)",
        "borderRadius": "14px",
    }

    def _panel(children: Any,
               title_text: Optional[str] = None,
               subtitle_text: Optional[str] = None,
               padding: str = "14px 16px",
               min_height: Optional[str] = None) -> Any:
        header_children = []
        if title_text:
            header_children.append(
                html.Div(
                    title_text,
                    style={
                        "fontSize": "11px",
                        "fontWeight": "600",
                        "textTransform": "uppercase",
                        "letterSpacing": "0.14em",
                        "color": theme["muted"],
                    },
                )
            )
        if subtitle_text:
            header_children.append(
                html.Div(
                    subtitle_text,
                    style={
                        "fontSize": "12px",
                        "color": theme["muted"],
                        "marginTop": "4px",
                    },
                )
            )
        body = [html.Div(header_children, style={"marginBottom": "12px"})] if header_children else []
        body.extend(children if isinstance(children, list) else [children])
        style = dict(shell_card_style)
        style.update({"padding": padding})
        if min_height is not None:
            style["minHeight"] = min_height
        return html.Div(body, style=style)

    def _table(df: pd.DataFrame, max_rows: int = 500, page_size: int = 14) -> Any:
        if df.empty:
            return html.Div("No data available.", style={"color": theme["muted"], "fontSize": "12px"})
        return DataTable(
            columns=[{"name": col, "id": col} for col in df.columns],
            data=df.head(max_rows).to_dict("records"),
            page_size=min(page_size, len(df)),
            sort_action="native",
            style_as_list_view=True,
            style_table={"overflowX": "auto", "overflowY": "auto", "maxHeight": "420px", "minWidth": 0},
            style_header={
                "backgroundColor": "#1A232D",
                "color": theme["text"],
                "fontWeight": "600",
                "fontSize": 10,
                "textTransform": "uppercase",
                "letterSpacing": "0.06em",
                "borderBottom": f"1px solid {theme['border']}",
                "padding": "6px 8px",
            },
            style_cell={
                "backgroundColor": "#10161D",
                "color": theme["text"],
                "fontFamily": FONT_STACK,
                "fontSize": 11,
                "padding": "6px 8px",
                "borderBottom": f"1px solid {theme['border']}",
                "textAlign": "left",
                "minWidth": "72px",
                "maxWidth": "220px",
                "whiteSpace": "nowrap",
                "overflow": "hidden",
                "textOverflow": "ellipsis",
                "fontVariantNumeric": "tabular-nums",
            },
            style_data_conditional=[
                {"if": {"row_index": "odd"}, "backgroundColor": "#0D1319"},
                {"if": {"state": "selected"}, "backgroundColor": "#1B2E40", "border": f"1px solid {theme['accent']}"},
            ],
        )

    def _summary_tile(label: str, value: Any, tone: str = "text") -> Any:
        color = theme.get(tone, theme["text"])
        return html.Div(
            [
                html.Div(
                    label,
                    style={
                        "color": theme["muted"],
                        "fontSize": "10px",
                        "textTransform": "uppercase",
                        "letterSpacing": "0.1em",
                        "marginBottom": "6px",
                    },
                ),
                html.Div(
                    str(value),
                    style={
                        "fontSize": "clamp(14px, 1.35vw, 20px)",
                        "fontWeight": "600",
                        "lineHeight": "1.1",
                        "color": color,
                        "fontVariantNumeric": "tabular-nums",
                        "whiteSpace": "nowrap",
                        "overflow": "hidden",
                        "textOverflow": "ellipsis",
                        "minWidth": 0,
                    },
                ),
            ],
            style={
                "background": "linear-gradient(180deg, rgba(17,24,33,0.95) 0%, rgba(11,15,20,0.95) 100%)",
                "border": f"1px solid {theme['border']}",
                "borderRadius": "12px",
                "padding": "12px 14px",
                "minWidth": 0,
                "maxWidth": "100%",
                "flex": "1 1 132px",
                "overflow": "hidden",
            },
        )

    def _metric_grid(values: Mapping[str, Any]) -> Any:
        if not values:
            return html.Div("No state available.", style={"color": theme["muted"], "fontSize": "12px"})
        cards = []
        for key, value in values.items():
            tone = "text"
            key_name = str(key).lower()
            if "drawdown" in key_name or "cost" in key_name:
                tone = "negative"
            elif "return" in key_name or "sharpe" in key_name or "profit" in key_name or "win" in key_name:
                tone = "positive"
            cards.append(_summary_tile(str(key), value, tone=tone))
        return html.Div(cards, style={"display": "flex", "gap": "10px", "flexWrap": "wrap"})

    def _kv_grid(values: Mapping[str, Any], columns: int = 2) -> Any:
        if not values:
            return html.Div("No state available.", style={"color": theme["muted"], "fontSize": "12px"})
        rows = []
        for key, value in values.items():
            rows.append(
                html.Div(
                    [
                        html.Div(
                            str(key),
                            style={
                                "color": theme["muted"],
                                "fontSize": "10px",
                                "textTransform": "uppercase",
                                "letterSpacing": "0.08em",
                            },
                        ),
                        html.Div(
                            str(value),
                            style={
                                "fontSize": "13px",
                                "fontWeight": "500",
                                "marginTop": "4px",
                                "fontVariantNumeric": "tabular-nums",
                                "whiteSpace": "nowrap",
                                "overflow": "hidden",
                                "textOverflow": "ellipsis",
                            },
                        ),
                    ],
                    style={
                        "padding": "8px 0",
                        "borderBottom": f"1px solid {theme['border']}",
                    },
                )
            )
        return html.Div(
            rows,
            style={
                "display": "grid",
                "gridTemplateColumns": f"repeat({columns}, minmax(0, 1fr))",
                "columnGap": "14px",
            },
        )

    def _control_chip(label: str, value: Any) -> Any:
        return html.Div(
            [
                html.Div(
                    label,
                    style={
                        "fontSize": "10px",
                        "color": theme["muted"],
                        "textTransform": "uppercase",
                        "letterSpacing": "0.08em",
                    },
                ),
                html.Div(
                    str(value),
                    style={
                        "fontSize": "12px",
                        "fontWeight": "600",
                        "color": theme["text"],
                        "marginTop": "4px",
                        "fontVariantNumeric": "tabular-nums",
                        "whiteSpace": "nowrap",
                        "overflow": "hidden",
                        "textOverflow": "ellipsis",
                    },
                ),
            ],
            style={
                "padding": "10px 12px",
                "border": f"1px solid {theme['border']}",
                "borderRadius": "10px",
                "background": "rgba(17,24,33,0.86)",
                "minWidth": 0,
                "maxWidth": "100%",
                "flex": "1 1 116px",
                "overflow": "hidden",
            },
        )

    tab_style = {
        "background": "#111821",
        "border": f"1px solid {theme['border']}",
        "borderBottom": "none",
        "color": theme["muted"],
        "padding": "8px 12px",
        "fontSize": "10px",
        "fontWeight": "600",
        "textTransform": "uppercase",
        "letterSpacing": "0.08em",
        "marginRight": "2px",
        "borderTopLeftRadius": "6px",
        "borderTopRightRadius": "6px",
    }
    selected_tab_style = {
        **tab_style,
        "color": theme["text"],
        "borderTop": f"2px solid {theme['accent']}",
        "background": "#1A232D",
    }

    def _tab(label: str, children: Any) -> Any:
        return dcc.Tab(
            label=label,
            children=children,
            style=tab_style,
            selected_style=selected_tab_style,
        )

    replay_fig = _build_execution_replay_figure(payload, end_index=replay_initial_index, include_animation=False)
    equity_fig = go.Figure(payload["figure"]) if isinstance(payload["figure"], go.Figure) else go.Figure()
    if equity_fig.data == () and not equity_df.empty and "equity" in equity_df.columns:
        equity_fig.add_trace(
            go.Scatter(x=equity_df.index, y=equity_df["equity"], name="Equity", line=dict(color=theme["accent"], width=2))
        )
        if not drawdown.empty:
            equity_fig.add_trace(
                go.Scatter(
                    x=drawdown.index,
                    y=drawdown,
                    name="Drawdown",
                    yaxis="y2",
                    line=dict(color=theme["negative"], width=1),
                    fill="tozeroy",
                    opacity=0.18,
                )
            )
        equity_fig.update_layout(
            title=None,
            yaxis=dict(title="Equity"),
            yaxis2=dict(title="Drawdown", overlaying="y", side="right", rangemode="tozero"),
            paper_bgcolor="#0F151C",
            plot_bgcolor="#0F151C",
            font=dict(family=FONT_STACK, color=theme["text"]),
            margin=dict(l=44, r=44, t=18, b=32),
            legend=dict(orientation="h", x=0.01, y=1.08, bgcolor="rgba(0,0,0,0)"),
        )
        equity_fig.add_annotation(
            xref="paper",
            yref="paper",
            x=0.01,
            y=0.98,
            text="Balance / Equity",
            showarrow=False,
            font=dict(size=11, color=theme["muted"]),
            align="left",
        )
    equity_fig.update_xaxes(gridcolor="rgba(143, 161, 179, 0.10)", zeroline=False)
    equity_fig.update_yaxes(gridcolor="rgba(143, 161, 179, 0.10)", zeroline=False)

    venue_fig = go.Figure(payload.get("venue_figure")) if isinstance(payload.get("venue_figure"), go.Figure) else go.Figure()
    if venue_fig.data == () and not venue_df.empty and "venue" in venue_df.columns:
        venue_fig.add_trace(
            go.Bar(x=venue_df["venue"], y=venue_df.get("total_cost", 0.0), marker_color=theme["warning"], name="Total Cost")
        )
        venue_fig.update_layout(
            title="Venue Cost Split",
            paper_bgcolor="#0F151C",
            plot_bgcolor="#0F151C",
            font=dict(family=FONT_STACK, color=theme["text"]),
            margin=dict(l=30, r=30, t=34, b=28),
        )
    venue_fig.update_xaxes(gridcolor="rgba(143, 161, 179, 0.10)")
    venue_fig.update_yaxes(gridcolor="rgba(143, 161, 179, 0.10)")

    regime_fig = go.Figure()
    if not regime_perf_df.empty and "regime" in regime_perf_df.columns:
        regime_fig.add_trace(
            go.Bar(
                x=regime_perf_df["regime"],
                y=regime_perf_df.get("return", 0.0),
                marker_color=theme["accent"],
                name="Return",
            )
        )
    regime_fig.update_layout(
        title="Regime Performance",
        height=340,
        paper_bgcolor="#0F151C",
        plot_bgcolor="#0F151C",
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=30, r=30, t=34, b=28),
    )
    regime_fig.update_xaxes(gridcolor="rgba(143, 161, 179, 0.10)")
    regime_fig.update_yaxes(gridcolor="rgba(143, 161, 179, 0.10)")

    transition_fig = go.Figure()
    if not transition_matrix.empty:
        transition_fig = go.Figure(
            data=go.Heatmap(
                z=transition_matrix.values,
                x=transition_matrix.columns,
                y=transition_matrix.index,
                colorscale=[[0.0, "#111821"], [0.5, "#39A0ED"], [1.0, "#FFD166"]],
            )
        )
    transition_fig.update_layout(
        title="Regime Transition Matrix",
        height=340,
        paper_bgcolor="#0F151C",
        plot_bgcolor="#0F151C",
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=30, r=30, t=34, b=28),
    )

    optimization_fig = go.Figure()
    if not optimization_windows_df.empty:
        optimization_fig.add_trace(
            go.Scatter(
                x=optimization_windows_df["window"],
                y=optimization_windows_df["is_fitness"],
                mode="lines+markers",
                name="IS Fitness",
                line=dict(color=theme["accent"], width=2),
            )
        )
        optimization_fig.add_trace(
            go.Scatter(
                x=optimization_windows_df["window"],
                y=optimization_windows_df["oos_fitness"],
                mode="lines+markers",
                name="OOS Fitness",
                line=dict(color=theme["warning"], width=2),
            )
        )
    optimization_fig.update_layout(
        title="Optimization Window Fitness",
        height=340,
        paper_bgcolor="#0F151C",
        plot_bgcolor="#0F151C",
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=30, r=30, t=34, b=28),
    )
    optimization_fig.update_xaxes(gridcolor="rgba(143, 161, 179, 0.10)")
    optimization_fig.update_yaxes(gridcolor="rgba(143, 161, 179, 0.10)")

    param_fig = go.Figure()
    if not optimization_params_df.empty:
        for column in optimization_params_df.columns:
            param_fig.add_trace(
                go.Scatter(
                    x=optimization_params_df.index,
                    y=optimization_params_df[column],
                    mode="lines+markers",
                    name=str(column),
                )
            )
    param_fig.update_layout(
        title="Parameter Evolution",
        height=340,
        paper_bgcolor="#0F151C",
        plot_bgcolor="#0F151C",
        font=dict(family=FONT_STACK, color=theme["text"]),
        margin=dict(l=30, r=30, t=34, b=28),
    )
    param_fig.update_xaxes(gridcolor="rgba(143, 161, 179, 0.10)")
    param_fig.update_yaxes(gridcolor="rgba(143, 161, 179, 0.10)")

    if report:
        try:
            import json

            diagnostics_payload = json.dumps(report, indent=2, sort_keys=True)
        except Exception:
            diagnostics_payload = str(report)
    else:
        diagnostics_payload = "No report data available"

    base_time = setup.get("timestamp", snapshot.get("timestamp", "n/a"))
    diagnostics_lines = diagnostics_payload.splitlines()
    diagnostics_table_df = pd.DataFrame(
        [{"line": idx + 1, "payload": line} for idx, line in enumerate(diagnostics_lines[:120])]
    )

    tabs = [
        _tab("Setup", [_panel(_table(setup_df), title_text="Tester Setup")]),
        _tab(
            "Report",
            [
                _panel(_metric_grid(headline), title_text="Report Summary"),
                _panel(_table(report_sections["summary"], page_size=16), title_text="Summary Metrics"),
                _panel(_table(report_sections["statistics"], page_size=14), title_text="Detailed Statistics"),
                _panel(_table(report_sections["account"], page_size=10), title_text="Account State"),
                _panel(_table(report_sections["execution"], page_size=10), title_text="Execution State"),
            ],
        ),
        _tab(
            "Graph",
            [
                _panel(dcc.Graph(figure=replay_fig, config={"displayModeBar": False})),
                _panel(dcc.Graph(figure=equity_fig, config={"displayModeBar": False})),
                _panel(dcc.Graph(figure=venue_fig, config={"displayModeBar": False})),
                _panel(dcc.Graph(figure=regime_fig, config={"displayModeBar": False})),
                _panel(dcc.Graph(figure=transition_fig, config={"displayModeBar": False})),
            ],
        ),
        _tab("Trades", [_panel(_table(trades_df), title_text="Recent Fills")]),
        _tab(
            "Orders",
            [
                _panel(_table(orders_df), title_text="Open Orders"),
                _panel(_table(positions_df), title_text="Positions"),
            ],
        ),
        _tab(
            "Venues",
            [
                _panel(_table(venue_df), title_text="Venue Summary"),
                _panel(_table(regime_metrics_df), title_text="Regime Attribution"),
                _panel(_table(perf_df), title_text="Risk Metrics"),
            ],
        ),
    ]

    if optimization["enabled"]:
        tabs.append(
            _tab(
                "Optimization",
                [
                    _panel(_kv_grid(optimization_summary), title_text="Optimization Summary"),
                    _panel(dcc.Graph(figure=optimization_fig, config={"displayModeBar": False})),
                    _panel(dcc.Graph(figure=param_fig, config={"displayModeBar": False})),
                    _panel(_table(optimization_windows_df), title_text="Window Passes"),
                    _panel(_table(optimization_params_df.reset_index()), title_text="Parameter Evolution"),
                    _panel(_table(optimization_stability_df), title_text="Parameter Stability"),
                ],
            )
        )

    tabs.append(
        _tab(
            "Journal",
            [
                _panel(
                    _table(journal_df, page_size=10),
                    title_text="Journal",
                    subtitle_text="Tester event log",
                ),
                _panel(
                    _table(diagnostics_table_df, page_size=12),
                    title_text="Diagnostics Payload",
                    subtitle_text="Structured report serializer output",
                ),
                _panel(_kv_grid(regime, columns=1), title_text="Current Regime"),
            ],
        )
    )

    control_bar_items = [
        ("Strategy", setup.get("strategy_name", "n/a")),
        ("Symbols", ", ".join(setup.get("symbols", [])) if setup.get("symbols") else "n/a"),
        ("Range", f"{setup.get('start_date', 'n/a')} -> {setup.get('end_date', 'n/a')}"),
        ("Bar", setup.get("bar_type", "n/a")),
        ("Tick Model", setup.get("tick_mode", "n/a")),
        ("Execution", setup.get("execution_model", "n/a")),
        ("Fill Policy", setup.get("fill_policy", "n/a")),
    ]
    if optimization["enabled"]:
        control_bar_items.append(("Optimization", "walk-forward"))

    setup_overview = {
        "Data Source": setup.get("data_source", "n/a"),
        "Bar Mode": setup.get("bar_mode", "n/a"),
        "Synthetic Tick": setup.get("synthetic_tick_profile", "n/a"),
        "Timestamp": setup.get("timestamp", "n/a"),
    }
    market_model = {
        "Routing": "enabled" if setup.get("routing_enabled") else "off",
        "Queue": "enabled" if setup.get("queue_enabled") else "off",
        "Session": "enabled" if setup.get("session_enabled") else "off",
        "Drift": setup.get("price_drift_action", "ignore"),
    }
    risk_controls = {
        "Margin": "enabled" if setup.get("margin_enabled") else "off",
        "Financing": "enabled" if setup.get("financing_enabled") else "off",
        "Margin Call": account.get("margin_call", "clear"),
        "Stop Out": account.get("stop_out", "clear"),
    }

    right_rail_metrics = {
        "Regime": regime.get("regime", regime.get("name", "n/a")),
        "Confidence": regime.get("confidence", "n/a"),
        "Alerts": len(alerts),
        "Open Orders": execution.get("open_order_count", len(orders_df.index)),
        "Fills": execution.get("fill_count", len(trades_df.index)),
    }

    def _toolbar_button(label: str, active: bool = False) -> Any:
        border = theme["accent"] if active else theme["border"]
        color = theme["text"] if active else theme["muted"]
        background = "#1A2A38" if active else "rgba(17,24,33,0.92)"
        return html.Div(
            label,
            style={
                "padding": "8px 12px",
                "border": f"1px solid {border}",
                "borderRadius": "6px",
                "background": background,
                "color": color,
                "fontSize": "11px",
                "fontWeight": "600",
                "textTransform": "uppercase",
                "letterSpacing": "0.08em",
            },
        )

    def _status_pill(label: str, value: Any, tone: str = "muted") -> Any:
        return html.Div(
            [
                html.Span(f"{label}: ", style={"color": theme["muted"]}),
                html.Span(str(value), style={"color": theme.get(tone, theme["text"])}),
            ],
            style={
                "padding": "4px 8px",
                "border": f"1px solid {theme['border']}",
                "background": "#0F151C",
                "fontSize": "10px",
                "borderRadius": "4px",
                "whiteSpace": "nowrap",
            },
        )

    top_shell = html.Div(
        [
            html.Div(
                [
                    html.Div(
                        "Strategy Tester",
                        style={
                            "fontSize": "16px",
                            "fontWeight": "700",
                            "letterSpacing": "0.02em",
                            "color": theme["text"],
                        },
                    ),
                    html.Div(
                        "MetaTrader 5-style tester workspace",
                        style={"color": theme["muted"], "fontSize": "11px", "marginTop": "2px"},
                    ),
                ]
            ),
            html.Div(
                [
                    html.Div(
                        "RegimeFlow",
                        style={
                            "fontSize": "11px",
                            "fontWeight": "700",
                            "letterSpacing": "0.14em",
                            "textTransform": "uppercase",
                            "color": theme["accent"],
                        },
                    ),
                    html.Div(
                        str(setup.get("timestamp", snapshot.get("timestamp", "n/a"))),
                        style={"fontSize": "11px", "color": theme["muted"], "marginTop": "2px", "textAlign": "right"},
                    ),
                ]
            ),
        ],
        style={
            "display": "flex",
            "justifyContent": "space-between",
            "alignItems": "center",
            "gap": "16px",
            "padding": "10px 14px",
            "borderBottom": f"1px solid {theme['border']}",
            "background": "linear-gradient(180deg, rgba(18,25,34,0.98) 0%, rgba(11,15,20,0.98) 100%)",
        },
    )

    control_ribbon = html.Div(
        [
            html.Div(
                [_control_chip(label, value) for label, value in control_bar_items],
                style={"display": "flex", "gap": "8px", "flexWrap": "wrap", "alignItems": "center"},
            ),
            html.Div(
                [
                    _toolbar_button("Start", active=True),
                    _toolbar_button("Pause"),
                    _toolbar_button("Step"),
                    _toolbar_button("Export"),
                ],
                style={"display": "flex", "gap": "8px", "flexWrap": "wrap"},
            ),
        ],
        style={
            "display": "flex",
            "justifyContent": "space-between",
            "alignItems": "center",
            "gap": "12px",
            "padding": "10px 14px",
            "borderBottom": f"1px solid {theme['border']}",
            "background": "rgba(13,18,24,0.98)",
        },
    )

    sidebar_style = {
        "background": "linear-gradient(180deg, rgba(17,24,33,0.98) 0%, rgba(10,14,19,0.98) 100%)",
        "borderRight": f"1px solid {theme['border']}",
        "padding": "12px",
        "display": "grid",
        "gap": "12px",
        "alignContent": "start",
    }
    workspace_card_style = {
        "background": "rgba(12,17,23,0.98)",
        "border": f"1px solid {theme['border']}",
        "borderRadius": "8px",
        "overflow": "hidden",
    }

    left_rail = html.Div(
        [
            _panel(_kv_grid(setup_overview, columns=1), title_text="Settings", subtitle_text="Symbol, mode, and source", padding="12px", min_height="0"),
            _panel(_kv_grid(market_model, columns=1), title_text="Testing", subtitle_text="Model, routing, and drift", padding="12px", min_height="0"),
            _panel(_kv_grid(risk_controls, columns=1), title_text="Risk", subtitle_text="Margin and financing", padding="12px", min_height="0"),
        ],
        style=sidebar_style,
    )

    center_stage = html.Div(
        [
            html.Div(
                [
                    html.Div(
                        [
                            html.Div("Graph", style={"fontSize": "11px", "fontWeight": "700", "textTransform": "uppercase", "letterSpacing": "0.08em", "color": theme["muted"]}),
                            html.Div("Visual mode / candles / execution replay", style={"fontSize": "11px", "color": theme["muted"], "marginTop": "2px"}),
                        ]
                    ),
                    html.Div(
                        [
                            _status_pill("Model", setup.get("tick_mode", "n/a")),
                            _status_pill("Bars", len(market_bars_df.index) if not market_bars_df.empty else "n/a"),
                            _status_pill("Fills", execution.get("fill_count", len(trades_df.index))),
                        ]
                        ,
                        style={"display": "flex", "gap": "6px", "flexWrap": "wrap"},
                    ),
                ],
                style={
                    "display": "flex",
                    "justifyContent": "space-between",
                    "alignItems": "center",
                    "padding": "10px 12px",
                    "borderBottom": f"1px solid {theme['border']}",
                    "background": "rgba(18,24,31,0.96)",
                },
            ),
            dcc.Graph(id="replay-graph", figure=replay_fig, config={"displayModeBar": False}, style={"height": "520px"}),
            html.Div(
                [
                    html.Div(
                        [
                            html.Button("Start", id="replay-start", n_clicks=0, style={"padding": "6px 10px", "border": f"1px solid {theme['accent']}", "background": "#163247", "color": theme["text"], "borderRadius": "6px", "fontSize": "11px"}),
                            html.Button("Pause", id="replay-pause", n_clicks=0, style={"padding": "6px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px", "fontSize": "11px"}),
                            html.Button("Step", id="replay-step", n_clicks=0, style={"padding": "6px 10px", "border": f"1px solid {theme['border']}", "background": "#111821", "color": theme["text"], "borderRadius": "6px", "fontSize": "11px"}),
                        ],
                        style={"display": "flex", "gap": "8px", "alignItems": "center"},
                    ),
                    html.Div(id="replay-status", style={"color": theme["muted"], "fontSize": "11px", "fontVariantNumeric": "tabular-nums"}),
                ],
                style={
                    "display": "flex",
                    "justifyContent": "space-between",
                    "alignItems": "center",
                    "gap": "12px",
                    "padding": "10px 12px 0 12px",
                    "background": "rgba(10,14,19,0.96)",
                },
            ),
            dcc.Slider(
                id="replay-slider",
                min=1,
                max=replay_max_index,
                step=1,
                value=replay_initial_index,
                included=False,
                tooltip={"placement": "bottom", "always_visible": False},
            ),
            html.Div(
                [
                    html.Div(
                        dcc.Graph(figure=equity_fig, config={"displayModeBar": False}, style={"height": "240px"}),
                        style={**workspace_card_style, "minWidth": 0},
                    ),
                    html.Div(
                        dcc.Graph(figure=venue_fig, config={"displayModeBar": False}, style={"height": "240px"}),
                        style={**workspace_card_style, "minWidth": 0},
                    ),
                ],
                style={
                    "display": "grid",
                    "gridTemplateColumns": "1fr 1fr",
                    "gap": "12px",
                    "padding": "12px",
                    "borderTop": f"1px solid {theme['border']}",
                    "background": "rgba(10,14,19,0.96)",
                },
            ),
            html.Div(
                dcc.Graph(figure=regime_fig, config={"displayModeBar": False}, style={"height": "220px"}),
                style={**workspace_card_style, "minWidth": 0, "margin": "0 12px 12px 12px"},
            ),
        ],
        style=workspace_card_style,
    )

    right_rail = html.Div(
        [
            _panel(_metric_grid(headline), title_text="Results", subtitle_text="Core report metrics", padding="12px", min_height="0"),
            _panel(_kv_grid(account), title_text="Account", subtitle_text="Balance, margin, and buying power", padding="12px", min_height="0"),
            _panel(_kv_grid(execution), title_text="Execution", subtitle_text="Fills, orders, and costs", padding="12px", min_height="0"),
            _panel(_kv_grid(right_rail_metrics), title_text="Journal State", subtitle_text="Alerts and runtime state", padding="12px", min_height="0"),
        ],
        style=sidebar_style,
    )

    app = dash.Dash(__name__)
    app.layout = html.Div(
        [
            dcc.Store(id="replay-state", data={"index": replay_initial_index, "playing": False}),
            dcc.Interval(id="replay-interval", interval=250, n_intervals=0, disabled=True),
            html.Div(
                [
                    top_shell,
                    control_ribbon,
                    html.Div(
                        [
                            html.Div(left_rail, style={"minWidth": 0}),
                            html.Div(center_stage, style={"minWidth": 0}),
                            html.Div(right_rail, style={"minWidth": 0}),
                        ],
                        style={
                            "display": "grid",
                            "gridTemplateColumns": "minmax(220px, 0.9fr) minmax(0, 2fr) minmax(240px, 1fr)",
                            "gap": "0",
                            "minHeight": "690px",
                        },
                    ),
                    html.Div(
                        [
                            html.Div(
                                [
                                    html.Div(
                                        "Tester Results",
                                        style={
                                            "fontSize": "11px",
                                            "fontWeight": "700",
                                            "textTransform": "uppercase",
                                            "letterSpacing": "0.14em",
                                            "color": theme["muted"],
                                        },
                                    ),
                                    html.Div(
                                        "Results, graph, trades, orders, optimization and journal views",
                                        style={"fontSize": "11px", "color": theme["muted"], "marginTop": "4px"},
                                    ),
                                ],
                                style={"padding": "10px 12px", "borderBottom": f"1px solid {theme['border']}"},
                            ),
                            dcc.Tabs(
                                tabs,
                                colors={
                                    "border": theme["border"],
                                    "primary": theme["accent"],
                                    "background": "transparent",
                                },
                                parent_style={"background": "transparent"},
                                content_style={"padding": "12px"},
                                style={"background": "transparent"},
                                className="regimeflow-strategy-tester-tabs",
                                vertical=False,
                            ),
                        ],
                        style={**workspace_card_style, "marginTop": "12px"},
                    ),
                    html.Div(
                        [
                            _status_pill("Mode", "Backtest"),
                            _status_pill("Symbol", ", ".join(setup.get("symbols", [])) if setup.get("symbols") else "n/a"),
                            _status_pill("Model", setup.get("tick_mode", "n/a")),
                            _status_pill("Optimization", "on" if optimization["enabled"] else "off"),
                        ],
                        style={
                            "display": "flex",
                            "justifyContent": "flex-start",
                            "flexWrap": "wrap",
                            "gap": "12px",
                            "padding": "8px 12px",
                            "borderTop": f"1px solid {theme['border']}",
                            "background": "rgba(10,14,19,0.98)",
                            "color": theme["muted"],
                        },
                    ),
                ],
                style={
                    "background": "linear-gradient(180deg, rgba(16,22,30,0.99) 0%, rgba(8,11,15,0.99) 100%)",
                    "border": f"1px solid {theme['border']}",
                    "borderRadius": "10px",
                    "overflow": "hidden",
                },
            ),
        ],
        style={
            "padding": "18px",
            "background": "radial-gradient(circle at 12% 8%, #16202B 0%, #0B0F14 42%, #06080B 100%)",
            "minHeight": "100vh",
            "color": theme["text"],
            "fontFamily": FONT_STACK,
        },
    )

    if not market_bars_df.empty:
        @app.callback(
            [
                dash.Output("replay-state", "data"),
                dash.Output("replay-slider", "value"),
                dash.Output("replay-interval", "disabled"),
            ],
            [
                dash.Input("replay-start", "n_clicks"),
                dash.Input("replay-pause", "n_clicks"),
                dash.Input("replay-step", "n_clicks"),
                dash.Input("replay-slider", "value"),
                dash.Input("replay-interval", "n_intervals"),
            ],
            dash.State("replay-state", "data"),
            prevent_initial_call=True,
        )
        def _advance_replay(start_clicks, pause_clicks, step_clicks, slider_value, _n_intervals, state):
            state = dict(state or {"index": replay_initial_index, "playing": False})
            trigger = dash.callback_context.triggered_id
            current_index = int(state.get("index", replay_initial_index))
            playing = bool(state.get("playing", False))

            if trigger == "replay-start":
                playing = True
            elif trigger == "replay-pause":
                playing = False
            elif trigger == "replay-step":
                playing = False
                current_index = min(replay_max_index, current_index + 1)
            elif trigger == "replay-slider" and slider_value is not None:
                playing = False
                current_index = int(slider_value)
            elif trigger == "replay-interval" and playing:
                current_index = min(replay_max_index, current_index + 1)
                if current_index >= replay_max_index:
                    playing = False

            new_state = {"index": current_index, "playing": playing}
            return new_state, current_index, (not playing)

        @app.callback(
            [
                dash.Output("replay-graph", "figure"),
                dash.Output("replay-status", "children"),
            ],
            dash.Input("replay-state", "data"),
            prevent_initial_call=False,
        )
        def _render_replay(state):
            current_index = int((state or {}).get("index", replay_initial_index))
            figure = _build_execution_replay_figure(payload, end_index=current_index, include_animation=False)
            cutoff = market_bars_df.iloc[current_index - 1]["timestamp"]
            status = f"Bar {current_index}/{replay_max_index} - {pd.to_datetime(cutoff).strftime('%Y-%m-%d %H:%M:%S')}"
            return figure, status

    if run:
        app.run_server(host=host, port=port, debug=debug)
    return app


__all__ = [
    "create_strategy_tester_dashboard",
    "create_live_runtime_payload",
    "create_live_dashboard",
    "dashboard_snapshot_to_live_dashboard",
    "_normalize_equity_curve",
    "create_interactive_dashboard",
]
