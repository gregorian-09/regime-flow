/**
 * @file db_csv_adapter.h
 * @brief RegimeFlow regimeflow db csv adapter declarations.
 */

#pragma once

#include "regimeflow/data/db_client.h"
#include "regimeflow/data/csv_reader.h"

namespace regimeflow::data {

/**
 * @brief DB client adapter backed by CSV data source.
 */
class CsvDbClient final : public DbClient {
public:
    /**
     * @brief Construct from a CSV data source.
     * @param source CSV data source instance.
     */
    explicit CsvDbClient(CSVDataSource source);

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
     * @brief Query order book snapshots for a symbol.
     */
    std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;

private:
    CSVDataSource source_;
};

}  // namespace regimeflow::data
