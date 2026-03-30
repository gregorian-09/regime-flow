<section class="rf-hero">
  <div class="rf-kicker">Getting Started</div>
  <h1>Build A Working Quant Workflow Fast</h1>
  <p class="rf-lead">
    RegimeFlow is a C++ trading engine with Python bindings for repeatable backtests,
    regime-aware strategy selection, and live execution through broker adapters. The
    fastest route through the docs is designed around how quant developers actually work.
  </p>
  <div class="rf-hero-actions">
    <a class="rf-button rf-button-primary" href="../quickstart/">Run the quickstart</a>
    <a class="rf-button rf-button-secondary" href="../../guide/backtesting/">Study the backtest flow</a>
  </div>
  <div class="rf-feature-list">
    <div class="rf-feature">
      <strong>One event model</strong>
      <span>Backtest and live share the same strategy contract and event pipeline.</span>
    </div>
    <div class="rf-feature">
      <strong>Code-aligned docs</strong>
      <span>Reference pages and examples are expected to match the actual implementation.</span>
    </div>
    <div class="rf-feature">
      <strong>Research to execution</strong>
      <span>Move from local runs to live adapters without changing the conceptual model.</span>
    </div>
  </div>
</section>

## How Quants Typically Move Through RegimeFlow

<div class="rf-grid">
  <div class="rf-card rf-card--motif rf-card--data">
    <span class="rf-icon rf-icon--grid"></span>
    <h3>Start From Data</h3>
    <p>Choose a source, validate the data, and confirm the symbol and time assumptions before strategy logic enters the loop.</p>
  </div>
  <div class="rf-card rf-card--motif rf-card--engine">
    <span class="rf-icon rf-icon--flow"></span>
    <h3>Run The Engine</h3>
    <p>Drive bars, ticks, or books through one event pipeline that preserves strategy semantics across research and execution.</p>
  </div>
  <div class="rf-card rf-card--motif rf-card--risk">
    <span class="rf-icon rf-icon--shield"></span>
    <h3>Apply Regime And Risk</h3>
    <p>Use regime state to influence selection, sizing, risk limits, and execution-cost assumptions instead of treating it as a cosmetic metric.</p>
  </div>
  <div class="rf-card rf-card--motif rf-card--live">
    <span class="rf-icon rf-icon--pulse"></span>
    <h3>Carry It Live</h3>
    <p>Reuse the strategy contract with explicit broker boundaries, reconciliation, audit logging, and readiness constraints.</p>
  </div>
</div>

## System Components

- **Data sources**: CSV, tick CSV, memory, mmap, API, Alpaca, Postgres, or plugin-based.
- **Validation**: schema, bounds, gaps, outliers, and trading hours checks.
- **Event pipeline**: bars, ticks, order books, and system events flow into the engine.
- **Strategies**: built-ins (moving average, pairs, harmonic) and plugins.
- **Regime detection**: constant, HMM, ensemble, or plugins.
- **Execution**: slippage, commission, market impact, and latency models.
- **Risk**: limits, stop-loss, and regime-aware risk rules.

<div class="rf-band">
  <h2>Fastest Practical Path</h2>
  <div class="rf-grid">
    <div class="rf-card">
      <h3>1. Install</h3>
      <p><a href="../installation/">Installation</a> covers native builds, Python packaging, and environment assumptions.</p>
    </div>
    <div class="rf-card">
      <h3>2. Run A Backtest</h3>
      <p><a href="../quickstart/">Quickstart</a> gets you to a complete backtest and report with minimal setup noise.</p>
    </div>
    <div class="rf-card">
      <h3>3. Adjust Strategy And Regime</h3>
      <p><a href="../../guide/strategies/">Strategies</a> and <a href="../../guide/regime-detection/">Regime Detection</a> explain the major control surfaces.</p>
    </div>
    <div class="rf-card">
      <h3>4. Validate Config</h3>
      <p><a href="../../reference/configuration/">Configuration</a> is the authoritative reference once you move beyond defaults.</p>
    </div>
  </div>
</div>

## Typical Backtest Flow

```mermaid
sequenceDiagram
  participant User
  participant Config
  participant DataSource
  participant Engine
  participant Strategy
  participant Results

  User->>Config: define YAML
  Config->>DataSource: configure data
  Config->>Engine: configure execution, risk, regime
  Engine->>Strategy: initialize
  Engine->>DataSource: load bars/ticks/books
  Engine->>Strategy: on_bar / on_tick / on_order_book
  Strategy->>Engine: submit orders
  Engine->>Results: metrics + report
```

## Key Design Guarantees

- **Consistent pipeline**: the same events and strategy lifecycle are used in backtest and live.
- **Pluggable subsystems**: data sources, execution, and detectors can be extended via plugins.
- **Deterministic testing**: local CSV-based backtests are reproducible by default.

## Where To Go After This Page

<div class="rf-grid">
  <div class="rf-card">
    <h3>Backtesting</h3>
    <p><a href="../../guide/backtesting/">Open the backtesting guide</a> if you want the operational workflow first.</p>
  </div>
  <div class="rf-card">
    <h3>Execution Models</h3>
    <p><a href="../../guide/execution-models/">Open execution models</a> if realism and fill assumptions matter most.</p>
  </div>
  <div class="rf-card">
    <h3>Python Workflow</h3>
    <p><a href="../../python/workflow/">Open the Python workflow</a> if your research loop lives in Python.</p>
  </div>
  <div class="rf-card">
    <h3>Live Boundary</h3>
    <p><a href="../../live/overview/">Open live overview</a> before treating a strategy as operationally ready.</p>
  </div>
</div>

## Glossary

- **Bar**: OHLCV aggregation over a fixed interval.
- **Tick**: a trade or quote update at sub-second resolution.
- **Order book**: Level-2 market depth snapshots.
- **Regime**: a market state label inferred from features.
- **Strategy**: decision logic that produces orders from events and context.
- **Execution model**: logic translating desired trades into fills with costs.

## Next Steps

- `getting-started/installation.md`
- `getting-started/quickstart.md`
- `guide/backtesting.md`
- `reference/configuration.md`
