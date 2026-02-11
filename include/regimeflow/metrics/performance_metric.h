#pragma once

#include "regimeflow/metrics/performance.h"

#include <string>

namespace regimeflow::metrics {

class PerformanceMetric {
public:
    virtual ~PerformanceMetric() = default;
    virtual std::string name() const = 0;
    virtual double compute(const EquityCurve& curve, double periods_per_year) const = 0;
};

}  // namespace regimeflow::metrics
