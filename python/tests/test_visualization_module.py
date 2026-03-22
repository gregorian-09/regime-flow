import os
from types import SimpleNamespace
import warnings

import pytest

import regimeflow as rf


pytestmark = pytest.mark.filterwarnings(
    "ignore::DeprecationWarning:dash\\.development\\.base_component"
)


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


def test_strategy_tester_dashboard_helper():
    try:
        import plotly  # noqa: F401
    except Exception:
        try:
            import matplotlib  # noqa: F401
        except Exception:
            pytest.skip("plotly/matplotlib not installed")

    results = _build_results()

    from regimeflow.visualization import create_strategy_tester_dashboard

    output = create_strategy_tester_dashboard(results)
    assert "figure" in output
    assert "drawdown" in output
    assert "setup" in output
    assert "headline" in output
    assert "account_curve" in output
    assert "tabs" in output
    assert "report_sections" in output
    assert "journal" in output
    assert "market_bars" in output
    assert "replay_figure" in output
    assert "Report" in output["tabs"]
    assert output["setup"]["symbols"] == ["TEST"]
    assert "summary" in output["report_sections"]
    assert not output["journal"].empty
    assert "category" in output["journal"].columns
    assert not output["market_bars"].empty


def test_strategy_tester_dashboard_walkforward_helper():
    try:
        import plotly  # noqa: F401
    except Exception:
        try:
            import matplotlib  # noqa: F401
        except Exception:
            pytest.skip("plotly/matplotlib not installed")

    results = _build_results()
    window = SimpleNamespace(
        in_sample_range=("2020-01-01", "2020-01-02"),
        out_of_sample_range=("2020-01-02", "2020-01-03"),
        optimal_params={"lookback": 10, "threshold": 1.5},
        is_fitness=1.2,
        oos_fitness=0.9,
        oos_results=results,
        efficiency_ratio=0.75,
    )
    walkforward = SimpleNamespace(
        windows=[window],
        stitched_oos_results=results,
        param_evolution={"lookback": [10.0], "threshold": [1.5]},
        param_stability_score={"lookback": 0.0, "threshold": 0.0},
        avg_is_sharpe=1.2,
        avg_oos_sharpe=0.9,
        overall_oos_sharpe=0.85,
        avg_efficiency_ratio=0.75,
        potential_overfit=False,
        overfit_diagnosis="",
        regime_consistency_score=0.8,
    )

    from regimeflow.visualization import create_strategy_tester_dashboard

    output = create_strategy_tester_dashboard(walkforward)
    assert output["optimization"]["enabled"] is True
    assert "Optimization" in output["tabs"]
    assert not output["optimization"]["windows"].empty
    assert not output["optimization"]["params"].empty


def test_export_dashboard_html(tmp_path):
    try:
        import plotly  # noqa: F401
    except Exception:
        pytest.skip("plotly not installed")

    results = _build_results()
    output_path = tmp_path / "dashboard.html"

    from regimeflow.visualization import export_dashboard_html

    export_dashboard_html(results, str(output_path))
    html = output_path.read_text(encoding="utf-8")
    assert "RegimeFlow Strategy Tester" in html
    assert "Plotly.newPlot" in html


def test_interactive_dashboard_layout_shell():
    try:
        import dash  # noqa: F401
        import plotly  # noqa: F401
    except Exception:
        pytest.skip("dash/plotly not installed")

    results = _build_results()

    from regimeflow.visualization import create_interactive_dashboard

    with warnings.catch_warnings():
        warnings.filterwarnings(
            "ignore",
            message=r"The dash_table\.DataTable will be removed from the builtin dash components in a future major version\.",
            category=DeprecationWarning,
        )
        app = create_interactive_dashboard(results)
    layout_text = str(app.layout)
    assert "Strategy Tester" in layout_text
    assert "Tester Results" in layout_text
    assert "Settings" in layout_text
    assert "replay-slider" in layout_text


def test_live_dashboard_layout_controls():
    try:
        import dash  # noqa: F401
        import plotly  # noqa: F401
        from dash._utils import to_json
    except Exception:
        pytest.skip("dash/plotly not installed")

    results = _build_results()

    def snapshot_provider():
        return {
            "dashboard_snapshot": results.dashboard_snapshot(),
            "trades": results.trades().to_dict("records"),
        }

    from regimeflow.visualization.dashboard_app import create_live_dash_app

    app = create_live_dash_app(snapshot_provider, advance_step=lambda: False)
    layout_text = str(app.layout)
    assert "Toggle Rail" in layout_text
    assert "Toggle Console" in layout_text
    assert "Balanced" in layout_text
    assert "Low Latency" in layout_text
    assert "High History" in layout_text
    assert "live-profile" in layout_text
    assert "live-cb-latency" in layout_text
    assert "live-zoom-1h" in layout_text
    assert "live-window" in layout_text
    to_json(app.layout)


def test_live_dashboard_entry_factory():
    try:
        import dash  # noqa: F401
        import plotly  # noqa: F401
        from dash._utils import to_json
    except Exception:
        pytest.skip("dash/plotly not installed")

    from regimeflow.visualization.live_dashboard_entry import create_intraday_live_dashboard_app

    app = create_intraday_live_dashboard_app()
    to_json(app.layout)
