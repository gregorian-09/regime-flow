#include "regimeflow/engine/order_book_cache.h"

namespace regimeflow::engine {

void OrderBookCache::update(const data::OrderBook& book) {
    books_[book.symbol] = book;
}

std::optional<data::OrderBook> OrderBookCache::latest(SymbolId symbol) const {
    auto it = books_.find(symbol);
    if (it == books_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace regimeflow::engine
