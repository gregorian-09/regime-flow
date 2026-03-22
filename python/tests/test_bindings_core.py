import os
import sys
import importlib
import pathlib

import pytest

TEST_ROOT = os.environ.get("REGIMEFLOW_TEST_ROOT")
if not TEST_ROOT:
    raise RuntimeError("REGIMEFLOW_TEST_ROOT not set")

build_python = os.path.join(TEST_ROOT, "build", "python")
source_python = os.path.join(TEST_ROOT, "python")
if build_python not in sys.path:
    sys.path.insert(0, build_python)
if source_python not in sys.path:
    sys.path.insert(0, source_python)

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

    account_df = results.account_curve()
    assert not account_df.empty
    assert "buying_power" in account_df.columns
    assert "margin_excess" in account_df.columns

    account_state = results.account_state()
    assert "equity" in account_state
    assert "buying_power" in account_state

    portfolio_df = engine.portfolio.equity_curve()
    assert not portfolio_df.empty
    assert "buying_power" in portfolio_df.columns
    assert "margin_excess" in portfolio_df.columns

    engine_dashboard = engine.dashboard_snapshot()
    assert "headline" in engine_dashboard
    assert "setup" in engine_dashboard
    assert engine_dashboard["setup"]["symbols"] == ["TEST"]

    engine_dashboard_json = engine.dashboard_snapshot_json()
    assert "\"setup\"" in engine_dashboard_json

    engine_terminal = engine.dashboard_terminal(ansi_colors=False)
    assert "REGIMEFLOW STRATEGY TESTER" in engine_terminal

    trades_df = results.trades()
    assert trades_df is not None
    assert "transaction_cost" in trades_df.columns
    assert "venue" in trades_df.columns

    venue_df = results.venue_fill_summary()
    assert not venue_df.empty
    assert "venue" in venue_df.columns
    assert "total_cost" in venue_df.columns

    dashboard = results.dashboard_snapshot()
    assert "setup" in dashboard
    assert "headline" in dashboard
    assert "account" in dashboard
    assert "execution" in dashboard
    assert "venue_summary" in dashboard
    assert dashboard["setup"]["symbols"] == ["TEST"]
    assert dashboard["setup"]["start_date"] == "2020-01-01"
    assert dashboard["setup"]["end_date"] == "2020-01-03"
    assert dashboard["setup"]["bar_type"] == "1d"
    assert "equity" in dashboard["headline"]
    assert "total_cost" in dashboard["execution"]

    dashboard_json = results.dashboard_snapshot_json()
    assert "\"headline\"" in dashboard_json
    assert "\"venue_summary\"" in dashboard_json

    dashboard_terminal = results.dashboard_terminal(ansi_colors=False)
    assert "REGIMEFLOW STRATEGY TESTER" in dashboard_terminal
    assert "SUMMARY" in dashboard_terminal

    tester_report = results.tester_report()
    assert "headline" in tester_report
    assert "summary" in tester_report
    assert "statistics" in tester_report

    tester_journal = results.tester_journal()
    assert not tester_journal.empty
    assert "category" in tester_journal.columns
    assert "event" in tester_journal.columns
    assert "message" in tester_journal.columns

    regime_metrics = results.regime_metrics()
    assert isinstance(regime_metrics, dict)

    closes = engine.get_close_prices("TEST")
    assert closes.size > 0

    bars = engine.get_bars_array("TEST")
    assert bars.size > 0


def test_backtest_engine_prepare_and_step():
    cfg = _base_config()

    class SimpleStrategy(rf.Strategy):
        def __init__(self):
            super().__init__()
            self._ctx = None
            self._submitted = False

        def initialize(self, ctx):
            self._ctx = ctx

        def on_bar(self, bar):
            if not self._submitted:
                self._ctx.submit_order(
                    rf.Order(
                        symbol="TEST",
                        side=rf.OrderSide.BUY,
                        type=rf.OrderType.MARKET,
                        quantity=1.0,
                    )
                )
                self._submitted = True

    engine = rf.BacktestEngine(cfg)
    engine.prepare(SimpleStrategy())

    assert engine.is_complete() is False

    steps = 0
    while engine.step():
        steps += 1
        snapshot = engine.dashboard_snapshot()
        assert "setup" in snapshot
        assert "headline" in snapshot

    assert steps >= 1
    assert engine.is_complete() is True

    results = engine.results()
    assert not results.equity_curve().empty
    assert not results.tester_journal().empty


