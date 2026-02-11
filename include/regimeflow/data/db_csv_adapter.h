#pragma once

#include "regimeflow/data/db_client.h"
#include "regimeflow/data/csv_reader.h"

namespace regimeflow::data {

class CsvDbClient final : public DbClient {
public:
    explicit CsvDbClient(CSVDataSource source);

    std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;
    std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;
    std::vector<SymbolInfo> list_symbols() override;
    TimeRange get_available_range(SymbolId symbol) override;
    std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                         TimeRange range) override;
    std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;

private:
    CSVDataSource source_;
};

}  // namespace regimeflow::data
