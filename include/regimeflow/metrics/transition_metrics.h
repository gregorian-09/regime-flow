#pragma once

#include "regimeflow/regime/types.h"

#include <map>
#include <utility>
#include <vector>

namespace regimeflow::metrics {

struct TransitionStats {
    double avg_return = 0.0;
    double volatility = 0.0;
    int observations = 0;
};

class TransitionMetrics {
public:
    void update(regime::RegimeType from, regime::RegimeType to, double equity_return);
    const std::map<std::pair<regime::RegimeType, regime::RegimeType>, TransitionStats>& results() const {
        return results_;
    }

private:
    struct Accumulator {
        std::vector<double> returns;
    };

    void rebuild_results();

    std::map<std::pair<regime::RegimeType, regime::RegimeType>, Accumulator> acc_;
    std::map<std::pair<regime::RegimeType, regime::RegimeType>, TransitionStats> results_;
};

}  // namespace regimeflow::metrics
