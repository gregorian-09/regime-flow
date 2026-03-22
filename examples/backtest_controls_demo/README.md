# C++ Backtest Controls Demo

This is the compiled C++ example for runtime backtest controls.

It demonstrates a single order moving through three states:

1. submitted on a configured market-closure date and kept pending
2. still pending while a runtime `TradingHalt` is active for the symbol
3. filled only after a matching `TradingResume`

## Run

```bash
./build/bin/regimeflow_backtest_controls_demo
```

## What It Covers

- `execution.session.weekdays`
- `execution.session.closed_dates`
- runtime `TradingHalt`
- runtime `TradingResume`
- preserving a resting order across blocked execution windows

The demo prints the order status after each phase so the effect is visible without attaching a debugger.
