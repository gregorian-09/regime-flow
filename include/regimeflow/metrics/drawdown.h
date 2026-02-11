#pragma once

#include "regimeflow/common/time.h"

namespace regimeflow::metrics {

struct DrawdownSnapshot {
    Timestamp timestamp;
    double equity = 0;
    double peak = 0;
    double drawdown = 0;
};

class DrawdownTracker {
public:
    void update(Timestamp timestamp, double equity);

    double max_drawdown() const { return max_drawdown_; }
    Timestamp max_drawdown_start() const { return max_start_; }
    Timestamp max_drawdown_end() const { return max_end_; }

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
