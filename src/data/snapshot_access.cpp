#include "regimeflow/data/snapshot_access.h"

#include <stdexcept>

namespace regimeflow::data {

SnapshotAccess::SnapshotAccess(std::shared_ptr<DataSource> source)
    : source_(std::move(source)) {
    if (!source_) {
        throw std::invalid_argument("SnapshotAccess requires a data source");
    }
}

std::optional<Bar> SnapshotAccess::bar_at(SymbolId symbol, Timestamp ts, BarType bar_type) {
    TimeRange range;
    range.start = Timestamp(0);
    range.end = ts;
    auto bars = source_->get_bars(symbol, range, bar_type);
    if (bars.empty()) {
        return std::nullopt;
    }
    return bars.back();
}

std::optional<Tick> SnapshotAccess::tick_at(SymbolId symbol, Timestamp ts) {
    TimeRange range;
    range.start = Timestamp(0);
    range.end = ts;
    auto ticks = source_->get_ticks(symbol, range);
    if (ticks.empty()) {
        return std::nullopt;
    }
    return ticks.back();
}

std::optional<OrderBook> SnapshotAccess::order_book_at(SymbolId symbol, Timestamp ts) {
    TimeRange range;
    range.start = Timestamp(0);
    range.end = ts;
    auto books = source_->get_order_books(symbol, range);
    if (books.empty()) {
        return std::nullopt;
    }
    return books.back();
}

}  // namespace regimeflow::data
