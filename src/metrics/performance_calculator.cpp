#include "regimeflow/metrics/performance_calculator.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_map>

namespace regimeflow::metrics {

double PerformanceCalculator::mean(const std::vector<double>& values) const {
    if (values.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return sum / static_cast<double>(values.size());
}

double PerformanceCalculator::stddev(const std::vector<double>& values, double mean_value) const {
    if (values.size() < 2) {
        return 0.0;
    }
    double sum = 0.0;
    for (double v : values) {
        double diff = v - mean_value;
        sum += diff * diff;
    }
    return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

double PerformanceCalculator::percentile(std::vector<double> values, double alpha) const {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    double pos = alpha * (values.size() - 1);
    size_t idx = static_cast<size_t>(pos);
    double frac = pos - static_cast<double>(idx);
    if (idx + 1 < values.size()) {
        return values[idx] * (1.0 - frac) + values[idx + 1] * frac;
    }
    return values.back();
}

double PerformanceCalculator::compute_periods_per_year(
    const std::vector<engine::PortfolioSnapshot>& curve) const {
    if (curve.size() < 2) {
        return 252.0;
    }
    double total_seconds = static_cast<double>(
        (curve.back().timestamp - curve.front().timestamp).total_seconds());
    if (total_seconds <= 0.0) {
        return 252.0;
    }
    double avg_delta = total_seconds / static_cast<double>(curve.size() - 1);
    if (avg_delta <= 0.0) {
        return 252.0;
    }
    return (365.25 * 24.0 * 3600.0) / avg_delta;
}

std::vector<double> PerformanceCalculator::compute_returns(
    const std::vector<engine::PortfolioSnapshot>& curve) const {
    std::vector<double> returns;
    if (curve.size() < 2) {
        return returns;
    }
    returns.reserve(curve.size() - 1);
    for (size_t i = 1; i < curve.size(); ++i) {
        double prev = curve[i - 1].equity;
        if (prev == 0.0) {
            returns.push_back(0.0);
        } else {
            returns.push_back((curve[i].equity - prev) / prev);
        }
    }
    return returns;
}

std::map<std::string, std::vector<double>> PerformanceCalculator::bucket_returns(
    const std::vector<engine::PortfolioSnapshot>& curve,
    const std::string& format) const {
    std::map<std::string, std::vector<double>> buckets;
    if (curve.size() < 2) {
        return buckets;
    }
    for (size_t i = 1; i < curve.size(); ++i) {
        auto key = curve[i].timestamp.to_string(format);
        double prev = curve[i - 1].equity;
        double ret = prev == 0.0 ? 0.0 : (curve[i].equity - prev) / prev;
        buckets[key].push_back(ret);
    }
    return buckets;
}

double PerformanceCalculator::max_drawdown(
    const std::vector<engine::PortfolioSnapshot>& curve,
    Timestamp& start, Timestamp& end) const {
    if (curve.empty()) {
        return 0.0;
    }
    double peak = curve.front().equity;
    Timestamp peak_time = curve.front().timestamp;
    double max_dd = 0.0;
    for (const auto& snap : curve) {
        if (snap.equity > peak) {
            peak = snap.equity;
            peak_time = snap.timestamp;
        }
        double dd = peak > 0.0 ? (peak - snap.equity) / peak : 0.0;
        if (dd > max_dd) {
            max_dd = dd;
            start = peak_time;
            end = snap.timestamp;
        }
    }
    return max_dd;
}

std::vector<PerformanceCalculator::TradeSummary>
PerformanceCalculator::build_trades_from_fills(const std::vector<engine::Fill>& fills) const {
    struct Lot {
        double quantity = 0.0;
        double price = 0.0;
        Timestamp timestamp;
        double commission = 0.0;
    };
    std::unordered_map<SymbolId, std::deque<Lot>> open_lots;
    std::vector<TradeSummary> trades;

    for (const auto& fill : fills) {
        if (fill.symbol == 0 || fill.quantity == 0.0) {
            continue;
        }
        auto& lots = open_lots[fill.symbol];
        double qty = fill.quantity;
        double remaining = qty;
        double fill_qty_abs = std::abs(fill.quantity);
        double used_close_commission = 0.0;
        while (!lots.empty() && (remaining * lots.front().quantity) < 0.0) {
            Lot lot = lots.front();
            lots.pop_front();
            double close_qty = std::min(std::abs(remaining), std::abs(lot.quantity));
            double sign = lot.quantity > 0 ? 1.0 : -1.0;
            double open_commission = lot.commission * (close_qty / std::abs(lot.quantity));
            double close_commission = fill_qty_abs > 0.0
                ? fill.commission * (close_qty / fill_qty_abs)
                : 0.0;
            used_close_commission += close_commission;
            double pnl = close_qty * (fill.price - lot.price) * sign - open_commission
                - close_commission;
            double notional = close_qty * lot.price;
            double duration_days = static_cast<double>(
                (fill.timestamp - lot.timestamp).total_seconds()) / (24.0 * 3600.0);
            trades.push_back({pnl, notional, duration_days});
            double new_qty = lot.quantity + close_qty * (lot.quantity > 0 ? -1.0 : 1.0);
            double new_commission = lot.commission - open_commission;
            if (new_qty != 0.0) {
                lots.push_front({new_qty, lot.price, lot.timestamp, new_commission});
            }
            remaining += close_qty * (lot.quantity > 0 ? 1.0 : -1.0);
        }
        if (remaining != 0.0) {
            double remaining_commission = std::max(0.0, fill.commission - used_close_commission);
            lots.push_back({remaining, fill.price, fill.timestamp, remaining_commission});
        }
    }
    return trades;
}

PerformanceSummary PerformanceCalculator::calculate(
    const std::vector<engine::PortfolioSnapshot>& equity_curve,
    const std::vector<engine::Fill>& fills,
    double risk_free_rate,
    const std::vector<double>* benchmark_returns) {
    PerformanceSummary summary;
    if (equity_curve.size() < 2) {
        return summary;
    }

    double periods_per_year = compute_periods_per_year(equity_curve);
    auto returns = compute_returns(equity_curve);
    double avg = mean(returns);
    double vol = stddev(returns, avg);

    summary.total_return =
        (equity_curve.back().equity - equity_curve.front().equity) / equity_curve.front().equity;
    double years = (equity_curve.back().timestamp - equity_curve.front().timestamp).total_seconds()
        / (365.25 * 24.0 * 3600.0);
    if (years > 0) {
        summary.cagr = std::pow(1.0 + summary.total_return, 1.0 / years) - 1.0;
    }

    auto daily = bucket_returns(equity_curve, "%Y-%m-%d");
    std::vector<double> daily_returns;
    for (const auto& [_, vals] : daily) {
        double compounded = 1.0;
        for (double r : vals) {
            compounded *= (1.0 + r);
        }
        daily_returns.push_back(compounded - 1.0);
    }
    summary.avg_daily_return = mean(daily_returns);
    if (!daily_returns.empty()) {
        summary.best_day = *std::max_element(daily_returns.begin(), daily_returns.end());
        summary.worst_day = *std::min_element(daily_returns.begin(), daily_returns.end());
    }

    auto monthly = bucket_returns(equity_curve, "%Y-%m");
    std::vector<double> monthly_returns;
    for (const auto& [_, vals] : monthly) {
        double compounded = 1.0;
        for (double r : vals) {
            compounded *= (1.0 + r);
        }
        monthly_returns.push_back(compounded - 1.0);
    }
    summary.avg_monthly_return = mean(monthly_returns);
    if (!monthly_returns.empty()) {
        summary.best_month = *std::max_element(monthly_returns.begin(), monthly_returns.end());
        summary.worst_month = *std::min_element(monthly_returns.begin(), monthly_returns.end());
    }

    summary.volatility = vol * std::sqrt(periods_per_year);
    double rf_per_period = risk_free_rate / periods_per_year;
    if (vol > 0.0) {
        summary.sharpe_ratio = ((avg - rf_per_period) / vol) * std::sqrt(periods_per_year);
    }

    double downside_sum = 0.0;
    for (double r : returns) {
        double diff = r - rf_per_period;
        if (diff < 0.0) {
            downside_sum += diff * diff;
        }
    }
    double downside_dev = returns.empty()
        ? 0.0
        : std::sqrt(downside_sum / static_cast<double>(returns.size()));
    summary.downside_deviation = downside_dev * std::sqrt(periods_per_year);
    if (downside_dev > 0.0) {
        summary.sortino_ratio = ((avg - rf_per_period) / downside_dev) * std::sqrt(periods_per_year);
    }

    summary.max_drawdown_start = equity_curve.front().timestamp;
    summary.max_drawdown_end = equity_curve.front().timestamp;
    summary.max_drawdown = max_drawdown(equity_curve, summary.max_drawdown_start,
                                        summary.max_drawdown_end);
    summary.max_drawdown_duration = summary.max_drawdown_end - summary.max_drawdown_start;
    if (summary.max_drawdown > 0.0) {
        summary.calmar_ratio = summary.cagr / summary.max_drawdown;
    }

    if (!returns.empty()) {
        double var95 = percentile(returns, 0.05);
        double var99 = percentile(returns, 0.01);
        summary.var_95 = -var95;
        summary.var_99 = -var99;
        double sum = 0.0;
        int count = 0;
        for (double r : returns) {
            if (r <= var95) {
                sum += r;
                count += 1;
            }
        }
        if (count > 0) {
            summary.cvar_95 = -(sum / static_cast<double>(count));
        }
        double p95 = percentile(returns, 0.95);
        double p05 = percentile(returns, 0.05);
        if (p05 != 0.0) {
            summary.tail_ratio = std::abs(p95 / p05);
        }
    }

    std::vector<double> drawdowns;
    double peak = equity_curve.front().equity;
    for (const auto& snap : equity_curve) {
        peak = std::max(peak, snap.equity);
        double dd = peak > 0.0 ? (peak - snap.equity) / peak : 0.0;
        drawdowns.push_back(dd * dd);
    }
    summary.ulcer_index = drawdowns.empty() ? 0.0 : std::sqrt(mean(drawdowns));

    double gain = 0.0;
    double loss = 0.0;
    for (double r : returns) {
        double excess = r - rf_per_period;
        if (excess > 0) gain += excess;
        if (excess < 0) loss += -excess;
    }
    if (loss > 0.0) {
        summary.omega_ratio = gain / loss;
    }

    if (benchmark_returns && benchmark_returns->size() == returns.size()) {
        std::vector<double> active;
        active.reserve(returns.size());
        for (size_t i = 0; i < returns.size(); ++i) {
            active.push_back(returns[i] - (*benchmark_returns)[i]);
        }
        double active_mean = mean(active);
        double tracking_error = stddev(active, active_mean);
        if (tracking_error > 0.0) {
            summary.information_ratio = (active_mean / tracking_error)
                * std::sqrt(periods_per_year);
        }
        double benchmark_mean = mean(*benchmark_returns);
        double cov = 0.0;
        double var = 0.0;
        for (size_t i = 0; i < returns.size(); ++i) {
            cov += (returns[i] - avg) * ((*benchmark_returns)[i] - benchmark_mean);
            var += ((*benchmark_returns)[i] - benchmark_mean)
                * ((*benchmark_returns)[i] - benchmark_mean);
        }
        double beta = var > 0.0 ? cov / var : 0.0;
        if (beta != 0.0) {
            double annualized = (avg * periods_per_year) - risk_free_rate;
            summary.treynor_ratio = annualized / beta;
        }
    }

    auto trades = build_trades_from_fills(fills);
    summary.total_trades = static_cast<int>(trades.size());
    double win_sum = 0.0;
    double loss_sum = 0.0;
    double duration_sum = 0.0;
    for (const auto& trade : trades) {
        duration_sum += trade.duration_days;
        if (trade.pnl >= 0) {
            summary.winning_trades += 1;
            win_sum += trade.pnl;
        } else {
            summary.losing_trades += 1;
            loss_sum += trade.pnl;
        }
    }
    if (summary.total_trades > 0) {
        summary.win_rate = static_cast<double>(summary.winning_trades)
            / static_cast<double>(summary.total_trades);
        summary.avg_trade_duration_days = duration_sum / summary.total_trades;
    }
    if (summary.winning_trades > 0) {
        summary.avg_win = win_sum / summary.winning_trades;
    }
    if (summary.losing_trades > 0) {
        summary.avg_loss = loss_sum / summary.losing_trades;
    }
    if (summary.avg_loss != 0.0) {
        summary.win_loss_ratio = std::abs(summary.avg_win / summary.avg_loss);
    }
    if (loss_sum != 0.0) {
        summary.profit_factor = std::abs(win_sum / loss_sum);
    }
    summary.expectancy = summary.win_rate * summary.avg_win
        - (1.0 - summary.win_rate) * std::abs(summary.avg_loss);

    double total_trade_value = 0.0;
    for (const auto& fill : fills) {
        total_trade_value += std::abs(fill.quantity * fill.price);
    }
    double avg_equity = 0.0;
    for (const auto& snap : equity_curve) {
        avg_equity += snap.equity;
    }
    avg_equity = avg_equity / static_cast<double>(equity_curve.size());
    if (avg_equity > 0.0 && years > 0.0) {
        summary.annual_turnover = (total_trade_value / avg_equity) / years;
    }

    return summary;
}

std::map<regime::RegimeType, RegimeMetrics> PerformanceCalculator::calculate_by_regime(
    const std::vector<engine::PortfolioSnapshot>& equity_curve,
    const std::vector<engine::Fill>& fills,
    const std::vector<regime::RegimeState>& regimes,
    double risk_free_rate) {
    std::map<regime::RegimeType, RegimeMetrics> out;
    if (equity_curve.size() < 2 || regimes.empty()) {
        return out;
    }
    size_t regime_idx = 0;
    std::map<regime::RegimeType, std::vector<engine::PortfolioSnapshot>> curves;
    for (const auto& snap : equity_curve) {
        while (regime_idx + 1 < regimes.size()
               && regimes[regime_idx + 1].timestamp <= snap.timestamp) {
            regime_idx++;
        }
        curves[regimes[regime_idx].regime].push_back(snap);
    }

    std::map<regime::RegimeType, std::vector<engine::Fill>> fills_by_regime;
    regime_idx = 0;
    for (const auto& fill : fills) {
        while (regime_idx + 1 < regimes.size()
               && regimes[regime_idx + 1].timestamp <= fill.timestamp) {
            regime_idx++;
        }
        fills_by_regime[regimes[regime_idx].regime].push_back(fill);
    }

    for (const auto& [regime, curve] : curves) {
        if (curve.size() < 2) {
            continue;
        }
        RegimeMetrics metrics;
        metrics.regime = regime;
        auto it = fills_by_regime.find(regime);
        std::vector<engine::Fill> empty_fills;
        const auto& regime_fills = (it != fills_by_regime.end()) ? it->second : empty_fills;
        metrics.summary = calculate(curve, regime_fills, risk_free_rate);
        metrics.time_percentage = static_cast<double>(curve.size())
            / static_cast<double>(equity_curve.size());
        metrics.trade_count = metrics.summary.total_trades;
        out[regime] = metrics;
    }
    return out;
}

std::vector<TransitionMetricsSummary> PerformanceCalculator::calculate_transitions(
    const std::vector<engine::PortfolioSnapshot>& equity_curve,
    const std::vector<regime::RegimeTransition>& transitions) {
    std::vector<TransitionMetricsSummary> out;
    if (equity_curve.size() < 2 || transitions.empty()) {
        return out;
    }
    struct Aggregate {
        int occurrences = 0;
        double sum_return = 0.0;
        double sum_volatility = 0.0;
        int64_t sum_duration_us = 0;
    };
    std::map<std::pair<regime::RegimeType, regime::RegimeType>, Aggregate> aggregates;
    for (size_t i = 0; i < transitions.size(); ++i) {
        Timestamp start = transitions[i].timestamp;
        Timestamp end = (i + 1 < transitions.size()) ? transitions[i + 1].timestamp
                                                     : equity_curve.back().timestamp;
        std::vector<double> window_returns;
        Duration duration = end - start;
        for (size_t j = 1; j < equity_curve.size(); ++j) {
            if (equity_curve[j].timestamp < start || equity_curve[j].timestamp > end) {
                continue;
            }
            double prev = equity_curve[j - 1].equity;
            double ret = prev == 0.0 ? 0.0 : (equity_curve[j].equity - prev) / prev;
            window_returns.push_back(ret);
        }
        double avg = mean(window_returns);
        double vol = stddev(window_returns, avg);
        auto key = std::make_pair(transitions[i].from, transitions[i].to);
        auto& agg = aggregates[key];
        agg.occurrences += 1;
        agg.sum_return += avg;
        agg.sum_volatility += vol;
        agg.sum_duration_us += duration.total_microseconds();
    }

    for (const auto& [key, agg] : aggregates) {
        TransitionMetricsSummary metrics;
        metrics.from = key.first;
        metrics.to = key.second;
        metrics.occurrences = agg.occurrences;
        metrics.avg_return = agg.occurrences > 0 ? agg.sum_return / agg.occurrences : 0.0;
        metrics.volatility = agg.occurrences > 0 ? agg.sum_volatility / agg.occurrences : 0.0;
        metrics.avg_duration = agg.occurrences > 0
            ? Duration::microseconds(agg.sum_duration_us / agg.occurrences)
            : Duration::seconds(0);
        out.push_back(metrics);
    }
    return out;
}

AttributionResult PerformanceCalculator::calculate_attribution(
    const std::vector<engine::PortfolioSnapshot>& equity_curve,
    const std::vector<regime::RegimeState>& regimes,
    const std::map<std::string, std::vector<double>>& factor_returns) {
    AttributionResult result;
    auto returns = compute_returns(equity_curve);
    if (returns.empty()) {
        return result;
    }
    if (!regimes.empty()) {
        size_t idx = 0;
        std::map<regime::RegimeType, double> sums;
        std::map<regime::RegimeType, int> counts;
        for (size_t i = 1; i < equity_curve.size(); ++i) {
            while (idx + 1 < regimes.size()
                   && regimes[idx + 1].timestamp <= equity_curve[i].timestamp) {
                idx++;
            }
            sums[regimes[idx].regime] += returns[i - 1];
            counts[regimes[idx].regime] += 1;
        }
        std::map<regime::RegimeType, double> contributions;
        for (const auto& [regime, sum] : sums) {
            double time_pct = static_cast<double>(counts[regime])
                / static_cast<double>(returns.size());
            double avg_return = counts[regime] > 0 ? sum / counts[regime] : 0.0;
            contributions[regime] = time_pct * avg_return;
        }
        result.regime_contribution = contributions;
    }

    double avg_return = mean(returns);
    double explained = 0.0;
    std::vector<double> explained_series(returns.size(), 0.0);
    for (const auto& [name, factor] : factor_returns) {
        if (factor.size() != returns.size()) {
            continue;
        }
        double factor_mean = mean(factor);
        double cov = 0.0;
        double var = 0.0;
        for (size_t i = 0; i < factor.size(); ++i) {
            cov += (returns[i] - avg_return) * (factor[i] - factor_mean);
            var += (factor[i] - factor_mean) * (factor[i] - factor_mean);
        }
        double beta = var > 0.0 ? cov / var : 0.0;
        double contribution = beta * factor_mean;
        result.factor_contribution[name] = contribution;
        explained += contribution;
        for (size_t i = 0; i < factor.size(); ++i) {
            explained_series[i] += beta * factor[i];
        }
    }
    result.alpha = avg_return - explained;
    std::vector<double> residuals;
    residuals.reserve(returns.size());
    for (size_t i = 0; i < returns.size(); ++i) {
        residuals.push_back(returns[i] - result.alpha - explained_series[i]);
    }
    result.residual = mean(residuals);
    return result;
}

double PerformanceCalculator::regime_robustness_score(
    const std::map<regime::RegimeType, RegimeMetrics>& regime_metrics) const {
    std::vector<double> sharpes;
    for (const auto& [_, metrics] : regime_metrics) {
        sharpes.push_back(metrics.summary.sharpe_ratio);
    }
    if (sharpes.empty()) {
        return 0.0;
    }
    double avg = mean(sharpes);
    double dev = stddev(sharpes, avg);
    if (avg == 0.0) {
        return 0.0;
    }
    return 1.0 - (dev / avg);
}

}  // namespace regimeflow::metrics
