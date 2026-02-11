import os
import sys

TEST_ROOT = os.environ.get("REGIMEFLOW_TEST_ROOT")
if not TEST_ROOT:
    raise RuntimeError("REGIMEFLOW_TEST_ROOT not set")

build_python = os.path.join(TEST_ROOT, "build", "python")
if build_python not in sys.path:
    sys.path.insert(0, build_python)

import pandas as pd
import numpy as np
import pytest

import regimeflow as rf
from regimeflow.data import pandas_utils

pytestmark = pytest.mark.python_native


def test_bars_roundtrip():
    ts = pd.date_range("2020-01-01", periods=3, freq="D")
    df = pd.DataFrame(
        {
            "open": [100.0, 101.0, 102.0],
            "high": [101.0, 102.0, 103.0],
            "low": [99.0, 100.0, 101.0],
            "close": [100.5, 101.5, 102.5],
            "volume": [1000, 1100, 1200],
        },
        index=ts,
    )

    bars = pandas_utils.dataframe_to_bars(df)
    assert len(bars) == 3

    out = pandas_utils.bars_to_dataframe(bars)
    assert not out.empty
    assert list(out.columns) == ["open", "high", "low", "close", "volume"]
    assert np.isclose(out.iloc[0]["open"], 100.0)


def test_results_dataframe_conversion():
    data_config = {
        "data_directory": os.path.join(TEST_ROOT, "tests", "fixtures"),
        "file_pattern": "{symbol}.csv",
        "has_header": True,
    }

    cfg = rf.BacktestConfig()
    cfg.data_source = "csv"
    cfg.data_config = data_config
    cfg.symbols = ["TEST"]
    cfg.start_date = "2020-01-01"
    cfg.end_date = "2020-01-03"
    cfg.bar_type = "1d"
    cfg.initial_capital = 100000.0
    cfg.currency = "USD"
    cfg.regime_detector = "constant"
    cfg.regime_params = {"regime": "neutral"}
    cfg.slippage_model = "zero"
    cfg.commission_model = "zero"

    class NoopStrategy(rf.Strategy):
        def initialize(self, ctx):
            pass

    engine = rf.BacktestEngine(cfg)
    results = engine.run(NoopStrategy())

    frames = pandas_utils.results_to_dataframe(results)
    assert "summary" in frames
    assert "equity_curve" in frames
    assert "trades" in frames
    assert "regime_metrics" in frames

    assert not frames["summary"].empty
    assert not frames["equity_curve"].empty
    summary_columns = set(frames["summary"].columns)
    assert "avg_daily_return" in summary_columns
    assert "avg_monthly_return" in summary_columns
    assert "volatility" in summary_columns
    assert "downside_deviation" in summary_columns
    assert "var_95" in summary_columns
    assert "cvar_95" in summary_columns
    assert "omega_ratio" in summary_columns
    assert "ulcer_index" in summary_columns
    assert "total_trades" in summary_columns
    assert "annual_turnover" in summary_columns
