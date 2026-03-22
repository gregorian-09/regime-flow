from __future__ import annotations

import os
from typing import Any, Callable, Mapping

import pandas as pd
import regimeflow as rf

from .dashboard_app import create_live_dash_app


_PROFILE_RUNTIME_DEFAULTS: dict[str, dict[str, int]] = {
    "balanced": {"steps_per_tick": 4, "interval_ms": 800},
    "low_latency": {"steps_per_tick": 1, "interval_ms": 250},
    "high_history": {"steps_per_tick": 6, "interval_ms": 1200},
}


def _normalize_profile_name(value: str) -> str:
    key = str(value or "balanced").strip().lower().replace("-", "_")
    return key if key in _PROFILE_RUNTIME_DEFAULTS else "balanced"


def _runtime_int_env(name: str, default: int) -> int:
    raw = os.environ.get(name)
    if raw is None or not str(raw).strip():
        return int(default)
    return int(raw)


class IntradayDashboardStrategy(rf.Strategy):
    def __init__(self, trade_symbol: str) -> None:
        super().__init__()
        self._trade_symbol = trade_symbol
        self._ctx = None
        self._bar_count = 0
        self._last_close = None
        self._entry_order_id = None
        self._entry_bar = None
        self._pending_side = None
        self._protective_stop_id = None

    def initialize(self, ctx) -> None:
        self._ctx = ctx

    def on_bar(self, bar) -> None:
        if bar.symbol != self._trade_symbol or self._ctx is None:
            return

        self._bar_count += 1
        position = self._ctx.get_position(self._trade_symbol)
        if self._last_close is None:
            self._last_close = bar.close
            return

        momentum_up = bar.close > self._last_close
        desired_side = rf.OrderSide.BUY if momentum_up else rf.OrderSide.SELL

        if self._entry_order_id is not None and self._entry_bar is not None:
            if self._bar_count - self._entry_bar >= 3 and position == 0:
                try:
                    self._ctx.cancel_order(self._entry_order_id)
                except RuntimeError:
                    pass
                market_side = self._pending_side or desired_side
                self._ctx.submit_order(
                    rf.Order(self._trade_symbol, market_side, rf.OrderType.MARKET, 1.0)
                )
                self._entry_order_id = None
                self._entry_bar = self._bar_count

        if position == 0 and self._entry_order_id is None and self._bar_count % 18 == 0:
            limit_multiplier = 0.9994 if desired_side == rf.OrderSide.BUY else 1.0006
            self._entry_order_id = self._ctx.submit_order(
                rf.Order(
                    self._trade_symbol,
                    desired_side,
                    rf.OrderType.LIMIT,
                    1.0,
                    limit_price=bar.close * limit_multiplier,
                )
            )
            self._entry_bar = self._bar_count
            self._pending_side = desired_side
            self._last_close = bar.close
            return

        if position != 0 and self._entry_bar is not None and self._bar_count - self._entry_bar >= 9:
            if self._protective_stop_id is not None:
                try:
                    self._ctx.cancel_order(self._protective_stop_id)
                except RuntimeError:
                    pass
                self._protective_stop_id = None
            exit_side = rf.OrderSide.SELL if position > 0 else rf.OrderSide.BUY
            self._ctx.submit_order(
                rf.Order(self._trade_symbol, exit_side, rf.OrderType.MARKET, float(abs(position)))
            )
            self._entry_order_id = None
            self._pending_side = None
            self._entry_bar = self._bar_count

    def on_fill(self, fill) -> None:
        if fill.symbol != self._trade_symbol or self._ctx is None:
            return
        self._entry_order_id = None
        self._pending_side = None
        self._entry_bar = self._bar_count
        if fill.quantity > 0:
            self._protective_stop_id = self._ctx.submit_order(
                rf.Order(
                    self._trade_symbol,
                    rf.OrderSide.SELL,
                    rf.OrderType.STOP_LIMIT,
                    float(fill.quantity),
                    limit_price=fill.price * 0.9965,
                    stop_price=fill.price * 0.9975,
                )
            )
        elif fill.quantity < 0:
            self._protective_stop_id = None
        self._last_close = fill.price


