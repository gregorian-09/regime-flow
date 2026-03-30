<section class="rf-hero">
  <div class="rf-kicker">Quant Engineering Platform</div>
  <h1>RegimeFlow</h1>
  <p>
    A production-oriented quantitative trading platform with a C++ execution core,
    Python bindings, regime-aware strategy infrastructure, and a documentation set
    designed for developers who need exact behavior, not hand-wavy marketing prose.
  </p>
  <div class="rf-hero-actions">
    <a class="rf-button rf-button-primary" href="getting-started/quickstart/">Run A Backtest</a>
    <a class="rf-button rf-button-secondary" href="live/overview/">Inspect Live Trading</a>
  </div>
</section>

<div class="rf-stat-row">
  <div class="rf-stat">
    <span class="rf-stat-label">Core Runtime</span>
    <span class="rf-stat-value">C++ / Python</span>
  </div>
  <div class="rf-stat">
    <span class="rf-stat-label">Primary Focus</span>
    <span class="rf-stat-value">Regime-Aware Research</span>
  </div>
  <div class="rf-stat">
    <span class="rf-stat-label">Execution Surface</span>
    <span class="rf-stat-value">Backtest + Live</span>
  </div>
  <div class="rf-stat">
    <span class="rf-stat-label">Documentation Style</span>
    <span class="rf-stat-value">Reference First</span>
  </div>
</div>

## Why This Documentation Exists

RegimeFlow is built for quant developers who need to move from idea to validated
execution without losing track of assumptions. The documentation is organized so
you can do two things quickly:

- reach a working backtest or live setup fast
- drill into the exact accounting, execution, regime, and broker semantics behind it

<div class="rf-tag-list">
  <span class="rf-tag">Deterministic Backtesting</span>
  <span class="rf-tag">Execution Realism</span>
  <span class="rf-tag">Live Broker Adapters</span>
  <span class="rf-tag">Python Automation</span>
  <span class="rf-tag">API Traceability</span>
</div>

## Fastest Path For Quant Developers

<div class="rf-grid">
  <div class="rf-card">
    <h3>1. Install And Run</h3>
    <p>Get the project running with the shortest path to a real backtest.</p>
    <ul>
      <li><a href="getting-started/installation/">Installation</a></li>
      <li><a href="getting-started/quick-install/">Quick Install</a></li>
      <li><a href="getting-started/quickstart/">Quickstart</a></li>
    </ul>
  </div>
  <div class="rf-card">
    <h3>2. Learn The Quant Flow</h3>
    <p>Understand how data, strategies, regimes, risk, and execution fit together.</p>
    <ul>
      <li><a href="guide/backtesting/">Backtesting</a></li>
      <li><a href="guide/regime-detection/">Regime Detection</a></li>
      <li><a href="guide/execution-models/">Execution Models</a></li>
    </ul>
  </div>
  <div class="rf-card">
    <h3>3. Work In Python</h3>
    <p>Use the bindings for research loops, automation, and dashboard tooling.</p>
    <ul>
      <li><a href="python/overview/">Python Overview</a></li>
      <li><a href="python/workflow/">Python Workflow</a></li>
      <li><a href="tutorials/python-usage/">Python Usage</a></li>
    </ul>
  </div>
  <div class="rf-card">
    <h3>4. Verify The Live Boundary</h3>
    <p>Check broker config, resiliency, and what has been validated versus what is operationally gated.</p>
    <ul>
      <li><a href="live/overview/">Live Overview</a></li>
      <li><a href="live/brokers/">Brokers</a></li>
      <li><a href="live/production-readiness/">Production Readiness</a></li>
    </ul>
  </div>
</div>

<div class="rf-band">
  <h2>Project Map</h2>
  <div class="rf-grid">
    <div class="rf-card">
      <h3>Getting Started</h3>
      <p>Install paths, quickstarts, and the repo layout for first contact.</p>
    </div>
    <div class="rf-card">
      <h3>Quant Guide</h3>
      <p>Core workflows for backtesting, regime detection, risk, execution, and walk-forward analysis.</p>
    </div>
    <div class="rf-card">
      <h3>Reference</h3>
      <p>Configuration keys, runtime flags, plugin contracts, and full API coverage.</p>
    </div>
    <div class="rf-card">
      <h3>Explanation</h3>
      <p>Deeper engineering notes for event flow, portfolio state, risk math, and execution semantics.</p>
    </div>
  </div>
</div>

## Architecture Summary

RegimeFlow keeps backtest and live flows aligned around one event pipeline:

```mermaid
flowchart LR
  A[Data Sources] --> B[Validation and Normalization]
  B --> C[Event Generator]
  C --> D[Strategy Context]
  D --> E[Strategy Logic]
  E --> F[Execution Pipeline]
  F --> G[Portfolio and Metrics]
  C --> H[Regime Detector]
  H --> D
  H --> G
```

## Start Here

<div class="rf-grid">
  <div class="rf-card">
    <h3>Run A Backtest</h3>
    <p><a href="getting-started/quickstart/">Open the quickstart</a> if you want the shortest route from install to report.</p>
  </div>
  <div class="rf-card">
    <h3>See Every Config Knob</h3>
    <p><a href="reference/configuration/">Open the configuration reference</a> when you need exact field behavior.</p>
  </div>
  <div class="rf-card">
    <h3>Work From Python</h3>
    <p><a href="python/overview/">Open the Python overview</a> for bindings, CLI, and workflow entry points.</p>
  </div>
  <div class="rf-card">
    <h3>Understand Live Constraints</h3>
    <p><a href="live/overview/">Open the live overview</a> for broker adapters, readiness boundaries, and runtime expectations.</p>
  </div>
</div>
