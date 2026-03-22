#include "regimeflow/metrics/report.h"

namespace regimeflow::metrics
{
    namespace {

        std::vector<engine::PortfolioSnapshot> to_snapshots(const EquityCurve& curve) {
            std::vector<engine::PortfolioSnapshot> snapshots;
            const auto& ts = curve.timestamps();
            const auto& eq = curve.equities();
            const size_t n = std::min(ts.size(), eq.size());
            snapshots.reserve(n);
            for (size_t i = 0; i < n; ++i) {
                engine::PortfolioSnapshot snap;
                snap.timestamp = ts[i];
                snap.equity = eq[i];
                snapshots.push_back(snap);
            }
            return snapshots;
        }

        double infer_periods_per_year(const std::vector<engine::PortfolioSnapshot>& snapshots) {
            if (snapshots.size() < 2) {
                return 252.0;
            }
            const double total_seconds = static_cast<double>(
                (snapshots.back().timestamp - snapshots.front().timestamp).total_seconds());
            if (total_seconds <= 0.0) {
                return 252.0;
            }
            const double avg_delta = total_seconds / static_cast<double>(snapshots.size() - 1);
            if (avg_delta <= 0.0) {
                return 252.0;
            }
            return (365.25 * 24.0 * 3600.0) / avg_delta;
        }

        std::vector<regime::RegimeTransition> build_transitions(
            const std::vector<regime::RegimeState>& regime_history) {
            std::vector<regime::RegimeTransition> transitions;
            if (regime_history.size() < 2) {
                return transitions;
            }
            for (size_t index = 1; index < regime_history.size(); ++index) {
                const auto& previous = regime_history[index - 1];
                const auto& current = regime_history[index];
                if (previous.regime == current.regime) {
                    continue;
                }
                regime::RegimeTransition transition;
                transition.from = previous.regime;
                transition.to = current.regime;
                transition.timestamp = current.timestamp;
                transition.confidence = current.confidence;
                transition.duration_in_from = static_cast<double>(
                    (current.timestamp - previous.timestamp).total_seconds());
                transitions.emplace_back(transition);
            }
            return transitions;
        }

    }  // namespace

    Report build_report(const MetricsTracker& tracker, const double periods_per_year) {
        Report report;
        constexpr PerformanceCalculator calculator;
        const auto snapshots = !tracker.portfolio_snapshots().empty()
            ? tracker.portfolio_snapshots()
            : to_snapshots(tracker.equity_curve());
        const double inferred_periods_per_year = infer_periods_per_year(snapshots);
        report.performance = compute_stats(tracker.equity_curve(),
                                           periods_per_year > 0.0 ? periods_per_year : inferred_periods_per_year);
        if (!tracker.portfolio_snapshots().empty()) {
            report.performance_summary = calculator.calculate(tracker.portfolio_snapshots(), {});
        } else {
            report.performance_summary = calculator.calculate(snapshots, {});
        }
        report.max_drawdown = tracker.drawdown().max_drawdown();
        report.max_drawdown_start = tracker.drawdown().max_drawdown_start();
        report.max_drawdown_end = tracker.drawdown().max_drawdown_end();
        if (!tracker.regime_history().empty()) {
            auto by_regime = calculator.calculate_by_regime(snapshots, {}, tracker.regime_history());
            for (const auto& [regime, metrics] : by_regime) {
                RegimePerformance performance;
                performance.total_return = metrics.summary.total_return;
                performance.avg_return = metrics.avg_period_return;
                performance.sharpe = metrics.summary.sharpe_ratio;
                performance.max_drawdown = metrics.summary.max_drawdown;
                performance.time_pct = metrics.time_percentage;
                performance.observations = metrics.observations;
                report.regime_performance[regime] = performance;
            }

            const auto transitions = calculator.calculate_transitions(
                snapshots, build_transitions(tracker.regime_history()));
            for (const auto& metrics : transitions) {
                TransitionStats stats;
                stats.avg_return = metrics.avg_return;
                stats.volatility = metrics.volatility;
                stats.observations = metrics.occurrences;
                report.transitions[{metrics.from, metrics.to}] = stats;
            }
        }
        return report;
    }

    Report build_report(const MetricsTracker& tracker,
                        const std::vector<engine::Fill>& fills,
                        const double risk_free_rate,
                        const std::vector<double>* benchmark_returns) {
        const auto snapshots = !tracker.portfolio_snapshots().empty()
            ? tracker.portfolio_snapshots()
            : to_snapshots(tracker.equity_curve());
        Report report = build_report(tracker, infer_periods_per_year(snapshots));
        constexpr PerformanceCalculator calculator;
        if (!tracker.portfolio_snapshots().empty()) {
            report.performance_summary = calculator.calculate(tracker.portfolio_snapshots(), fills,
                                                              risk_free_rate, benchmark_returns);
        } else {
            const auto snapshots = to_snapshots(tracker.equity_curve());
            report.performance_summary = calculator.calculate(snapshots, fills, risk_free_rate,
                                                              benchmark_returns);
        }
        return report;
    }
}  // namespace regimeflow::metrics
