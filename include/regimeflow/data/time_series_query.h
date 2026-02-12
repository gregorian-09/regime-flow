/**
 * @file time_series_query.h
 * @brief RegimeFlow regimeflow time series query declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"

#include <memory>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Helper for querying time series data from a source.
 */
class TimeSeriesQuery {
public:
    /**
     * @brief Construct with a data source.
     * @param source Data source.
     */
    explicit TimeSeriesQuery(std::shared_ptr<DataSource> source);

    /**
     * @brief Query bars for a symbol and range.
     */
    std::vector<Bar> bars(SymbolId symbol, TimeRange range, BarType bar_type);
    /**
     * @brief Query ticks for a symbol and range.
     */
    std::vector<Tick> ticks(SymbolId symbol, TimeRange range);
    /**
     * @brief Query order books for a symbol and range.
     */
    std::vector<OrderBook> order_books(SymbolId symbol, TimeRange range);

private:
    std::shared_ptr<DataSource> source_;
};

}  // namespace regimeflow::data
