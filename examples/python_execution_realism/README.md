# Python Execution-Realism Strategy

This example is the canonical reference for the advanced backtesting surface.

It shows how to combine:

- synthetic tick replay from bars (`execution.simulation.*`)
- resting limit orders and stop-limit exits
- fill-policy and price-drift controls (`execution.policy.*`)
- queue progression and maker/taker tagging (`execution.queue.*`)
- smart routing with split children and venue-specific latency/price overrides (`execution.routing.*`)
- maker/taker transaction costs (`execution.transaction_cost.*`)
- account margin, enforcement, and financing (`execution.account.*`)
- account and per-venue diagnostics from `BacktestResults`

## Run

```bash
PYTHONPATH=build/lib:build/python:python .venv/bin/python \
  examples/python_execution_realism/run_python_execution_realism.py \
  --config examples/python_execution_realism/config.yaml
```

## What To Inspect

- `examples/python_execution_realism/config.yaml`
  This shows the full nested `execution_params` block in YAML form.
- `examples/python_execution_realism/run_python_execution_realism.py`
  This shows the same controls through `BacktestConfig` helper methods and a strategy that submits:
  - a passive `LIMIT` entry
  - a fallback `MARKET` entry if the limit never fills
  - a protective `STOP_LIMIT` exit
  - a scheduled `MARKET_ON_CLOSE` exit after multiple sessions

When the run completes, the script prints:

- `results.performance_summary()`
- `results.account_state()`
- `results.venue_fill_summary()`
- `results.trades()`

That output is where you verify venue splits, maker/taker costs, and the margin/financing account state.

## Runtime Halts

Dynamic `TradingHalt` / `TradingResume` events are currently injected from the C++ engine API rather than the Python bindings.

Use this pattern in C++ when you want to pause fills for a symbol mid-backtest:

```cpp
engine.enqueue(regimeflow::events::make_system_event(
    regimeflow::events::SystemEventKind::TradingHalt,
    halt_ts,
    0,
    "BTCUSDT"));

engine.enqueue(regimeflow::events::make_system_event(
    regimeflow::events::SystemEventKind::TradingResume,
    resume_ts,
    0,
    "BTCUSDT"));
```

That runtime halt state layers on top of the static session config shown in the YAML example.
