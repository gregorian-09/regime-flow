# RegimeFlow MUST-DO

## Core Tension

The main architectural tension is that RegimeFlow sits between several different product goals:

- research/backtesting framework
- live trading engine
- Python-accessible quant platform
- C++ execution core
- packaged open-source library

Those goals pull in different directions. Research code wants flexibility. Live execution wants safety. Low-latency code wants minimal abstraction. Open-source packaging wants portability. The project must keep those boundaries clean.

## Low-Latency Positioning

RegimeFlow is probably sufficient for Interactive Brokers and retail/professional broker workflows where strategy horizons are seconds, minutes, hours, or regime-level decisions.

It should not be positioned as true HFT or colocated exchange infrastructure. IBKR itself is not a low-latency venue API: TWS/Gateway, socket hops, broker-side routing, and exchange routing dominate latency.

The better target is:

- low overhead
- deterministic behavior
- replay/live parity
- robust risk gates
- predictable order lifecycle
- strong observability
- fast enough for systematic retail/professional workflows

Recommended positioning:

> Production-oriented, regime-aware, execution-safe trading infrastructure.

## Features That Would Make RegimeFlow Stand Out

1. Broker capability matrix -- IMPLEMENTED

Expose what each broker supports: order types, time-in-force values, asset classes, fractional support, shorting, crypto, bracket orders, and rate limits.

Status:

- Implemented `BrokerCapabilities` and `BrokerOrderCapability` in `include/regimeflow/live/broker_adapter.h`.
- Alpaca, Binance, and Interactive Brokers now expose `capabilities()`.
- Existing `supports_tif()` checks now use the capability matrix.
- Added unit coverage in `tests/unit/test_broker_adapter_capabilities.cpp`.
- Documented the capability matrix in `docs/live/production-readiness.md`.

2. Replay-to-live parity -- PARTIALLY IMPLEMENTED

Use the same strategy code, event model, risk checks, and order manager in both backtest and live execution.

Status:

- Added `engine::ReplayJournalWriter` and JSONL event serialization/parsing in `regimeflow/engine/replay_journal.h`.
- Replay journals persist the shared `events::Event` model for market, order, and system events.
- Added `live::to_engine_event(update)` so live market data can be captured in the same event model used by backtests.
- Added unit coverage in `tests/unit/test_replay_journal.cpp`.
- Documented replay journals in `docs/guide/backtesting.md` and `docs/live/overview.md`.

Remaining:

- Add first-class CLI commands to capture a live session and replay a captured journal through `BacktestEngine`.
- Extend capture to order-manager decisions and risk-gate outcomes, not only normalized engine events.

3. Regime-aware risk engine -- PARTIALLY IMPLEMENTED

Let position sizing, drawdown limits, leverage caps, stop behavior, and execution aggressiveness respond to detected regime.

Status:

- Existing `RegimeScaledSizer` supports regime-scaled position sizing.
- Existing `RiskManager::set_regime_limits()` supports explicit per-regime limit sets.
- Added `RegimeRiskOverlayLimit` and `RegimeRiskOverlayProfile` for compact regime metadata-driven controls.
- The overlay can block new exposure, cap per-order notional, and cap projected position percentage by active regime while allowing risk-reducing orders.
- Added coverage in `tests/unit/test_risk_limits.cpp`.
- Documented the overlay in `docs/guide/risk-management.md`.

Remaining:

- Wire regime overlays from config and add execution-aggressiveness controls.

4. Execution quality analytics -- PARTIALLY IMPLEMENTED

Track slippage, queue model behavior, spread cost, rejected orders, cancel/fill latency, and venue comparison.

Status:

- Added `ExecutionQualityTracker`, `ExecutionQualitySnapshot`, and per-report samples in `include/regimeflow/live/execution_quality.h`.
- `LiveOrderManager` now records submitted orders, submit rejections, broker rejects/errors, acknowledgements, partial fills, fills, cancellations, acknowledgement latency, fill latency, and limit/stop-reference slippage in basis points.
- Added unit coverage in `tests/unit/test_execution_quality.cpp`.
- Documented execution-quality tracking in `docs/live/overview.md` and `docs/live/production-readiness.md`.

Remaining:

- Add queue-model/spread-cost attribution and venue-comparison rollups.

5. Operational safety layer

Provide kill switch, stale data detector, duplicate order detector, reconciliation journal, credential hygiene, and audit trails.

Status:

