#pragma once

#include "regimeflow/data/data_source.h"

#include <memory>
#include <vector>

namespace regimeflow::data {

class TimeSeriesQuery {
public:
    explicit TimeSeriesQuery(std::shared_ptr<DataSource> source);

    std::vector<Bar> bars(SymbolId symbol, TimeRange range, BarType bar_type);
    std::vector<Tick> ticks(SymbolId symbol, TimeRange range);
    std::vector<OrderBook> order_books(SymbolId symbol, TimeRange range);

private:
    std::shared_ptr<DataSource> source_;
};

}  // namespace regimeflow::data
