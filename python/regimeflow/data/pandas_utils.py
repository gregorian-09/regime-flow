import json
import pandas as pd
from typing import List, Optional

import regimeflow as rf

Timestamp = rf.Timestamp
Bar = rf.Bar
Tick = rf.Tick
BacktestResults = rf.BacktestResults


def bars_to_dataframe(bars: List[Bar]) -> pd.DataFrame:
    if not bars:
        return pd.DataFrame(columns=["timestamp", "open", "high", "low", "close", "volume"])
    data = {
        "timestamp": [b.timestamp.to_datetime() for b in bars],
        "open": [b.open for b in bars],
        "high": [b.high for b in bars],
        "low": [b.low for b in bars],
        "close": [b.close for b in bars],
        "volume": [b.volume for b in bars],
    }
    df = pd.DataFrame(data)
    return df.set_index("timestamp")


def dataframe_to_bars(df: pd.DataFrame) -> List[Bar]:
    bars: List[Bar] = []
    for timestamp, row in df.iterrows():
        bar = Bar()
        bar.timestamp = Timestamp.from_datetime(timestamp)
        bar.open = float(row["open"])
        bar.high = float(row["high"])
        bar.low = float(row["low"])
        bar.close = float(row["close"])
        bar.volume = int(row["volume"])
        bars.append(bar)
    return bars


def ticks_to_dataframe(ticks: List[Tick]) -> pd.DataFrame:
    if not ticks:
        return pd.DataFrame(columns=["timestamp", "price", "quantity"])
    data = {
        "timestamp": [t.timestamp.to_datetime() for t in ticks],
        "price": [t.price for t in ticks],
        "quantity": [t.quantity for t in ticks],
    }
    df = pd.DataFrame(data)
    return df.set_index("timestamp")


def dataframe_to_ticks(df: pd.DataFrame) -> List[Tick]:
    ticks: List[Tick] = []
    for timestamp, row in df.iterrows():
        tick = Tick()
        tick.timestamp = Timestamp.from_datetime(timestamp)
        tick.price = float(row["price"])
        tick.quantity = float(row["quantity"])
        ticks.append(tick)
    return ticks


def results_to_dataframe(results: BacktestResults) -> dict:
    report = json.loads(results.report_json())
    summary_payload = {}
    if "performance_summary" in report:
        summary_payload.update(report["performance_summary"])
    summary_payload.setdefault("total_return", results.total_return)
    summary_payload.setdefault("sharpe_ratio", results.sharpe_ratio)
    summary_payload.setdefault("sortino_ratio", results.sortino_ratio)
    summary_payload.setdefault("max_drawdown", results.max_drawdown)
    summary_payload.setdefault("win_rate", results.win_rate)
    summary_payload.setdefault("profit_factor", results.profit_factor)
    summary_payload.setdefault("num_trades", results.num_trades)

    return {
        "summary": pd.DataFrame([summary_payload]),
        "equity_curve": results.equity_curve(),
        "trades": results.trades(),
        "regime_metrics": pd.DataFrame(results.regime_metrics()).T,
    }


class DataFrameDataSource:
    def __init__(self, data: dict[str, pd.DataFrame]):
        self.data = data
        self._validate()

    def _validate(self):
        required_cols = {"open", "high", "low", "close", "volume"}
        for symbol, df in self.data.items():
            missing = required_cols - set(df.columns)
            if missing:
                raise ValueError(f"Symbol {symbol} missing columns: {missing}")
            if not isinstance(df.index, pd.DatetimeIndex):
                raise ValueError(f"Symbol {symbol} must have DatetimeIndex")

    def get_bars(self, symbol: str, start: Optional[str] = None,
                 end: Optional[str] = None) -> List[Bar]:
        df = self.data[symbol]
        if start:
            df = df[df.index >= start]
        if end:
            df = df[df.index <= end]
        return dataframe_to_bars(df)