- Implemented fail-closed stale market-data behavior in `LiveTradingEngine::check_heartbeat()`.
- Added `LiveConfig::disable_trading_on_heartbeat_timeout`.
- Added `LiveConfig::cancel_orders_on_heartbeat_timeout`.
- `live.heartbeat.enabled: false` now disables heartbeat checks instead of leaving the default timeout active.
- Added config parsing in `src/tools/live_main.cpp` and `src/tools/live_validation_main.cpp`.
- Documented the new gates in `docs/live/resilience.md` and `docs/live/config.md`.
- Added integration coverage in `tests/unit/test_live_engine_integration.cpp`.
- Added configurable duplicate-order rejection through `LiveOrderManager::set_duplicate_order_window()` and `live.duplicate_order_window_ms`.
- The duplicate guard rejects reused active internal IDs and matching order fingerprints before broker submission.
- Added unit coverage in `tests/unit/test_live_order_reconcile.cpp`.

6. Plugin SDK -- PARTIALLY IMPLEMENTED

Create clean plugin templates for strategies, regime detectors, risk modules, and broker adapters.

Status:

- Added `examples/plugins/template` as a minimal dynamic strategy-plugin SDK template.
- Template includes standalone CMake, lifecycle hooks, required C ABI exports, and runtime config guidance.
- Added `tools/plugins/check_plugin_template.py` to guard required export references.
- Linked the template from `docs/reference/plugin-api.md` and `examples/README.md`.

Remaining:

- Add dedicated templates for regime detectors, risk modules, and broker adapters.

7. Live dry-run / shadow mode -- IMPLEMENTED

Run strategies against live market data without sending orders, then compare would-trade behavior against actual portfolio state.

Status:

- Added `LiveConfig::dry_run_orders`.
- Added `live.dry_run` config parsing in live CLI and validation CLI.
- Dry-run mode keeps strategy, routing, risk, broker-normalization, rate-limit, and audit paths active.
- Dry-run mode suppresses the final broker submit and records a `DryRunOrder` audit event.
- Dry-run orders are cancelled internally after audit so dashboards do not show them as open broker orders.
- Added integration coverage in `tests/unit/test_live_engine_integration.cpp`.
- Documented `live.dry_run` in `docs/live/config.md` and `docs/live/resilience.md`.

8. Model governance -- PARTIALLY IMPLEMENTED

Version regime models, record training data range, feature schema, detector parameters, and runtime predictions.

Status:

- Added `ModelGovernanceMetadata` to the `RegimeDetector` interface.
- `HMMRegimeDetector` now exposes set/get metadata hooks.
- HMM save/load persists detector type, model version, training range, feature schema, and parameter digest.
- HMM config can populate metadata through `model.version`, `model.training_start_us`, `model.training_end_us`, `model.feature_schema`, and `model.parameter_digest`.
- Added persistence coverage in `tests/unit/test_hmm_persistence.cpp`.
- Documented model governance metadata in `docs/guide/regime-detection.md`.

Remaining:

- Attach metadata to runtime prediction audit events and plugin detectors.

9. First-class observability -- PARTIALLY IMPLEMENTED

Add Prometheus metrics, structured logs, dashboard snapshots, and alert hooks.

Status:

- Existing live dashboard snapshots expose portfolio, orders, positions, health, and alert state.
- Added Prometheus text exposition helpers in `include/regimeflow/live/prometheus_exporter.h`.
- Exporter covers dashboard/account gauges plus execution-quality counters, rejection rate, latency, and slippage gauges.
- Added unit coverage in `tests/unit/test_prometheus_exporter.cpp`.
- Documented the helper in `docs/live/overview.md` and `docs/reference/configuration.md`.

Remaining:

- Add an HTTP scrape endpoint and structured-log sink configuration.

10. Security / supply-chain posture -- PARTIALLY IMPLEMENTED

Generate SBOMs, scan dependencies, audit vendored dependencies, and sign release artifacts.

Status:

- Added `tools/security/generate_sbom.py` to emit dependency-free SPDX 2.3 JSON SBOMs.
- SBOMs include project version, git commit, Python dependencies, vcpkg dependencies, and vendored IB API file checksums.
- CI supply-chain gate now generates `build/sbom/regimeflow.spdx.json`.
- `tools/security/check_supply_chain.py` now requires the SBOM generator to remain present.
- Documented SBOM generation in `SECURITY.md`.

Remaining:

- Add vulnerability scanning (`osv-scanner`, `pip-audit`, or equivalent) and artifact signing.

## Interactive Brokers API Update Policy

If the project vendors IB API source directly, updates must be deliberate and reviewable. Do not casually overwrite the vendored tree.

Required process:

1. Track IB API as a vendored dependency -- IMPLEMENTED

Keep it under `third_party/ibapi`, but record:

- upstream version
- download URL
- SHA256 checksum
- local patches
- license file
- protobuf version used

Status:

