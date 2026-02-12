/**
 * @file data_source.h
 * @brief RegimeFlow regimeflow data source declarations.
 */

#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/corporate_actions.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"

#include <memory>
#include <string>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Metadata describing a tradeable symbol.
 */
struct SymbolInfo {
    SymbolId id = 0;
    std::string ticker;
    std::string exchange;
    AssetClass asset_class = AssetClass::Equity;
    std::string currency = "USD";
    double tick_size = 0.0;
    double lot_size = 0.0;
    double multiplier = 1.0;
    TimeRange trading_hours;
    std::string sector;
    std::string industry;
};

/**
 * @brief Iterator over bar data.
 */
class DataIterator {
public:
    virtual ~DataIterator() = default;
    /**
     * @brief True if more bars are available.
     */
    virtual bool has_next() const = 0;
    /**
     * @brief Retrieve the next bar.
     * @return Next bar.
     */
    virtual Bar next() = 0;
    /**
     * @brief Reset the iterator to the beginning.
     */
    virtual void reset() = 0;
};

/**
 * @brief Iterator over tick data.
 */
class TickIterator {
public:
    virtual ~TickIterator() = default;
    /**
     * @brief True if more ticks are available.
     */
    virtual bool has_next() const = 0;
    /**
     * @brief Retrieve the next tick.
     * @return Next tick.
     */
    virtual Tick next() = 0;
    /**
     * @brief Reset the iterator to the beginning.
     */
    virtual void reset() = 0;
};

/**
 * @brief Iterator over order book snapshots.
 */
class OrderBookIterator {
public:
    virtual ~OrderBookIterator() = default;
    /**
     * @brief True if more order books are available.
     */
    virtual bool has_next() const = 0;
    /**
     * @brief Retrieve the next order book snapshot.
     * @return Next order book.
     */
    virtual OrderBook next() = 0;
    /**
     * @brief Reset the iterator to the beginning.
     */
    virtual void reset() = 0;
};

/**
 * @brief Abstract base for market data sources.
 */
class DataSource {
public:
    virtual ~DataSource() = default;

    /**
     * @brief Enumerate available symbols.
     */
    virtual std::vector<SymbolInfo> get_available_symbols() const = 0;
    /**
     * @brief Retrieve available data range for a symbol.
     */
    virtual TimeRange get_available_range(SymbolId symbol) const = 0;

    /**
     * @brief Fetch bars for a symbol and range.
     */
    virtual std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                                      BarType bar_type = BarType::Time_1Day) = 0;
    /**
     * @brief Fetch ticks for a symbol and range.
     */
    virtual std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) = 0;
    /**
     * @brief Fetch order book snapshots for a symbol and range.
     */
    virtual std::vector<OrderBook> get_order_books([[maybe_unused]] SymbolId symbol,
                                                   [[maybe_unused]] TimeRange range) {
        return {};
    }

    /**
     * @brief Create a bar iterator for multiple symbols.
     */
    virtual std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) = 0;
    /**
     * @brief Create a tick iterator.
     */
    virtual std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>&,
        TimeRange) {
        return nullptr;
    }
    /**
     * @brief Create an order book iterator.
     */
    virtual std::unique_ptr<OrderBookIterator> create_book_iterator(
        const std::vector<SymbolId>&,
        TimeRange) {
        return nullptr;
    }

    /**
     * @brief Fetch corporate actions for a symbol.
     */
    virtual std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                               TimeRange range) = 0;
};

}  // namespace regimeflow::data
