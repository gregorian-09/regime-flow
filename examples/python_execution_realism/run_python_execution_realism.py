from __future__ import annotations

import argparse

import regimeflow as rf


class ExecutionRealismStrategy(rf.Strategy):
    def __init__(self, symbol: str, entry_qty: float = 8.0) -> None:
        super().__init__()
        self.symbol = symbol
        self.entry_qty = entry_qty
        self._ctx: rf.StrategyContext | None = None
        self._bar_count = 0
        self._entry_sent = False
        self._fallback_sent = False
        self._protective_stop_id: int | None = None
        self._close_sent = False

    def initialize(self, ctx: rf.StrategyContext) -> None:
        self._ctx = ctx

    def on_bar(self, bar: rf.Bar) -> None:
        if bar.symbol != self.symbol or self._ctx is None:
            return

        self._bar_count += 1
        position = self._ctx.get_position(self.symbol)

        if not self._entry_sent and self._bar_count == 2:
            self._submit_passive_entry(bar.close)
            return

        if not self._fallback_sent and position == 0 and self._bar_count >= 60:
            self._submit_market_fallback()
            return

        if position > 0 and self._protective_stop_id is None:
            self._submit_protective_stop(bar.close, position)
            return

        if position > 0 and not self._close_sent and self._bar_count >= 3000:
            if self._protective_stop_id is not None:
                try:
                    self._ctx.cancel_order(self._protective_stop_id)
                except RuntimeError:
                    pass
            order = rf.Order(
                self.symbol,
                rf.OrderSide.SELL,
                rf.OrderType.MARKET_ON_CLOSE,
                position,
            )
            self._ctx.submit_order(order)
            self._close_sent = True

    def on_fill(self, fill: rf.Fill) -> None:
        if self._ctx is None or fill.symbol != self.symbol:
            return
        if self._ctx.get_position(self.symbol) <= 0:
            return
        if self._protective_stop_id is not None:
            return
        self._submit_protective_stop(fill.price, self._ctx.get_position(self.symbol))

    def _submit_passive_entry(self, reference_price: float) -> None:
        assert self._ctx is not None
        order = rf.Order(
            self.symbol,
            rf.OrderSide.BUY,
            rf.OrderType.LIMIT,
            self.entry_qty,
            limit_price=reference_price * 0.9975,
            tif=rf.TimeInForce.GTC,
        )
        self._ctx.submit_order(order)
        self._entry_sent = True

    def _submit_market_fallback(self) -> None:
        assert self._ctx is not None
        order = rf.Order(
            self.symbol,
            rf.OrderSide.BUY,
            rf.OrderType.MARKET,
            self.entry_qty,
            tif=rf.TimeInForce.GTC,
        )
        self._ctx.submit_order(order)
        self._fallback_sent = True

    def _submit_protective_stop(self, reference_price: float, quantity: float) -> None:
        assert self._ctx is not None
        order = rf.Order(
            self.symbol,
            rf.OrderSide.SELL,
            rf.OrderType.STOP_LIMIT,
            quantity,
            limit_price=reference_price * 0.9390,
            stop_price=reference_price * 0.9400,
            tif=rf.TimeInForce.GTC,
        )
        self._protective_stop_id = self._ctx.submit_order(order)


def apply_runtime_execution_overrides(cfg: rf.BacktestConfig) -> None:
    # Show the helper methods alongside the YAML surface so users can choose either path.
    cfg.set_session_window("00:00", "23:59", open_auction_minutes=1, close_auction_minutes=2)
    cfg.set_session_halts([], halt_all=False)
    cfg.set_session_calendar(
        ["mon", "tue", "wed", "thu", "fri", "sat", "sun"],
        ["2024-01-02"],
    )
    cfg.set_queue_dynamics(
        enabled=True,
        progress_fraction=0.40,
        default_visible_qty=3.0,
        depth_mode="full_depth",
        aging_fraction=0.20,
        replenishment_fraction=0.15,
    )
    cfg.set_account_margin(0.35, 0.20, 0.10)
    cfg.set_account_enforcement(
        enabled=True,
        margin_call_action="cancel_open_orders",
        stop_out_action="liquidate_worst_first",
        halt_after_stop_out=False,
    )
    cfg.set_account_financing(
        enabled=True,
        long_rate_bps_per_day=1.25,
        short_borrow_bps_per_day=6.00,
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Python strategy showing the full backtest execution-realism surface"
    )
    parser.add_argument(
        "--config",
        default="examples/python_execution_realism/config.yaml",
    )
    args = parser.parse_args()

    cfg = rf.BacktestConfig.from_yaml(args.config)
    apply_runtime_execution_overrides(cfg)

    engine = rf.BacktestEngine(cfg)
    strategy = ExecutionRealismStrategy(symbol=cfg.symbols[0])
    results = engine.run(strategy)

    print("Performance Summary")
    print(results.performance_summary())
    print()

    print("Latest Account State")
    print(results.account_state())
    print()

    venue_summary = results.venue_fill_summary()
    print("Venue Fill Summary")
    if venue_summary.empty:
        print("(no fills)")
    else:
        print(venue_summary.to_string(index=False))
    print()

    trades = results.trades()
    print("Trades")
    if trades.empty:
        print("(no trades)")
    else:
        print(trades.tail(10).to_string(index=False))


if __name__ == "__main__":
    main()
