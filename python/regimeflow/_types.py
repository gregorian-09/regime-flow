"""Structural types shared by the Python convenience layer.

The pybind11 extension does not ship stub classes, so these protocols describe the
small capability surfaces consumed by helpers without coupling them to a concrete
extension implementation.
"""

from __future__ import annotations

from typing import Protocol

import pandas as pd


class SupportsReportJson(Protocol):
    """Object exposing RegimeFlow's JSON report serializer."""

    def report_json(self) -> str: ...


class SupportsReportCsv(Protocol):
    """Object exposing RegimeFlow's CSV report serializer."""

    def report_csv(self) -> str: ...


class SupportsEquityCurve(Protocol):
    """Object exposing an equity-curve dataframe."""

    def equity_curve(self) -> pd.DataFrame: ...


class SupportsTrades(Protocol):
    """Object exposing a trades dataframe."""

    def trades(self) -> pd.DataFrame: ...


class BacktestResultsView(
    SupportsReportJson,
    SupportsReportCsv,
    SupportsEquityCurve,
    SupportsTrades,
    Protocol,
):
    """Capability surface used by report and analytics helpers."""
