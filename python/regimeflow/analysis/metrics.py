from __future__ import annotations

import json

import pandas as pd

from .._types import SupportsEquityCurve, SupportsReportJson, SupportsTrades


def report_from_results(results: SupportsReportJson) -> dict[str, object]:
    """Return the full report payload as a dict.

    Uses the C++ report serializer via BacktestResults.report_json().
    """
    if not hasattr(results, "report_json"):
        raise AttributeError("results must provide report_json()")
    payload = results.report_json()
    if not payload:
        return {}
    try:
        return json.loads(payload)
    except json.JSONDecodeError:
        return {}


def performance_summary(results: SupportsReportJson) -> dict[str, object]:
    report = report_from_results(results)
    return report.get("performance_summary", {})


def performance_stats(results: SupportsReportJson) -> dict[str, object]:
    report = report_from_results(results)
    return report.get("performance", {})


def regime_performance(results: SupportsReportJson) -> dict[str, object]:
    report = report_from_results(results)
    return report.get("regime_performance", {})


def transition_metrics(results: SupportsReportJson) -> dict[str, object]:
    report = report_from_results(results)
    return report.get("transitions", {})


def equity_curve(results: SupportsEquityCurve) -> pd.DataFrame:
    if not hasattr(results, "equity_curve"):
        raise AttributeError("results must provide equity_curve()")
    return results.equity_curve()


def trades(results: SupportsTrades) -> pd.DataFrame:
    if not hasattr(results, "trades"):
        raise AttributeError("results must provide trades()")
    return results.trades()


def summary_dataframe(results: SupportsReportJson) -> pd.DataFrame:
    summary = performance_summary(results)
    if not summary:
        return pd.DataFrame()
    return pd.DataFrame([summary])


def stats_dataframe(results: SupportsReportJson) -> pd.DataFrame:
    stats = performance_stats(results)
    if not stats:
        return pd.DataFrame()
    return pd.DataFrame([stats])


def regime_dataframe(results: SupportsReportJson) -> pd.DataFrame:
    regimes = regime_performance(results)
    if not regimes:
        return pd.DataFrame()
    rows = []
    for key, value in regimes.items():
        if isinstance(value, dict):
            row = dict(value)
            row["regime"] = key
            rows.append(row)
    return pd.DataFrame(rows)


def transitions_dataframe(results: SupportsReportJson) -> pd.DataFrame:
    transitions = transition_metrics(results)
    if not transitions:
        return pd.DataFrame()
    rows = []
    for key, value in transitions.items():
        if isinstance(value, dict):
            row = dict(value)
            row["transition"] = key
            rows.append(row)
    return pd.DataFrame(rows)


__all__ = [
    "report_from_results",
    "performance_summary",
    "performance_stats",
    "regime_performance",
    "transition_metrics",
    "equity_curve",
    "trades",
    "summary_dataframe",
    "stats_dataframe",
    "regime_dataframe",
    "transitions_dataframe",
]
