from __future__ import annotations

import argparse
from dataclasses import dataclass
from typing import Iterable, List, Tuple

import numpy as np
import regimeflow as rf


@dataclass
class RegimeState:
    label: rf.RegimeType
    confidence: float


class PythonRegimeDetector:
    def __init__(self, window: int = 60, trend_threshold: float = 0.002, vol_threshold: float = 0.02) -> None:
        self.window = window
        self.trend_threshold = trend_threshold
        self.vol_threshold = vol_threshold
        self._returns: List[float] = []
        self._peak: float | None = None
        self._last_close: float | None = None

    def update(self, bar: rf.Bar) -> RegimeState:
        if self._last_close is not None:
            ret = (bar.close - self._last_close) / self._last_close
            self._returns.append(ret)
            if len(self._returns) > self.window:
                self._returns.pop(0)
        self._last_close = bar.close

        if self._peak is None:
            self._peak = bar.close
        else:
            self._peak = max(self._peak, bar.close)

        trend = self._returns[-1] if self._returns else 0.0
        vol = float(np.std(self._returns)) if self._returns else 0.0
        drawdown = 0.0 if self._peak is None else (self._peak - bar.close) / self._peak

        if drawdown > self.vol_threshold:
            return RegimeState(rf.RegimeType.CRISIS, 0.8)
        if trend > self.trend_threshold:
            return RegimeState(rf.RegimeType.BULL, 0.7)
        if trend < -self.trend_threshold:
            return RegimeState(rf.RegimeType.BEAR, 0.7)
        return RegimeState(rf.RegimeType.NEUTRAL, 0.6)


class Signal:
    def compute(self, bars: Iterable[rf.Bar]) -> float:
        raise NotImplementedError


class MomentumSignal(Signal):
    def __init__(self, short: int = 10, long: int = 40) -> None:
        self.short = short
        self.long = long

    def compute(self, bars: Iterable[rf.Bar]) -> float:
        closes = np.array([b.close for b in bars], dtype=float)
        if len(closes) < self.long:
            return 0.0
        short_ma = closes[-self.short :].mean()
        long_ma = closes[-self.long :].mean()
        return float((short_ma - long_ma) / long_ma)


class MeanReversionSignal(Signal):
    def __init__(self, lookback: int = 20) -> None:
        self.lookback = lookback

    def compute(self, bars: Iterable[rf.Bar]) -> float:
        closes = np.array([b.close for b in bars], dtype=float)
        if len(closes) < self.lookback:
            return 0.0
        window = closes[-self.lookback :]
        mean = window.mean()
        std = window.std() or 1e-8
        z = (closes[-1] - mean) / std
        return float(-z)


class BreakoutSignal(Signal):
    def __init__(self, lookback: int = 30) -> None:
        self.lookback = lookback

    def compute(self, bars: Iterable[rf.Bar]) -> float:
        closes = np.array([b.close for b in bars], dtype=float)
        if len(closes) < self.lookback:
            return 0.0
        window = closes[-self.lookback :]
        breakout = (window[-1] - window.min()) / max(window.max() - window.min(), 1e-8)
        return float((breakout - 0.5) * 2.0)


class SignalEnsemble:
    def __init__(self, signals: Iterable[Tuple[Signal, float]]) -> None:
        self.signals = list(signals)

    def score(self, bars: Iterable[rf.Bar]) -> float:
        weighted = 0.0
        weight_sum = 0.0
        for signal, weight in self.signals:
            value = signal.compute(bars)
            weighted += weight * value
            weight_sum += abs(weight)
        if weight_sum == 0.0:
            return 0.0
        return float(np.tanh(weighted / weight_sum))


class RegimeEnsembleStrategy(rf.Strategy):
    def __init__(
        self,
        symbols: list[str],
        lookback: int = 120,
        allow_short: bool = True,
        max_risk_pct: float = 0.02,
        signal_threshold: float = 0.12,
        min_hold_bars: int = 60,
    ) -> None:
        super().__init__()
        self.symbols = symbols
        self.lookback = lookback
        self.allow_short = allow_short
        self.max_risk_pct = max_risk_pct
        self.signal_threshold = signal_threshold
        self.min_hold_bars = min_hold_bars
        self.detectors = {symbol: PythonRegimeDetector(window=lookback) for symbol in symbols}
        self.ensemble = SignalEnsemble(
            [
                (MomentumSignal(short=20, long=80), 0.45),
                (MeanReversionSignal(lookback=30), 0.35),
                (BreakoutSignal(lookback=60), 0.2),
            ]
        )
        self._regime: dict[str, RegimeState] = {
            symbol: RegimeState(rf.RegimeType.NEUTRAL, 0.5) for symbol in symbols
        }
        self._bar_count: dict[str, int] = {symbol: 0 for symbol in symbols}
        self._last_trade_bar: dict[str, int] = {symbol: -10_000 for symbol in symbols}

    def initialize(self, ctx: rf.StrategyContext) -> None:
        self._ctx = ctx

    def on_bar(self, bar: rf.Bar) -> None:
        if bar.symbol not in self.detectors:
            return

        symbol = bar.symbol
        self._bar_count[symbol] += 1
        self._regime[symbol] = self.detectors[symbol].update(bar)
        bars = self._ctx.get_bars(symbol, self.lookback)
        if len(bars) < self.lookback:
            return

        score = self.ensemble.score(bars)
        if self._regime[symbol].label == rf.RegimeType.CRISIS:
            target_dir = 0
        elif self._regime[symbol].label == rf.RegimeType.BEAR:
            target_dir = -1 if self.allow_short else 0
        else:
            if score > self.signal_threshold:
                target_dir = 1
            elif score < -self.signal_threshold and self.allow_short:
                target_dir = -1
            else:
                target_dir = 0

        if self._bar_count[symbol] - self._last_trade_bar[symbol] < self.min_hold_bars:
            return

        portfolio = self._ctx.portfolio()
        qty = int((portfolio.equity * self.max_risk_pct) / max(bar.close, 1e-8))
        qty = max(qty, 1)
        target_qty = target_dir * qty

        current_qty = self._ctx.get_position(symbol)
        delta = target_qty - current_qty
        if delta == 0:
            return

        side = rf.OrderSide.BUY if delta > 0 else rf.OrderSide.SELL
        order = rf.Order(symbol, side, rf.OrderType.MARKET, abs(delta))
        self._ctx.submit_order(order)
        self._last_trade_bar[symbol] = self._bar_count[symbol]


def main() -> None:
    parser = argparse.ArgumentParser(description="Python custom regime + signal ensemble example")
    parser.add_argument("--config", default="examples/python_custom_regime_ensemble/config.yaml")
    args = parser.parse_args()

    config = rf.BacktestConfig.from_yaml(args.config)
    engine = rf.BacktestEngine(config)
    strategy = RegimeEnsembleStrategy(symbols=config.symbols)
    results = engine.run(strategy)
    print(results.report_json())


if __name__ == "__main__":
    main()
