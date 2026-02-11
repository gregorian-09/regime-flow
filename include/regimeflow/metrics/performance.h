#pragma once

#include "regimeflow/common/time.h"

#include <vector>

namespace regimeflow::metrics {

class EquityCurve {
public:
    void add_point(Timestamp timestamp, double equity);

    const std::vector<Timestamp>& timestamps() const { return timestamps_; }
    const std::vector<double>& equities() const { return equities_; }

    double total_return() const;

private:
    std::vector<Timestamp> timestamps_;
    std::vector<double> equities_;
};

}  // namespace regimeflow::metrics
