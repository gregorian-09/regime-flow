#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/corporate_actions.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace regimeflow::data {

class DbClient {
public:
    virtual ~DbClient() = default;

    virtual std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) = 0;
    virtual std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) = 0;
    virtual std::vector<SymbolInfo> list_symbols() = 0;
    virtual std::vector<SymbolInfo> list_symbols_with_metadata(const std::string& symbols_table) {
        (void)symbols_table;
        return list_symbols();
    }
    virtual TimeRange get_available_range(SymbolId symbol) = 0;
    virtual std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                                  TimeRange range) = 0;
    virtual std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) = 0;
};

class InMemoryDbClient final : public DbClient {
public:
    void add_bars(SymbolId symbol, std::vector<Bar> bars);
    void add_ticks(SymbolId symbol, std::vector<Tick> ticks);
    void add_symbol_info(SymbolInfo info);
    void add_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);
    void add_order_books(SymbolId symbol, std::vector<OrderBook> books);

    std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;
    std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;
    std::vector<SymbolInfo> list_symbols() override;
    TimeRange get_available_range(SymbolId symbol) override;
    std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                         TimeRange range) override;
    std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;

private:
    std::unordered_map<SymbolId, std::vector<Bar>> bars_;
    std::unordered_map<SymbolId, std::vector<Tick>> ticks_;
    std::unordered_map<SymbolId, SymbolInfo> symbols_;
    std::unordered_map<SymbolId, std::vector<CorporateAction>> actions_;
    std::unordered_map<SymbolId, std::vector<OrderBook>> books_;
};

}  // namespace regimeflow::data
