import os

import pytest

import regimeflow as rf


def _build_results():
    cfg = rf.BacktestConfig()
    cfg.data_source = "csv"
    cfg.data_config = {
        "data_directory": os.path.join(os.path.dirname(__file__), "..", "..", "tests", "fixtures"),
        "file_pattern": "{symbol}.csv",
        "has_header": True,
    }
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
    return engine.run(NoopStrategy())


def test_plot_results_plotly_or_matplotlib():
    try:
        import plotly  # noqa: F401
    except Exception:
        try:
            import matplotlib  # noqa: F401
        except Exception:
            pytest.skip("plotly/matplotlib not installed")

    results = _build_results()
    from regimeflow.visualization import plot_results

    output = plot_results(results)
    assert "figure" in output
    assert "equity" in output
    assert "drawdown" in output


def test_live_dashboard_basic():
    try:
        import plotly  # noqa: F401
    except Exception:
        try:
            import matplotlib  # noqa: F401
        except Exception:
            pytest.skip("plotly/matplotlib not installed")

    equity = _build_results().equity_curve()
    positions = [
        {"symbol": "TEST", "quantity": 10, "market_value": 1010.0},
    ]
    orders = [
        {"order_id": 1, "symbol": "TEST", "status": "FILLED"},
    ]
    regime_state = {"regime": "NEUTRAL", "confidence": 0.9}

    from regimeflow.visualization import create_live_dashboard

    output = create_live_dashboard(
        equity=equity,
        positions=positions,
        orders=orders,
        regime_state=regime_state,
        alerts=["risk_limit_ok"],
    )
    assert "figure" in output
    assert "equity" in output
    assert "drawdown" in output
    assert "positions" in output
    assert "orders" in output
    assert "regime_state" in output
    assert "alerts" in output


def test_dashboard_snapshot_helper():
    try:
        import plotly  # noqa: F401
    except Exception:
        try:
            import matplotlib  # noqa: F401
        except Exception:
            pytest.skip("plotly/matplotlib not installed")

    snapshot = {
        "equity_curve": [
            {"timestamp": "2024-01-01T00:00:00Z", "equity": 100000.0},
            {"timestamp": "2024-01-02T00:00:00Z", "equity": 101000.0},
        ],
        "positions": [{"symbol": "TEST", "quantity": 5, "market_value": 505.0}],
        "open_orders": [{"order_id": 1, "symbol": "TEST", "status": "NEW"}],
        "current_regime": {"regime": "NEUTRAL", "confidence": 0.8},
        "alerts": ["ok"],
    }

    from regimeflow.visualization import dashboard_snapshot_to_live_dashboard

    output = dashboard_snapshot_to_live_dashboard(snapshot)
    assert "figure" in output
    assert "equity" in output
    assert "drawdown" in output
    assert "positions" in output
    assert "orders" in output
    assert "regime_state" in output
    assert "alerts" in output