def test_backtest_config_accepts_nested_execution_policy():
    candidates = sorted(pathlib.Path(TEST_ROOT, "build", "python").glob("_core*.so"))
    if not candidates:
        candidates = sorted(pathlib.Path(TEST_ROOT, "build", "python").glob("_core*.pyd"))
    assert candidates, "built _core extension not found"

    if build_python in sys.path:
        sys.path.remove(build_python)
    sys.path.insert(0, build_python)
    sys.modules.pop("_core", None)
    core = importlib.import_module("_core")

    cfg = core.engine.BacktestConfig.from_dict(
        {
            "data_source": "csv",
            "data": {
                "data_directory": os.path.join(TEST_ROOT, "tests", "fixtures"),
                "file_pattern": "{symbol}.csv",
                "has_header": True,
            },
            "symbols": ["TEST"],
            "start_date": "2020-01-01",
            "end_date": "2020-01-03",
            "execution": {
                "model": "basic",
                "policy": {
                    "fill": "return",
                    "max_deviation_bps": 25.0,
                    "price_drift_action": "requote",
                },
                "simulation": {
                    "tick_mode": "real_ticks",
                    "synthetic_tick_profile": "ohlc_4tick",
                },
                "session": {
                    "enabled": True,
                    "start_hhmm": "09:30",
                    "end_hhmm": "16:00",
                    "weekdays": ["mon", "tue", "wed", "thu", "fri"],
                    "closed_dates": ["2020-01-01"],
                    "halted_symbols": ["TEST"],
                },
                "queue": {
                    "enabled": True,
                    "aging_fraction": 0.25,
                    "replenishment_fraction": 0.5,
                },
                "account": {
                    "margin": {
                        "initial_margin_ratio": 0.5,
                        "maintenance_margin_ratio": 0.25,
                        "stop_out_margin_level": 0.4,
                    },
                    "enforcement": {
                        "enabled": True,
                        "margin_call_action": "halt_trading",
                        "stop_out_action": "liquidate_worst_first",
                    },
                    "financing": {
                        "enabled": True,
                        "long_rate_bps_per_day": 5.0,
                        "short_borrow_bps_per_day": 12.0,
                    },
                },
            },
        }
    )

    assert cfg.execution_model == "basic"
    assert cfg.execution_params["policy"]["fill"] == "return"
    assert cfg.execution_params["policy"]["max_deviation_bps"] == 25.0
    assert cfg.execution_params["policy"]["price_drift_action"] == "requote"
    assert cfg.execution_params["simulation"]["tick_mode"] == "real_ticks"
    assert cfg.execution_params["simulation"]["synthetic_tick_profile"] == "ohlc_4tick"
    assert cfg.execution_params["session"]["start_hhmm"] == "09:30"
    assert cfg.execution_params["session"]["end_hhmm"] == "16:00"
    assert cfg.execution_params["session"]["weekdays"] == ["mon", "tue", "wed", "thu", "fri"]
    assert cfg.execution_params["session"]["closed_dates"] == ["2020-01-01"]
    assert cfg.execution_params["session"]["halted_symbols"] == ["TEST"]
    assert cfg.execution_params["queue"]["aging_fraction"] == 0.25
    assert cfg.execution_params["queue"]["replenishment_fraction"] == 0.5
    assert cfg.execution_params["account"]["margin"]["initial_margin_ratio"] == 0.5
    assert cfg.execution_params["account"]["margin"]["maintenance_margin_ratio"] == 0.25
    assert cfg.execution_params["account"]["margin"]["stop_out_margin_level"] == 0.4
    assert cfg.execution_params["account"]["enforcement"]["margin_call_action"] == "halt_trading"
    assert cfg.execution_params["account"]["enforcement"]["stop_out_action"] == "liquidate_worst_first"
    assert cfg.execution_params["account"]["financing"]["long_rate_bps_per_day"] == 5.0
    assert cfg.execution_params["account"]["financing"]["short_borrow_bps_per_day"] == 12.0


def test_backtest_config_helper_methods_populate_execution_params():
    cfg = rf.BacktestConfig()
    cfg.set_session_window("09:30", "16:00", open_auction_minutes=2, close_auction_minutes=3)
    cfg.set_session_halts(["TEST", "ALT"], halt_all=False)
    cfg.set_session_calendar(["mon", "tue", "wed", "thu", "fri"], ["2020-01-01"])
    cfg.set_queue_dynamics(
        enabled=True,
        progress_fraction=0.5,
        default_visible_qty=10.0,
        depth_mode="full_depth",
        aging_fraction=0.25,
        replenishment_fraction=0.5,
    )
    cfg.set_account_margin(0.5, 0.25, 0.4)
    cfg.set_account_enforcement(
        enabled=True,
        margin_call_action="halt_trading",
        stop_out_action="liquidate_worst_first",
        halt_after_stop_out=True,
    )
    cfg.set_account_financing(enabled=True, long_rate_bps_per_day=5.0, short_borrow_bps_per_day=12.0)

    assert cfg.execution_params["session"]["start_hhmm"] == "09:30"
    assert cfg.execution_params["session"]["end_hhmm"] == "16:00"
    assert cfg.execution_params["session"]["open_auction_minutes"] == 2
    assert cfg.execution_params["session"]["close_auction_minutes"] == 3
    assert cfg.execution_params["session"]["halted_symbols"] == ["TEST", "ALT"]
    assert cfg.execution_params["session"]["weekdays"] == ["mon", "tue", "wed", "thu", "fri"]
    assert cfg.execution_params["session"]["closed_dates"] == ["2020-01-01"]
    assert cfg.execution_params["queue"]["depth_mode"] == "full_depth"
    assert cfg.execution_params["queue"]["aging_fraction"] == 0.25
    assert cfg.execution_params["queue"]["replenishment_fraction"] == 0.5
    assert cfg.execution_params["account"]["margin"]["initial_margin_ratio"] == 0.5
    assert cfg.execution_params["account"]["enforcement"]["stop_out_action"] == "liquidate_worst_first"
    assert cfg.execution_params["account"]["financing"]["short_borrow_bps_per_day"] == 12.0


def test_backtest_config_from_yaml_reads_execution_params_block():
    cfg = rf.BacktestConfig.from_yaml(
        os.path.join(TEST_ROOT, "examples", "python_execution_realism", "config.yaml")
    )

    assert cfg.execution_params["simulation"]["tick_mode"] == "synthetic_ticks"
    assert cfg.execution_params["policy"]["price_drift_action"] == "requote"
    assert cfg.execution_params["routing"]["enabled"] is True
    assert cfg.execution_params["routing"]["venues"][0]["name"] == "lit_primary"
