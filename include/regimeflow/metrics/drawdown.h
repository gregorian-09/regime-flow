/**
 * @file drawdown.h
 * @brief RegimeFlow regimeflow drawdown declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

namespace regimeflow::metrics {

/**
 * @brief Drawdown snapshot at a specific time.
 */
struct DrawdownSnapshot {
    Timestamp timestamp;
    double equity = 0;
    double peak = 0;
    double drawdown = 0;
};

/**
 * @brief Tracks peak-to-trough drawdowns.
 */
class DrawdownTracker {
public:
    /**
     * @brief Update drawdown tracker with new equity.
     * @param timestamp Update time.
     * @param equity Current equity.
     */
    void update(Timestamp timestamp, double equity);

    /**
     * @brief Maximum drawdown observed.
     */
    double max_drawdown() const { return max_drawdown_; }
    /**
     * @brief Timestamp where max drawdown started.
     */
    Timestamp max_drawdown_start() const { return max_start_; }
    /**
     * @brief Timestamp where max drawdown ended.
     */
    Timestamp max_drawdown_end() const { return max_end_; }

    /**
     * @brief Last drawdown snapshot.
     */
    DrawdownSnapshot last_snapshot() const { return last_; }

private:
    double peak_ = 0;
    double max_drawdown_ = 0;
    Timestamp max_start_;
    Timestamp max_end_;
    DrawdownSnapshot last_;
    Timestamp current_peak_time_;
};

}  // namespace regimeflow::metrics
