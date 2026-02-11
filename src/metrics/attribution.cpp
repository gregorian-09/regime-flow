#include "regimeflow/metrics/attribution.h"

namespace regimeflow::metrics {

void AttributionTracker::update(Timestamp timestamp, const engine::Portfolio& portfolio) {
    AttributionSnapshot snapshot;
    snapshot.timestamp = timestamp;

    double total = 0.0;
    for (const auto& position : portfolio.get_all_positions()) {
        double value = position.market_value();
        double last = last_values_[position.symbol];
        double pnl = value - last;
        snapshot.pnl_by_symbol[position.symbol] = pnl;
        total += pnl;
        last_values_[position.symbol] = value;
    }

    snapshot.total_pnl = total;
    last_ = std::move(snapshot);
}

}  // namespace regimeflow::metrics