def _build_intraday_engine() -> rf.BacktestEngine:
    cfg = rf.BacktestConfig.from_yaml("examples/python_execution_realism/config.yaml")
    cfg.data_config = {
        "data_directory": "data/binance_vision/normalized",
        "file_pattern": "{symbol}.csv",
        "has_header": True,
        "datetime_format": "%Y-%m-%d %H:%M:%S",
    }
    cfg.start_date = "2024-01-01"
    cfg.end_date = "2024-01-15"
    cfg.symbols = ["BTCUSDT"]
    engine = rf.BacktestEngine(cfg)
    engine.prepare(IntradayDashboardStrategy(cfg.symbols[0]))
    engine.step()
    return engine


def build_intraday_live_dashboard_runtime() -> tuple[Callable[[], Mapping[str, Any]], Callable[[], bool]]:
    engine = _build_intraday_engine()

    def snapshot_provider() -> Mapping[str, Any]:
        results = engine.results()
        base_snapshot = engine.dashboard_snapshot()
        account_curve = base_snapshot.get("account_curve")
        if isinstance(account_curve, pd.DataFrame):
            account_curve = account_curve.tail(480).copy()
            if isinstance(account_curve.index, pd.DatetimeIndex):
                account_curve = account_curve.reset_index()
                first_column = account_curve.columns[0]
                if first_column != "timestamp":
                    account_curve = account_curve.rename(columns={first_column: "timestamp"})
            else:
                account_curve = account_curve.reset_index(drop=True)
            if "equity" in account_curve.columns:
                equity_series = pd.to_numeric(account_curve["equity"], errors="coerce")
                peak = equity_series.cummax()
                drawdown = (equity_series / peak) - 1.0
                drawdown = drawdown.where(peak != 0.0, 0.0).fillna(0.0)
                account_curve["drawdown"] = drawdown
        compact_snapshot = {
            "timestamp": base_snapshot.get("timestamp"),
            "setup": dict(base_snapshot.get("setup", {})),
            "headline": dict(base_snapshot.get("headline", {})),
            "account": dict(base_snapshot.get("account", {})),
            "execution": dict(base_snapshot.get("execution", {})),
            "regime": dict(base_snapshot.get("regime", {})),
            "account_curve": account_curve if account_curve is not None else pd.DataFrame(),
            "positions": list(base_snapshot.get("positions", [])),
            "open_orders": list(base_snapshot.get("open_orders", [])),
            "quote_summary": list(base_snapshot.get("quote_summary", [])),
            "alerts": list(base_snapshot.get("alerts", [])),
            "tester_journal": results.tester_journal().tail(120).reset_index(drop=True),
            "trades": results.trades().tail(80).reset_index(drop=True),
        }
        return compact_snapshot

    def advance_step() -> bool:
        profile_name = _normalize_profile_name(os.environ.get("DASHBOARD_PROFILE", "balanced"))
        defaults = _PROFILE_RUNTIME_DEFAULTS[profile_name]
        steps_per_tick = _runtime_int_env("STEPS_PER_TICK", defaults["steps_per_tick"])
        running = True
        for _ in range(max(1, steps_per_tick)):
            running = engine.step()
            if not running:
                break
        return running

    return snapshot_provider, advance_step


def create_intraday_live_dashboard_app():
    snapshot_provider, advance_step = build_intraday_live_dashboard_runtime()
    profile_name = _normalize_profile_name(os.environ.get("DASHBOARD_PROFILE", "balanced"))
    defaults = _PROFILE_RUNTIME_DEFAULTS[profile_name]
    interval_ms = _runtime_int_env("INTERVAL_MS", defaults["interval_ms"])
    return create_live_dash_app(snapshot_provider, interval_ms=interval_ms, advance_step=advance_step)


def create_intraday_live_dashboard_server():
    return create_intraday_live_dashboard_app().server


__all__ = [
    "IntradayDashboardStrategy",
    "build_intraday_live_dashboard_runtime",
    "create_intraday_live_dashboard_app",
    "create_intraday_live_dashboard_server",
]
