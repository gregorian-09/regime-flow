/**
 * @file db_client.h
 * @brief RegimeFlow regimeflow db client declarations.
 */

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

/**
 * @brief Abstract database client for historical market data.
 */
class DbClient {
public:
    virtual ~DbClient() = default;

    /**
     * @brief Query bars for a symbol and range.
     */
    virtual std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) = 0;
    /**
     * @brief Query ticks for a symbol and range.
     */
    virtual std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) = 0;
    /**
     * @brief List all symbols.
     */
    virtual std::vector<SymbolInfo> list_symbols() = 0;
    /**
     * @brief List symbols including metadata.
     * @param symbols_table Table name or identifier.
     * @return Symbol list with metadata when available.
     */
    virtual std::vector<SymbolInfo> list_symbols_with_metadata(const std::string& symbols_table) {
        (void)symbols_table;
        return list_symbols();
    }
    /**
     * @brief Get available range for a symbol.
     */
    virtual TimeRange get_available_range(SymbolId symbol) = 0;
    /**
     * @brief Query corporate actions for a symbol.
     */
    virtual std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                                  TimeRange range) = 0;
    /**
     * @brief Query order book snapshots for a symbol.
     */
    virtual std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) = 0;
};

/**
 * @brief In-memory database client for tests or ad-hoc data.
 */
class InMemoryDbClient final : public DbClient {
public:
    /**
     * @brief Add bar data.
     */
    void add_bars(SymbolId symbol, std::vector<Bar> bars);
    /**
     * @brief Add tick data.
     */
    void add_ticks(SymbolId symbol, std::vector<Tick> ticks);
    /**
     * @brief Add symbol metadata.
     */
    void add_symbol_info(SymbolInfo info);
    /**
     * @brief Add corporate actions.
     */
    void add_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);
    /**
     * @brief Add order book snapshots.
     */
    void add_order_books(SymbolId symbol, std::vector<OrderBook> books);

    /**
     * @brief Query bars for a symbol and range.
     */
    std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;
    /**
     * @brief Query ticks for a symbol and range.
     */
    std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;
    /**
     * @brief List all symbols.
     */
    std::vector<SymbolInfo> list_symbols() override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) override;
    /**
     * @brief Query corporate actions for a symbol.
     */
    std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                         TimeRange range) override;
    /**
     * @brief Query order books for a symbol.
     */
    std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;

private:
    std::unordered_map<SymbolId, std::vector<Bar>> bars_;
    std::unordered_map<SymbolId, std::vector<Tick>> ticks_;
    std::unordered_map<SymbolId, SymbolInfo> symbols_;
    std::unordered_map<SymbolId, std::vector<CorporateAction>> actions_;
    std::unordered_map<SymbolId, std::vector<OrderBook>> books_;
};

}  // namespace regimeflow::data
