# RegimeFlow Docs

RegimeFlow is a quantitative trading platform that combines regime detection, backtesting, and live execution across brokers. This docs site is structured so you can start with the big picture, then drill down into how data flows, how decisions are made, and how the system executes orders.

## Recommended Reading Path

If you are new, follow this order for a complete mental model:

1. **What the system is**  
   Start with the product overview and architecture.
   - [Overview](explanation/overview.md)
   - [Architecture](explanation/architecture.md)

2. **How data flows through the system**  
   Understand ingestion, validation, and event propagation.
   - [Data Flow](explanation/data-flow.md)
   - [Event Model](explanation/event-model.md)
   - [Execution Flow](explanation/execution-flow.md)
   - [Order State Machine](explanation/order-state-machine.md)

3. **How regimes influence trading**  
   Learn the regime model, transitions, and feature set, including custom detector plugins and user-defined regime labels.
   - [Regime Detection](explanation/regime-detection.md)
   - [Regime Features](explanation/regime-features.md)
   - [Regime Transitions](explanation/regime-transitions.md)
   - [HMM Math](explanation/hmm-math.md)

4. **How strategies are selected and executed**  
   Understand execution models, costs, and strategy selection.
   - [Strategy Selection](explanation/strategy-selection.md)
   - [Execution Models](explanation/execution-models.md)
   - [Execution Costs](explanation/execution-costs.md)
   - [Slippage Math](explanation/slippage-math.md)

5. **How risk and portfolio accounting work**  
   Review risk policies and portfolio accounting.
   - [Risk Controls](explanation/risk-controls.md)
   - [Risk Limits](explanation/risk-limits-deep.md)
   - [Portfolio Model](explanation/portfolio-model.md)
   - [Performance Metrics](explanation/performance-metrics.md)

6. **How to run backtests and live trading**  
   Hands-on usage and operational behavior.
   - [Examples](tutorials/examples.md)
   - [Data Validation](how-to/data-validation.md)
   - [Live Trading](how-to/live-trading.md)
   - [Dashboard Flow](how-to/dashboard-flow.md)
   - [Live Resiliency](explanation/live-resiliency.md)
   - [Live Order Reconciliation](explanation/live-order-reconciliation.md)

7. **Advanced systems and integrations**  
   Data storage, messaging, and plugins.
   - [Database](how-to/database.md)
   - [Mmap Storage](how-to/mmap-storage.md)
   - [Symbol Metadata](how-to/symbol-metadata.md)
   - [Message Bus](explanation/message-bus.md)
   - [Plugin API](reference/plugin-api.md)
   - [Plugins](reference/plugins.md)

## Quick Entry Points

- **API Reference (JavaDoc-style):** [API Index](api/index.md)
- **Configuration keys:** [Config Reference](reference/config-reference.md)
- **Python API:** [Python Interfaces](reference/python-interfaces.md) and [Python Usage](tutorials/python-usage.md)
- **Broker streaming behavior:** [Broker Streaming](explanation/broker-streaming.md)
- **Audit logging:** [Audit Logging](how-to/audit-logging.md)

## Diagrams

The diagrams live in `diagrams/` and are referenced throughout the docs. If you prefer visuals first:

- [Component Diagram](diagrams/component-diagram.md)
- [Structural Flow](diagrams/structural-flow.md)
- [UML Class Diagram](diagrams/uml-class-diagram.md)
- [Backtest Sequence](diagrams/sequence-backtest.md)
- [Live Sequence](diagrams/sequence-live.md)

## Docs Map (By Type)

### Tutorials
- [Examples](tutorials/examples.md)
- [Python Usage](tutorials/python-usage.md)

### How-to Guides
- [Live Trading](how-to/live-trading.md)
- [Data Validation](how-to/data-validation.md)
- [Database](how-to/database.md)
- [Mmap Storage](how-to/mmap-storage.md)
- [Symbol Metadata](how-to/symbol-metadata.md)
- [Audit Logging](how-to/audit-logging.md)
- [Dashboard Flow](how-to/dashboard-flow.md)

### Reference
- [Config Reference](reference/config-reference.md)
- [Broker Message Types](reference/broker-message-types.md)
- [Python Interfaces](reference/python-interfaces.md)
- [Plugin API](reference/plugin-api.md)
- [Plugins](reference/plugins.md)
- [API Index](api/index.md)

