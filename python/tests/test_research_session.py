import regimeflow as rf


def test_parity_check_dicts():
    backtest = {
        "strategy": {"name": "buy_and_hold"},
        "risk": {"limits": {"max_notional": 100000}},
        "data": {"type": "csv"},
    }
    live = {
        "strategy": {"name": "buy_and_hold"},
        "live": {"risk": {"limits": {"max_notional": 100000}}},
    }

    report = rf.research.parity_check(backtest, live)
    assert report.status in {"pass", "warn"}
    assert report.hard_errors == []


def test_research_session_parity():
    cfg = {
        "strategy": {"name": "buy_and_hold"},
        "risk": {"limits": {"max_notional": 100000}},
        "data": {"type": "csv"},
    }
    session = rf.research.ResearchSession(config=cfg)
    report = session.parity_check(live_config={"strategy": {"name": "buy_and_hold"}})
    assert report.status in {"pass", "warn", "fail"}
