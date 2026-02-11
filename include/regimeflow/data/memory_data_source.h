#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/corporate_actions.h"

#include <algorithm>
#include <map>
#include <utility>

namespace regimeflow::data {

class VectorBarIterator final : public DataIterator {
public:
    explicit VectorBarIterator(std::vector<Bar> bars);

    bool has_next() const override;
    Bar next() override;
    void reset() override;

private:
    std::vector<Bar> bars_;
    size_t index_ = 0;
};

class VectorOrderBookIterator final : public OrderBookIterator {
public:
    explicit VectorOrderBookIterator(std::vector<OrderBook> books);

    bool has_next() const override;
    OrderBook next() override;
    void reset() override;

private:
    std::vector<OrderBook> books_;
    size_t index_ = 0;
};

class VectorTickIterator final : public TickIterator {
public:
    explicit VectorTickIterator(std::vector<Tick> ticks);

    bool has_next() const override;
    Tick next() override;
    void reset() override;

private:
    std::vector<Tick> ticks_;
    size_t index_ = 0;
};

class MemoryDataSource final : public DataSource {
public:
    void add_bars(SymbolId symbol, std::vector<Bar> bars);
    void add_ticks(SymbolId symbol, std::vector<Tick> ticks);
    void add_order_books(SymbolId symbol, std::vector<OrderBook> books);
    void add_symbol_info(SymbolInfo info);
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;
    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;
    std::unique_ptr<OrderBookIterator> create_book_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;

    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;

private:
    static bool in_range(const Timestamp& ts, TimeRange range);

    std::map<SymbolId, std::vector<Bar>> bars_;
    std::map<SymbolId, std::vector<Tick>> ticks_;
    std::map<SymbolId, std::vector<OrderBook>> books_;
    std::map<SymbolId, SymbolInfo> symbols_;
    CorporateActionAdjuster adjuster_;
};

}  // namespace regimeflow::data
