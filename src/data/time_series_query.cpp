#include "regimeflow/data/time_series_query.h"

#include <stdexcept>

namespace regimeflow::data
{
    TimeSeriesQuery::TimeSeriesQuery(std::shared_ptr<DataSource> source)
        : source_(std::move(source)) {
        if (!source_) {
            throw std::invalid_argument("TimeSeriesQuery requires a data source");
        }
    }

    std::vector<Bar> TimeSeriesQuery::bars(const SymbolId symbol, const TimeRange range, const BarType bar_type) const
    {
        return source_->get_bars(symbol, range, bar_type);
    }

    std::vector<Tick> TimeSeriesQuery::ticks(const SymbolId symbol, const TimeRange range) const
    {
        return source_->get_ticks(symbol, range);
    }

    std::vector<OrderBook> TimeSeriesQuery::order_books(const SymbolId symbol, const TimeRange range) const
    {
        return source_->get_order_books(symbol, range);
    }
}  // namespace regimeflow::data
