#include "regimeflow/metrics/performance_metrics.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::metrics {

namespace {

double mean(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return sum / static_cast<double>(values.size());
}

double stddev(const std::vector<double>& values, double mean_value) {
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

double percentile(std::vector<double> values, double alpha) {
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

}  // namespace

PerformanceStats compute_stats(const EquityCurve& curve, double periods_per_year) {
    PerformanceStats stats;
    const auto& equity = curve.equities();
    const auto& timestamps = curve.timestamps();

    if (equity.size() < 2) {
        return stats;
    }

    stats.total_return = (equity.back() - equity.front()) / equity.front();

    double peak = equity.front();
    double max_dd = 0.0;
    for (double value : equity) {
        if (value > peak) {
            peak = value;
        }
        double dd = peak > 0 ? (peak - value) / peak : 0.0;
        if (dd > max_dd) {
            max_dd = dd;
        }
    }
    stats.max_drawdown = max_dd;

    std::vector<double> returns;
    returns.reserve(equity.size() - 1);
    for (size_t i = 1; i < equity.size(); ++i) {
        double prev = equity[i - 1];
        if (prev == 0) {
            returns.push_back(0.0);
        } else {
            returns.push_back((equity[i] - prev) / prev);
        }
    }
    if (!returns.empty()) {
        stats.best_return = *std::max_element(returns.begin(), returns.end());
        stats.worst_return = *std::min_element(returns.begin(), returns.end());
    }

    double avg = mean(returns);
    double vol = stddev(returns, avg);
    stats.volatility = vol * std::sqrt(periods_per_year);

    if (vol > 0) {
        stats.sharpe = (avg / vol) * std::sqrt(periods_per_year);
    }

    std::vector<double> downside;
    downside.reserve(returns.size());
    for (double r : returns) {
        if (r < 0) {
            downside.push_back(r);
        }
    }
    double down_mean = mean(downside);
    double down_vol = stddev(downside, down_mean);
    if (down_vol > 0) {
        stats.sortino = (avg / down_vol) * std::sqrt(periods_per_year);
    }

    if (timestamps.size() >= 2) {
        double years = (timestamps.back() - timestamps.front()).total_seconds() / (365.25 * 24 * 3600.0);
        if (years > 0) {
            stats.cagr = std::pow(1.0 + stats.total_return, 1.0 / years) - 1.0;
        }
    }

    if (stats.max_drawdown > 0) {
        stats.calmar = stats.cagr / stats.max_drawdown;
    }

    if (!returns.empty()) {
        double var = percentile(returns, 0.05);
        stats.var_95 = -var;
        double sum = 0.0;
        int count = 0;
        for (double r : returns) {
            if (r <= var) {
                sum += r;
                count += 1;
            }
        }
        if (count > 0) {
            stats.cvar_95 = -(sum / static_cast<double>(count));
        }
    }

    return stats;
}

}  // namespace regimeflow::metrics
