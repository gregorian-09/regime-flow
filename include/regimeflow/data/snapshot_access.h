#pragma once

#include "regimeflow/data/data_source.h"

#include <memory>
#include <optional>

namespace regimeflow::data {

class SnapshotAccess {
public:
    explicit SnapshotAccess(std::shared_ptr<DataSource> source);

    std::optional<Bar> bar_at(SymbolId symbol, Timestamp ts, BarType bar_type);
    std::optional<Tick> tick_at(SymbolId symbol, Timestamp ts);
    std::optional<OrderBook> order_book_at(SymbolId symbol, Timestamp ts);

private:
    std::shared_ptr<DataSource> source_;
};

}  // namespace regimeflow::data
