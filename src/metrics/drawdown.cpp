#include "regimeflow/metrics/drawdown.h"

namespace regimeflow::metrics {

void DrawdownTracker::update(Timestamp timestamp, double equity) {
    if (peak_ == 0 || equity >= peak_) {
        peak_ = equity;
        current_peak_time_ = timestamp;
    }

    double drawdown = peak_ > 0 ? (peak_ - equity) / peak_ : 0;
    if (drawdown > max_drawdown_) {
        max_drawdown_ = drawdown;
        max_start_ = current_peak_time_;
        max_end_ = timestamp;
    }

    last_.timestamp = timestamp;
    last_.equity = equity;
    last_.peak = peak_;
    last_.drawdown = drawdown;
}

}  // namespace regimeflow::metrics
