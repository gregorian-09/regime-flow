#include "regimeflow/metrics/report.h"

namespace regimeflow::metrics {

namespace {

std::vector<engine::PortfolioSnapshot> to_snapshots(const EquityCurve& curve) {
    std::vector<engine::PortfolioSnapshot> snapshots;
    const auto& ts = curve.timestamps();
    const auto& eq = curve.equities();
    size_t n = std::min(ts.size(), eq.size());
    snapshots.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        engine::PortfolioSnapshot snap;
        snap.timestamp = ts[i];
        snap.equity = eq[i];
        snapshots.push_back(snap);
    }
    return snapshots;
}

}  // namespace

Report build_report(const MetricsTracker& tracker, double periods_per_year) {
    Report report;
    report.performance = compute_stats(tracker.equity_curve(), periods_per_year);
    PerformanceCalculator calculator;
    if (!tracker.portfolio_snapshots().empty()) {
        report.performance_summary = calculator.calculate(tracker.portfolio_snapshots(), {});
    } else {
        auto snapshots = to_snapshots(tracker.equity_curve());
        report.performance_summary = calculator.calculate(snapshots, {});
    }
    report.max_drawdown = tracker.drawdown().max_drawdown();
    report.max_drawdown_start = tracker.drawdown().max_drawdown_start();
    report.max_drawdown_end = tracker.drawdown().max_drawdown_end();
    report.regime_performance = tracker.regime_attribution().results();
    report.transitions = tracker.transition_metrics().results();
    return report;
}

Report build_report(const MetricsTracker& tracker,
                    const std::vector<engine::Fill>& fills,
                    double risk_free_rate,
                    const std::vector<double>* benchmark_returns) {
    Report report = build_report(tracker, 252.0);
    PerformanceCalculator calculator;
    if (!tracker.portfolio_snapshots().empty()) {
        report.performance_summary = calculator.calculate(tracker.portfolio_snapshots(), fills,
                                                          risk_free_rate, benchmark_returns);
    } else {
        auto snapshots = to_snapshots(tracker.equity_curve());
        report.performance_summary = calculator.calculate(snapshots, fills, risk_free_rate,
                                                          benchmark_returns);
    }
    return report;
}

}  // namespace regimeflow::metrics
