#include "regimeflow/execution/order_book_execution_model.h"

#include <algorithm>

namespace regimeflow::execution {

OrderBookExecutionModel::OrderBookExecutionModel(std::shared_ptr<data::OrderBook> book)
    : book_(std::move(book)) {}

std::vector<engine::Fill> OrderBookExecutionModel::execute(const engine::Order& order,
                                                           Price reference_price,
                                                           Timestamp timestamp) {
    std::vector<engine::Fill> fills;
    if (!book_ || order.quantity <= 0) {
        return fills;
    }

    double remaining = order.quantity;
    auto consume_level = [&](const data::BookLevel& level) {
        if (level.quantity <= 0 || remaining <= 0) {
            return;
        }
        double qty = std::min(remaining, level.quantity);
        engine::Fill fill;
        fill.order_id = order.id;
        fill.symbol = order.symbol;
        fill.quantity = qty * (order.side == engine::OrderSide::Buy ? 1.0 : -1.0);
        fill.price = level.price > 0 ? level.price : reference_price;
        fill.timestamp = timestamp;
        fills.push_back(fill);
        remaining -= qty;
    };

    if (order.side == engine::OrderSide::Buy) {
        for (const auto& level : book_->asks) {
            consume_level(level);
            if (remaining <= 0) break;
        }
    } else {
        for (const auto& level : book_->bids) {
            consume_level(level);
            if (remaining <= 0) break;
        }
    }

    return fills;
}

}  // namespace regimeflow::execution
