/**
 * @file performance_calculator.h
 * @brief RegimeFlow regimeflow performance calculator declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/regime/types.h"

#include <map>
#include <string>
#include <vector>

namespace regimeflow::metrics {

/**
 * @brief Summary of performance metrics.
 */
struct PerformanceSummary {
    double total_return = 0.0;
    double cagr = 0.0;
    double avg_daily_return = 0.0;
    double avg_monthly_return = 0.0;
    double best_day = 0.0;
    double worst_day = 0.0;
    Timestamp best_day_date;
    Timestamp worst_day_date;
    double best_month = 0.0;
    double worst_month = 0.0;
    Timestamp best_month_date;
    Timestamp worst_month_date;

    double volatility = 0.0;
    double downside_deviation = 0.0;
    double max_drawdown = 0.0;
    Timestamp max_drawdown_start;
    Timestamp max_drawdown_end;
    Duration max_drawdown_duration = Duration::seconds(0);
    double var_95 = 0.0;
    double var_99 = 0.0;
    double cvar_95 = 0.0;

    double sharpe_ratio = 0.0;
    double sortino_ratio = 0.0;
    double calmar_ratio = 0.0;
    double omega_ratio = 0.0;
    double ulcer_index = 0.0;
    double information_ratio = 0.0;
    double treynor_ratio = 0.0;
    double tail_ratio = 0.0;

    int total_trades = 0;
    int winning_trades = 0;
    int losing_trades = 0;
    int open_trades = 0;
    int closed_trades = 0;
    double open_trades_unrealized_pnl = 0.0;
    Timestamp open_trades_snapshot_date;
    double win_rate = 0.0;
    double profit_factor = 0.0;
    double avg_win = 0.0;
    double avg_loss = 0.0;
    double win_loss_ratio = 0.0;
    double expectancy = 0.0;
    double avg_trade_duration_days = 0.0;
    double annual_turnover = 0.0;
};

/**
 * @brief Performance metrics segmented by regime.
 */
struct RegimeMetrics {
    regime::RegimeType regime = regime::RegimeType::Neutral;
    double time_percentage = 0.0;
    PerformanceSummary summary;
    int trade_count = 0;
};

/**
 * @brief Summary metrics for regime transitions.
 */
struct TransitionMetricsSummary {
    regime::RegimeType from = regime::RegimeType::Neutral;
    regime::RegimeType to = regime::RegimeType::Neutral;
    int occurrences = 0;
    double avg_return = 0.0;
    double volatility = 0.0;
    Duration avg_duration = Duration::seconds(0);
};

/**
 * @brief Attribution results for regimes/factors.
 */
struct AttributionResult {
    std::map<regime::RegimeType, double> regime_contribution;
    std::map<std::string, double> factor_contribution;
    double alpha = 0.0;
    double residual = 0.0;
};

/**
 * @brief Computes performance and attribution metrics.
 */
class PerformanceCalculator {
public:
    /**
     * @brief Calculate overall performance summary.
     * @param equity_curve Portfolio equity curve.
     * @param fills Execution fills.
     * @param risk_free_rate Risk-free rate for ratios.
     * @param benchmark_returns Optional benchmark returns.
     * @return Performance summary.
     */
    PerformanceSummary calculate(const std::vector<engine::PortfolioSnapshot>& equity_curve,
                                 const std::vector<engine::Fill>& fills,
                                 double risk_free_rate = 0.0,
                                 const std::vector<double>* benchmark_returns = nullptr);

    /**
     * @brief Calculate performance by regime.
     * @param equity_curve Portfolio equity curve.
     * @param fills Execution fills.
     * @param regimes Regime state timeline.
     * @param risk_free_rate Risk-free rate.
     * @return Map of regime metrics.
     */
    std::map<regime::RegimeType, RegimeMetrics> calculate_by_regime(
        const std::vector<engine::PortfolioSnapshot>& equity_curve,
        const std::vector<engine::Fill>& fills,
        const std::vector<regime::RegimeState>& regimes,
        double risk_free_rate = 0.0);

    /**
     * @brief Calculate metrics for regime transitions.
     * @param equity_curve Portfolio equity curve.
     * @param transitions Regime transitions.
     * @return Transition metrics list.
     */
    std::vector<TransitionMetricsSummary> calculate_transitions(
        const std::vector<engine::PortfolioSnapshot>& equity_curve,
        const std::vector<regime::RegimeTransition>& transitions);

private:

    /**
     * @brief Calculate attribution versus external factors.
     * @param equity_curve Portfolio equity curve.
     * @param regimes Regime state timeline.
     * @param factor_returns Map of factor name to returns series.
     * @return Attribution result.
     */
    AttributionResult calculate_attribution(
        const std::vector<engine::PortfolioSnapshot>& equity_curve,
        const std::vector<regime::RegimeState>& regimes,
        const std::map<std::string, std::vector<double>>& factor_returns);

    /**
     * @brief Compute a regime robustness score from regime metrics.
     * @param regime_metrics Map of regime metrics.
     * @return Robustness score.
     */
    double regime_robustness_score(
        const std::map<regime::RegimeType, RegimeMetrics>& regime_metrics) const;

private:
    /**
     * @brief Aggregated trade summary built from fills.
     */
    struct TradeSummary {
        double pnl = 0.0;
        double notional = 0.0;
        double duration_days = 0.0;
    };

    double compute_periods_per_year(const std::vector<engine::PortfolioSnapshot>& curve) const;
    std::vector<double> compute_returns(const std::vector<engine::PortfolioSnapshot>& curve) const;
    std::map<std::string, std::vector<double>> bucket_returns(
        const std::vector<engine::PortfolioSnapshot>& curve,
        const std::string& format) const;
    std::vector<TradeSummary> build_trades_from_fills(const std::vector<engine::Fill>& fills) const;

    double mean(const std::vector<double>& values) const;
    double stddev(const std::vector<double>& values, double mean_value) const;
    double percentile(std::vector<double> values, double alpha) const;
    double max_drawdown(const std::vector<engine::PortfolioSnapshot>& curve,
                        Timestamp& start, Timestamp& end) const;
};

}  // namespace regimeflow::metrics
