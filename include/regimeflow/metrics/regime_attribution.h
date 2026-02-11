#pragma once

#include "regimeflow/regime/types.h"

#include <map>

namespace regimeflow::metrics {

struct RegimePerformance {
    double total_return = 0.0;
    double avg_return = 0.0;
    double sharpe = 0.0;
    double max_drawdown = 0.0;
    double time_pct = 0.0;
    int observations = 0;
};

class RegimeAttribution {
public:
    void update(regime::RegimeType regime, double equity_return);

    const std::map<regime::RegimeType, RegimePerformance>& results() const { return results_; }

private:
    struct RegimeStats {
        double total_return = 0.0;
        double sum = 0.0;
        double sum_sq = 0.0;
        double equity = 1.0;
        double peak = 1.0;
        double max_dd = 0.0;
        int observations = 0;
    };

    void rebuild_results();

    std::map<regime::RegimeType, RegimeStats> stats_;
    std::map<regime::RegimeType, RegimePerformance> results_;
    int total_obs_ = 0;
};

}  // namespace regimeflow::metrics
