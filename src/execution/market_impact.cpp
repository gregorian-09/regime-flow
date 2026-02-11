#include "regimeflow/execution/market_impact.h"

#include <algorithm>

namespace regimeflow::execution {

double OrderBookImpactModel::impact_bps(const engine::Order& order,
                                        const data::OrderBook* book) const {
    if (!book || order.quantity <= 0) {
        return 0.0;
    }
    const auto& levels = (order.side == engine::OrderSide::Buy) ? book->asks : book->bids;
    double available = 0.0;
    for (const auto& level : levels) {
        if (level.quantity > 0) {
            available += level.quantity;
        }
    }
    if (available <= 0.0) {
        return max_bps_;
    }
    double ratio = std::clamp(order.quantity / available, 0.0, 1.0);
    return ratio * max_bps_;
}

}  // namespace regimeflow::execution
