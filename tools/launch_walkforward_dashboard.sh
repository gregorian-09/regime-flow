#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

export PYTHONPATH="build/lib:build/python:python${PYTHONPATH:+:${PYTHONPATH}}"
export REGIMEFLOW_TEST_ROOT="${REGIMEFLOW_TEST_ROOT:-$ROOT_DIR}"
export HOST="${HOST:-127.0.0.1}"
export PORT="${PORT:-8051}"

exec .venv/bin/python - <<'PY'
import os

import regimeflow as rf
from regimeflow.visualization import launch_dashboard


class ParametricReplayStrategy(rf.Strategy):
    def __init__(self, symbol: str, hold_bars: int = 45, limit_offset_bps: float = 8.0) -> None:
        super().__init__()
        self._symbol = symbol
        self._hold_bars = int(hold_bars)
        self._limit_offset_bps = float(limit_offset_bps)
        self._ctx = None
        self._bar_count = 0
        self._entry_id = None
        self._entry_bar = None
        self._position_open = False

    def initialize(self, ctx) -> None:
        self._ctx = ctx

    def on_bar(self, bar) -> None:
        if bar.symbol != self._symbol or self._ctx is None:
            return
        self._bar_count += 1
        position = self._ctx.get_position(self._symbol)
        if self._entry_id is None and position <= 0 and self._bar_count >= 10:
            limit_price = bar.close * (1.0 - self._limit_offset_bps / 10_000.0)
            self._entry_id = self._ctx.submit_order(
                rf.Order(self._symbol, rf.OrderSide.BUY, rf.OrderType.LIMIT, 1.0, limit_price=limit_price)
            )
            return
        if self._position_open and position > 0 and self._entry_bar is not None:
            if self._bar_count - self._entry_bar >= self._hold_bars:
                self._ctx.submit_order(
                    rf.Order(self._symbol, rf.OrderSide.SELL, rf.OrderType.MARKET, float(position))
                )
                self._position_open = False

    def on_fill(self, fill) -> None:
        if fill.symbol != self._symbol:
            return
        if fill.quantity > 0:
            self._position_open = True
            self._entry_bar = self._bar_count
        elif fill.quantity < 0:
            self._entry_id = None
            self._entry_bar = None
            self._position_open = False


cfg = rf.WalkForwardConfig()
cfg.window_type = rf.walkforward.WindowType.ROLLING
cfg.in_sample_days = 2
cfg.out_of_sample_days = 1
cfg.step_days = 1
cfg.optimization_method = rf.walkforward.OptMethod.GRID
cfg.max_trials = 4
cfg.fitness_metric = "sharpe"
cfg.maximize = True
cfg.initial_capital = 100000.0
cfg.bar_type = rf.BarType.TIME_1MIN

hold_bars = rf.ParameterDef()
hold_bars.name = "hold_bars"
hold_bars.type = rf.walkforward.ParamType.INT
hold_bars.min_value = 20
hold_bars.max_value = 40
hold_bars.step = 20

limit_offset = rf.ParameterDef()
limit_offset.name = "limit_offset_bps"
limit_offset.type = rf.walkforward.ParamType.DOUBLE
limit_offset.min_value = 4.0
limit_offset.max_value = 8.0
limit_offset.step = 4.0

params = [hold_bars, limit_offset]

data_source_cfg = {
    "type": "csv",
    "data_directory": "examples/python_custom_regime_ensemble/data_intraday",
    "file_pattern": "{symbol}.csv",
    "has_header": True,
    "datetime_format": "%Y-%m-%d %H:%M:%S",
    "symbols": ["BTCUSDT"],
}

def factory(p):
    return ParametricReplayStrategy("BTCUSDT", hold_bars=p["hold_bars"], limit_offset_bps=p["limit_offset_bps"])

optimizer = rf.WalkForwardOptimizer(cfg)
results = optimizer.optimize(
    params=params,
    strategy_factory=factory,
    data_source=data_source_cfg,
    date_range=("2024-01-01", "2024-01-04"),
)

launch_dashboard(
    results,
    host=os.environ.get("HOST", "127.0.0.1"),
    port=int(os.environ.get("PORT", "8051")),
    debug=False,
)
PY
