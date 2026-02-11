#include "regimeflow/data/time_series_query.h"

#include <stdexcept>

namespace regimeflow::data {

TimeSeriesQuery::TimeSeriesQuery(std::shared_ptr<DataSource> source)
    : source_(std::move(source)) {
    if (!source_) {
        throw std::invalid_argument("TimeSeriesQuery requires a data source");
    }
}

std::vector<Bar> TimeSeriesQuery::bars(SymbolId symbol, TimeRange range, BarType bar_type) {
    return source_->get_bars(symbol, range, bar_type);
}

std::vector<Tick> TimeSeriesQuery::ticks(SymbolId symbol, TimeRange range) {
    return source_->get_ticks(symbol, range);
}

std::vector<OrderBook> TimeSeriesQuery::order_books(SymbolId symbol, TimeRange range) {
    return source_->get_order_books(symbol, range);
}

}  // namespace regimeflow::data
