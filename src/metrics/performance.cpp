#include "regimeflow/metrics/performance.h"

namespace regimeflow::metrics
{
    void EquityCurve::add_point(const Timestamp timestamp, const double equity) {
        timestamps_.push_back(timestamp);
        equities_.push_back(equity);
    }

    double EquityCurve::total_return() const {
        if (equities_.size() < 2 || equities_.front() == 0) {
            return 0.0;
        }
        return (equities_.back() - equities_.front()) / equities_.front();
    }
}  // namespace regimeflow::metrics
