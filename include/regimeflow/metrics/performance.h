/**
 * @file performance.h
 * @brief RegimeFlow regimeflow performance declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

#include <vector>

namespace regimeflow::metrics {

/**
 * @brief Tracks equity curve over time.
 */
class EquityCurve {
public:
    /**
     * @brief Add an equity sample.
     * @param timestamp Sample time.
     * @param equity Equity value.
     */
    void add_point(Timestamp timestamp, double equity);

    /**
     * @brief Timestamps for the equity curve.
     */
    const std::vector<Timestamp>& timestamps() const { return timestamps_; }
    /**
     * @brief Equity values corresponding to timestamps.
     */
    const std::vector<double>& equities() const { return equities_; }

    /**
     * @brief Total return from first to last point.
     * @return Total return as a fraction.
     */
    double total_return() const;

private:
    std::vector<Timestamp> timestamps_;
    std::vector<double> equities_;
};

}  // namespace regimeflow::metrics