- Added `third_party/ibapi/VENDOR.md`.
- Added `third_party/ibapi/PATCHES.md`.
- Added `third_party/ibapi/SHA256SUMS`.
- Current vendored IB API version is recorded as `10.42.01`.
- Current bundled protobuf stub runtime expectation is recorded as `3.21.12`.

2. Add vendoring metadata -- IMPLEMENTED

Required files:

```text
third_party/ibapi/VENDOR.md
third_party/ibapi/LICENSE
third_party/ibapi/PATCHES.md
third_party/ibapi/SHA256SUMS
```

3. Automate update checks -- IMPLEMENTED

Add a script:

```bash
tools/vendor/update_ibapi.sh
```

The script should:

- download the official IBKR API archive
- verify a manually reviewed checksum
- replace only required C++ files
- regenerate protobuf files with the pinned protoc version
- apply local patches
- run build/tests

Status:

- Added `tools/vendor/update_ibapi.sh`.
- The script verifies the current vendored tree with `--verify-current`.
- The script requires a manually reviewed archive checksum before applying updates.
- The script rejects unused Java/Python/proto/binary payloads in the vendored scope.
- The script gates bundled protobuf stubs against the pinned protobuf `3.21.12` macro.

4. Avoid blind auto-upgrades

IB API updates can affect:

- message protocol
- order fields
- Decimal/BID math linkage
- protobuf generated files
- platform-specific headers

Use dependency bots for notification if possible, but require manual review before merging.

5. Add compatibility tests -- PARTIALLY IMPLEMENTED

Add tests around:

- order mapping
- status mapping
- Decimal conversion
- contract serialization
- reconnect behavior
- unsupported feature rejection

Status:

- Extended IB adapter tests in `tests/unit/test_broker_adapter_capabilities.cpp`.
- Coverage now verifies concrete IB contract mapping for option overrides.
- Coverage now verifies stop-limit, GTD, MarketOnClose, MarketOnOpen, and Decimal quantity mapping.
- Fixed IB MarketOnClose/MarketOnOpen order mapping so they no longer fall through as generic market orders.

Remaining:

- Add reconnect behavior tests around reader lifecycle and live gateway state transitions.

## Third-Party Security Gating

Security needs several layers.

### Commit-Time Gates

- Require vendored updates to include license, checksum, upstream version, and patch notes.
- Block unknown binary files unless explicitly allowed.
- Keep vendored code minimal.
- Do not vendor Java jars, samples, unrelated clients, or unused language bindings.

Status:

- Added `tools/security/check_supply_chain.py`.
- The checker requires IB vendor metadata and `SECURITY.md`.
- The checker blocks disallowed IB payloads such as JARs, Java/Python clients, source `.proto` files, and shared binary payloads.
- The checker verifies every vendored IB file against `third_party/ibapi/SHA256SUMS`.
- The checker requires Python typing marker `python/regimeflow/py.typed`.

### CI-Time Gates

Add gates for:

- dependency scanning
- SBOM generation
- license scanning
- secret scanning
- CodeQL
- compiler warnings
- sanitizers where possible
- vendored file allowlist checks

Status:

- Added a `supply-chain` job to `.github/workflows/ci.yml`.
- Linux, macOS, and Windows CI jobs now depend on the supply-chain gate.
- The gate validates vendored IB checksums and scope before expensive builds start.
- The gate also enforces pinned GitHub Actions references.
- Pinned remaining docs workflow actions in `.github/workflows/docs.yml`.

Recommended tools:

- `osv-scanner` for known vulnerabilities
- `trivy fs` or `grype` for dependency and filesystem scanning
- GitHub CodeQL for C++/Python
- `pip-audit` for Python dependencies
- `gitleaks` for secrets
- `reuse lint` or another license checker
- `syft` for SBOM generation

### Runtime Gates

Security is not only dependency scanning. Runtime safety controls should include:

- deny remote plaintext IB hosts by default
- require explicit opt-in for unsafe broker endpoints
- redact secrets in logs
- separate paper/live credentials
- fail closed if reconciliation state is inconsistent
- alert on unknown broker order states
- kill switch if market data is stale

## Recommended Direction

Define the project as:

> A regime-aware C++/Python trading platform focused on robust backtest/live parity, broker-safe execution, and operational risk controls -- not HFT.

Priorities:

1. Broker abstraction and capability matrix.
2. IB API vendoring/update policy.
3. Security supply-chain gates.
4. Live/replay parity tests.
5. Observability and auditability.
6. Plugin SDK and examples.

The defensible identity is not being faster than everything. It is being safer, more explainable, more regime-aware, and more production-disciplined than typical research backtesters.
