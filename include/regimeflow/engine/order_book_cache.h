#pragma once

#include "regimeflow/data/order_book.h"

#include <optional>
#include <unordered_map>

namespace regimeflow::engine {

class OrderBookCache {
public:
    void update(const data::OrderBook& book);
    std::optional<data::OrderBook> latest(SymbolId symbol) const;

private:
    std::unordered_map<SymbolId, data::OrderBook> books_;
};

}  // namespace regimeflow::engine