### Explanation
- [Overview](explanation/overview.md)
- [Architecture](explanation/architecture.md)
- [Data Flow](explanation/data-flow.md)
- [Execution Flow](explanation/execution-flow.md)
- [Event Model](explanation/event-model.md)
- [Order State Machine](explanation/order-state-machine.md)
- [Live Resiliency](explanation/live-resiliency.md)
- [Live Order Reconciliation](explanation/live-order-reconciliation.md)
- [Broker Streaming](explanation/broker-streaming.md)
- [Regime Detection](explanation/regime-detection.md)
- [Regime Features](explanation/regime-features.md)
- [Regime Transitions](explanation/regime-transitions.md)
- [HMM Math](explanation/hmm-math.md)
- [Strategy Selection](explanation/strategy-selection.md)
- [Risk Controls](explanation/risk-controls.md)
- [Risk Limits](explanation/risk-limits-deep.md)
- [Execution Models](explanation/execution-models.md)
- [Execution Costs](explanation/execution-costs.md)
- [Slippage Math](explanation/slippage-math.md)
- [Portfolio Model](explanation/portfolio-model.md)
- [Performance Metrics](explanation/performance-metrics.md)
- [Backtest Methodology](explanation/backtest-methodology.md)
- [Walk-Forward](explanation/walk-forward.md)
- [Message Bus](explanation/message-bus.md)

### Diagrams
- [Component Diagram](diagrams/component-diagram.md)
- [Structural Flow](diagrams/structural-flow.md)
- [UML Class Diagram](diagrams/uml-class-diagram.md)
- [Backtest Sequence](diagrams/sequence-backtest.md)
- [Live Sequence](diagrams/sequence-live.md)

## Role-Based Reading Paths

### Quant Developer
- [Quant Workflow](tutorials/quant-workflow.md)
- [Custom Regime Workflow](tutorials/quant-custom-regime-workflow.md)
- [Overview](explanation/overview.md)
- [Regime Detection](explanation/regime-detection.md)
- [Regime Features](explanation/regime-features.md)
- [Regime Transitions](explanation/regime-transitions.md)
- [HMM Math](explanation/hmm-math.md)
- [Strategy Selection](explanation/strategy-selection.md)
- [Execution Models](explanation/execution-models.md)
- [Execution Costs](explanation/execution-costs.md)
- [Slippage Math](explanation/slippage-math.md)
- [Performance Metrics](explanation/performance-metrics.md)
- [Backtest Methodology](explanation/backtest-methodology.md)
- [Walk-Forward](explanation/walk-forward.md)
- [Examples](tutorials/examples.md)
- [Data Validation](how-to/data-validation.md)

### Software Engineer / Platform
- [Architecture](explanation/architecture.md)
- [Data Flow](explanation/data-flow.md)
- [Event Model](explanation/event-model.md)
- [Execution Flow](explanation/execution-flow.md)
- [Order State Machine](explanation/order-state-machine.md)
- [Live Resiliency](explanation/live-resiliency.md)
- [Live Order Reconciliation](explanation/live-order-reconciliation.md)
- [Message Bus](explanation/message-bus.md)
- [Database](how-to/database.md)
- [Mmap Storage](how-to/mmap-storage.md)
- [Symbol Metadata](how-to/symbol-metadata.md)
- [Config Reference](reference/config-reference.md)
- [API Index](api/index.md)

### Trading Operations / Risk
- [Risk Controls](explanation/risk-controls.md)
- [Risk Limits](explanation/risk-limits-deep.md)
- [Portfolio Model](explanation/portfolio-model.md)
- [Performance Metrics](explanation/performance-metrics.md)
- [Execution Models](explanation/execution-models.md)
- [Execution Costs](explanation/execution-costs.md)
- [Live Trading](how-to/live-trading.md)
- [Audit Logging](how-to/audit-logging.md)
- [Dashboard Flow](how-to/dashboard-flow.md)
- [Broker Message Types](reference/broker-message-types.md)

### Python / Research
- [Python Usage](tutorials/python-usage.md)
- [Python Interfaces](reference/python-interfaces.md)
- [Python API](api/python.md)
- [Examples](tutorials/examples.md)
- [Performance Metrics](explanation/performance-metrics.md)
- [Regime Detection](explanation/regime-detection.md)
- [Strategy Selection](explanation/strategy-selection.md)
