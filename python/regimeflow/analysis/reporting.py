from __future__ import annotations

from json import JSONDecodeError, loads

from .._types import SupportsReportCsv, SupportsReportJson


def report_json(results: SupportsReportJson) -> str:
    if not hasattr(results, "report_json"):
        raise AttributeError("results must provide report_json()")
    return results.report_json()


def report_csv(results: SupportsReportCsv) -> str:
    if not hasattr(results, "report_csv"):
        raise AttributeError("results must provide report_csv()")
    return results.report_csv()


def write_report_json(results: SupportsReportJson, path: str) -> str:
    payload = report_json(results)
    with open(path, "w", encoding="utf-8") as f:
        f.write(payload)
    return path


def write_report_csv(results: SupportsReportCsv, path: str) -> str:
    payload = report_csv(results)
    with open(path, "w", encoding="utf-8") as f:
        f.write(payload)
    return path


def report_html(results: SupportsReportJson) -> str:
    try:
        import pandas as pd
    except ImportError as exc:
        raise ImportError("report_html requires pandas") from exc
    try:
        payload = loads(report_json(results))
    except JSONDecodeError:
        payload = {}
    parts = ["<html><body>"]
    if "performance_summary" in payload:
        df = pd.DataFrame([payload["performance_summary"]])
        parts.append("<h2>Performance Summary</h2>")
        parts.append(df.to_html(index=False))
    if "performance" in payload:
        df = pd.DataFrame([payload["performance"]])
        parts.append("<h2>Performance Stats</h2>")
        parts.append(df.to_html(index=False))
    if "regime_performance" in payload:
        df = pd.DataFrame(payload["regime_performance"]).T
        parts.append("<h2>Regime Performance</h2>")
        parts.append(df.to_html())
    if "transitions" in payload:
        df = pd.DataFrame(payload["transitions"]).T
        parts.append("<h2>Transitions</h2>")
        parts.append(df.to_html())
    parts.append("</body></html>")
    return "\n".join(parts)


def write_report_html(results: SupportsReportJson, path: str) -> str:
    payload = report_html(results)
    with open(path, "w", encoding="utf-8") as f:
        f.write(payload)
    return path


__all__ = [
    "report_json",
    "report_csv",
    "report_html",
    "write_report_json",
    "write_report_csv",
    "write_report_html",
]
