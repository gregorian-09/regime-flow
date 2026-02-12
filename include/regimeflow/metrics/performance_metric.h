/**
 * @file performance_metric.h
 * @brief RegimeFlow regimeflow performance metric declarations.
 */

#pragma once

#include "regimeflow/metrics/performance.h"

#include <string>

namespace regimeflow::metrics {

/**
 * @brief Base class for a single performance metric.
 */
class PerformanceMetric {
public:
    virtual ~PerformanceMetric() = default;
    /**
     * @brief Name of the metric.
     */
    virtual std::string name() const = 0;
    /**
     * @brief Compute the metric from an equity curve.
     * @param curve Equity curve.
     * @param periods_per_year Annualization factor.
     * @return Metric value.
     */
    virtual double compute(const EquityCurve& curve, double periods_per_year) const = 0;
};

}  // namespace regimeflow::metrics
