<section class="rf-hero">
  <div class="rf-kicker">Live Trading</div>
  <h1>Carry Strategy Logic Into A Real Broker Boundary</h1>
  <p class="rf-lead">
    The live engine reuses the same strategy contract and event pipeline as backtests,
    then adds broker adapters, market-data streaming, reconciliation, throttling, risk
    enforcement, and auditability. This section is where research code meets operational reality.
  </p>
  <div class="rf-hero-actions">
    <a class="rf-button rf-button-primary" href="../config/">Configure live trading</a>
    <a class="rf-button rf-button-secondary" href="../production-readiness/">Read readiness boundaries</a>
  </div>
  <div class="rf-feature-list">
    <div class="rf-feature">
      <strong>Shared strategy contract</strong>
      <span>The engine preserves the backtest strategy interface instead of inventing a second runtime model.</span>
    </div>
    <div class="rf-feature">
      <strong>Operational guardrails</strong>
      <span>Risk limits, throttles, reconciliation, and audit events are part of the live surface.</span>
    </div>
    <div class="rf-feature">
      <strong>Broker boundaries are explicit</strong>
      <span>Capabilities, validation level, and external restrictions are documented instead of implied.</span>
    </div>
  </div>
</section>

## What The Live Engine Is Responsible For

<div class="rf-grid">
  <div class="rf-card rf-card--motif rf-card--data">
    <span class="rf-icon rf-icon--grid"></span>
    <h3>Market Data Intake</h3>
    <p>Connect to broker streams, normalize updates into engine events, and keep heartbeat expectations visible.</p>
  </div>
  <div class="rf-card rf-card--motif rf-card--engine">
    <span class="rf-icon rf-icon--flow"></span>
    <h3>Strategy Dispatch</h3>
    <p>Run the same callback contract used by backtests so live logic remains behaviorally traceable.</p>
  </div>
  <div class="rf-card rf-card--motif rf-card--risk">
    <span class="rf-icon rf-icon--shield"></span>
    <h3>Risk Enforcement</h3>
    <p>Apply live risk checks, order throttling, and reconciliation before calling the adapter layer production-ready.</p>
  </div>
  <div class="rf-card rf-card--motif rf-card--live">
    <span class="rf-icon rf-icon--pulse"></span>
    <h3>Audit And Drift</h3>
    <p>Track execution reports, account state, and optional backtest-vs-live performance drift.</p>
  </div>
</div>

## Entry Point

Primary source-build entry point:

```bash
./build/bin/regimeflow_live --config <path>
```

On Windows, use `build\bin\regimeflow_live.exe --config <path>`.

The live CLI loads `.env` automatically if present and merges environment variables with `live.broker_config`.

<div class="rf-band">
  <h2>Operational Checklist</h2>
  <div class="rf-grid">
    <div class="rf-card">
      <h3>1. Configure Broker Inputs</h3>
      <p>Use <a href="../config/">Live Config</a> to set the adapter surface, symbols, risk limits, and broker-specific options.</p>
    </div>
    <div class="rf-card">
      <h3>2. Validate Broker Boundaries</h3>
      <p>Use <a href="../brokers/">Brokers</a> to confirm what is implemented, paper-validated, or externally restricted.</p>
    </div>
    <div class="rf-card">
      <h3>3. Check Readiness</h3>
      <p>Use <a href="../production-readiness/">Production Readiness</a> before treating a configuration as deployable.</p>
    </div>
    <div class="rf-card">
      <h3>4. Review Resilience</h3>
      <p>Use <a href="../resilience/">Resilience</a> to understand reconnect, reconciliation, and operational failure handling.</p>
    </div>
  </div>
</div>

## What The Live Engine Does

- Connects to the broker adapter.
- Subscribes to market data.
- Runs strategy callbacks on incoming events.
- Applies risk limits and order throttling.
- Emits audit logs and system health events.
- Optionally tracks live performance drift vs a backtest baseline.

## Live Performance Drift Tracking

Enable live drift tracking by setting `metrics.live.enable: true`. When enabled, the engine writes:

- `live_drift.csv` for time-series snapshots.
- `live_performance.json` for a summary.

You can also point the tracker at a backtest report JSON via `metrics.live.baseline_report`.

## Read This Section With The Right Mental Model

- Live execution is not just “backtest with sockets”.
- Broker capabilities, IP restrictions, venue permissions, and account state remain external constraints.
- Validation status matters. A working adapter does not automatically imply unrestricted broker access.
- Operational truth comes from reconciled positions, account state, and audit logs, not from a single green startup message.

## Next Steps

- [Live Config](config.md)
- [Brokers](brokers.md)
- [Production Readiness](production-readiness.md)
- [Resilience](resilience.md)
