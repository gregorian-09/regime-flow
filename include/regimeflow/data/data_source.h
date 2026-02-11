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

class DataIterator {
public:
    virtual ~DataIterator() = default;
    virtual bool has_next() const = 0;
    virtual Bar next() = 0;
    virtual void reset() = 0;
};

class TickIterator {
public:
    virtual ~TickIterator() = default;
    virtual bool has_next() const = 0;
    virtual Tick next() = 0;
    virtual void reset() = 0;
};

class OrderBookIterator {
public:
    virtual ~OrderBookIterator() = default;
    virtual bool has_next() const = 0;
    virtual OrderBook next() = 0;
    virtual void reset() = 0;
};

class DataSource {
public:
    virtual ~DataSource() = default;

    virtual std::vector<SymbolInfo> get_available_symbols() const = 0;
    virtual TimeRange get_available_range(SymbolId symbol) const = 0;

    virtual std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                                      BarType bar_type = BarType::Time_1Day) = 0;
    virtual std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) = 0;
    virtual std::vector<OrderBook> get_order_books([[maybe_unused]] SymbolId symbol,
                                                   [[maybe_unused]] TimeRange range) {
        return {};
    }

    virtual std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) = 0;
    virtual std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>&,
        TimeRange) {
        return nullptr;
    }
    virtual std::unique_ptr<OrderBookIterator> create_book_iterator(
        const std::vector<SymbolId>&,
        TimeRange) {
        return nullptr;
    }

    virtual std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                               TimeRange range) = 0;
};

}  // namespace regimeflow::data
