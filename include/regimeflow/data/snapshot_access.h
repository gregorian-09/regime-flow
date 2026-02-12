/**
 * @file snapshot_access.h
 * @brief RegimeFlow regimeflow snapshot access declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"

#include <memory>
#include <optional>

namespace regimeflow::data {

/**
 * @brief Access point-in-time snapshots from a data source.
 */
class SnapshotAccess {
public:
    /**
     * @brief Construct with a data source.
     * @param source Data source.
     */
    explicit SnapshotAccess(std::shared_ptr<DataSource> source);

    /**
     * @brief Get bar at a specific timestamp.
     * @param symbol Symbol ID.
     * @param ts Timestamp.
     * @param bar_type Bar type.
     * @return Optional bar.
     */
    std::optional<Bar> bar_at(SymbolId symbol, Timestamp ts, BarType bar_type);
    /**
     * @brief Get tick at a specific timestamp.
     * @param symbol Symbol ID.
     * @param ts Timestamp.
     * @return Optional tick.
     */
    std::optional<Tick> tick_at(SymbolId symbol, Timestamp ts);
    /**
     * @brief Get order book at a specific timestamp.
     * @param symbol Symbol ID.
     * @param ts Timestamp.
     * @return Optional order book.
     */
    std::optional<OrderBook> order_book_at(SymbolId symbol, Timestamp ts);

private:
    std::shared_ptr<DataSource> source_;
};

}  // namespace regimeflow::data
