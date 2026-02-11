import os
import sys

import pytest

TEST_ROOT = os.environ.get("REGIMEFLOW_TEST_ROOT")
if not TEST_ROOT:
    raise RuntimeError("REGIMEFLOW_TEST_ROOT not set")

build_python = os.path.join(TEST_ROOT, "build", "python")
if build_python not in sys.path:
    sys.path.insert(0, build_python)

import regimeflow as rf

pytestmark = pytest.mark.python_native


def _base_config():
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
    return cfg


def test_timestamp_roundtrip():
    ts = rf.Timestamp(1_700_000_000_000_000)
    dt = ts.to_datetime()
    ts2 = rf.Timestamp.from_datetime(dt)
    assert abs(ts2.value - ts.value) < 1_000_000


def test_order_symbol_mapping():
    order = rf.Order(symbol="TEST", side=rf.OrderSide.BUY, type=rf.OrderType.MARKET, quantity=1.0)
    assert order.symbol == "TEST"


def test_backtest_engine_run_and_context():
    cfg = _base_config()

    class SimpleStrategy(rf.Strategy):
        def __init__(self):
            super().__init__()
            self.seen = False
            self.order_id = None
            self._ctx = None

        def initialize(self, ctx):
            # basic context access
            assert ctx.current_time() is not None
            self._ctx = ctx

        def on_bar(self, bar):
            if not self.seen:
                self.seen = True
                self.order_id = self._ctx.submit_order(
                    rf.Order(
                        symbol="TEST",
                        side=rf.OrderSide.BUY,
                        type=rf.OrderType.MARKET,
                        quantity=1.0,
                    )
                )

    engine = rf.BacktestEngine(cfg)
    results = engine.run(SimpleStrategy())

    assert results.total_return is not None
    assert results.max_drawdown is not None

    equity_df = results.equity_curve()
    assert not equity_df.empty
    assert "equity" in equity_df.columns

    trades_df = results.trades()
    assert trades_df is not None

    regime_metrics = results.regime_metrics()
    assert isinstance(regime_metrics, dict)

    closes = engine.get_close_prices("TEST")
    assert closes.size > 0

    bars = engine.get_bars_array("TEST")
    assert bars.size > 0
