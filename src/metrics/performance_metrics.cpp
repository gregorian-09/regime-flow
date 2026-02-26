#include "regimeflow/metrics/performance_metrics.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::metrics
{
    namespace {

        double mean(const std::vector<double>& values) {
            if (values.empty()) {
                return 0.0;
            }
            double sum = 0.0;
            for (const double v : values) {
                sum += v;
            }
            return sum / static_cast<double>(values.size());
        }

        double stddev(const std::vector<double>& values, const double mean_value) {
            if (values.size() < 2) {
                return 0.0;
            }
            double sum = 0.0;
            for (const double v : values) {
                const double diff = v - mean_value;
                sum += diff * diff;
            }
            return std::sqrt(sum / static_cast<double>(values.size() - 1));
        }

        double percentile(std::vector<double> values, double alpha) {
            if (values.empty()) {
                return 0.0;
            }
            std::ranges::sort(values);
            const double pos = alpha * (static_cast<double>(values.size()) - 1);
            const auto idx = static_cast<size_t>(pos);
            const double frac = pos - static_cast<double>(idx);
            if (idx + 1 < values.size()) {
                return values[idx] * (1.0 - frac) + values[idx + 1] * frac;
            }
            return values.back();
        }

    }  // namespace

    PerformanceStats compute_stats(const EquityCurve& curve, const double periods_per_year) {
        PerformanceStats stats;
        const auto& equity = curve.equities();
        const auto& timestamps = curve.timestamps();

        if (equity.size() < 2) {
            return stats;
        }

        stats.total_return = (equity.back() - equity.front()) / equity.front();

        double peak = equity.front();
        double max_dd = 0.0;
        for (const double value : equity) {
            if (value > peak) {
                peak = value;
            }
            if (const double dd = peak > 0 ? (peak - value) / peak : 0.0; dd > max_dd) {
                max_dd = dd;
            }
        }
        stats.max_drawdown = max_dd;

        std::vector<double> returns;
        returns.reserve(equity.size() - 1);
        for (size_t i = 1; i < equity.size(); ++i) {
            if (const double prev = equity[i - 1]; prev == 0) {
                returns.push_back(0.0);
            } else {
                returns.push_back((equity[i] - prev) / prev);
            }
        }
        if (!returns.empty()) {
            stats.best_return = *std::ranges::max_element(returns);
            stats.worst_return = *std::ranges::min_element(returns);
        }

        const double avg = mean(returns);
        const double vol = stddev(returns, avg);
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
        const double down_mean = mean(downside);
        if (const double down_vol = stddev(downside, down_mean); down_vol > 0) {
            stats.sortino = (avg / down_vol) * std::sqrt(periods_per_year);
        }

        if (timestamps.size() >= 2) {
            if (const double years = static_cast<double>((timestamps.back() - timestamps.front()).total_seconds()) / (365.25 * 24 * 3600.0); years > 0) {
                stats.cagr = std::pow(1.0 + stats.total_return, 1.0 / years) - 1.0;
            }
        }

        if (stats.max_drawdown > 0) {
            stats.calmar = stats.cagr / stats.max_drawdown;
        }

        if (!returns.empty()) {
            const double var = percentile(returns, 0.05);
            stats.var_95 = -var;
            double sum = 0.0;
            int count = 0;
            for (const double r : returns) {
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
