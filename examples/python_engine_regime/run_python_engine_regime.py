from __future__ import annotations

import argparse

import regimeflow as rf


class EngineRegimeStrategy(rf.Strategy):
    def __init__(self, symbol: str, max_risk_pct: float = 0.1) -> None:
        super().__init__()
        self.symbol = symbol
        self.max_risk_pct = max_risk_pct

    def initialize(self, ctx: rf.StrategyContext) -> None:
        self._ctx = ctx

    def on_bar(self, bar: rf.Bar) -> None:
        if bar.symbol != self.symbol:
            return

        regime = self._ctx.current_regime()
        if regime is None:
            return

        if regime.regime == rf.RegimeType.CRISIS:
            target_dir = 0
        elif regime.regime == rf.RegimeType.BEAR:
            target_dir = -1
        elif regime.regime == rf.RegimeType.BULL:
            target_dir = 1
        else:
            target_dir = 0

        portfolio = self._ctx.portfolio()
        qty = int((portfolio.equity * self.max_risk_pct) / max(bar.close, 1e-8))
        qty = max(qty, 0)
        target_qty = target_dir * qty

        current_qty = self._ctx.get_position(self.symbol)
        delta = target_qty - current_qty
        if delta == 0:
            return

        side = rf.OrderSide.BUY if delta > 0 else rf.OrderSide.SELL
        order = rf.Order(self.symbol, side, rf.OrderType.MARKET, abs(delta))
        self._ctx.submit_order(order)


def main() -> None:
    parser = argparse.ArgumentParser(description="Python strategy using engine regime detection")
    parser.add_argument("--config", default="examples/python_engine_regime/config.yaml")
    parser.add_argument("--allow-short", action="store_true")
    args = parser.parse_args()

    config = rf.BacktestConfig.from_yaml(args.config)
    if args.allow_short:
        config.strategy_params = {"allow_short": True}

    engine = rf.BacktestEngine(config)
    strategy = EngineRegimeStrategy(symbol=config.symbols[0])
    results = engine.run(strategy)
    print(results.report_json())


if __name__ == "__main__":
    main()
