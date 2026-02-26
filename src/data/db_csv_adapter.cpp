#include "regimeflow/data/db_csv_adapter.h"

namespace regimeflow::data
{
    CsvDbClient::CsvDbClient(CSVDataSource source) : source_(std::move(source)) {}

    std::vector<Bar> CsvDbClient::query_bars(const SymbolId symbol, const TimeRange range, const BarType bar_type) {
        return source_.get_bars(symbol, range, bar_type);
    }

    std::vector<Tick> CsvDbClient::query_ticks(SymbolId, TimeRange) {
        return {};
    }

    std::vector<SymbolInfo> CsvDbClient::list_symbols() {
        return source_.get_available_symbols();
    }

    TimeRange CsvDbClient::get_available_range(const SymbolId symbol) {
        return source_.get_available_range(symbol);
    }

    std::vector<CorporateAction> CsvDbClient::query_corporate_actions(const SymbolId symbol,
                                                                      const TimeRange range) {
        return source_.get_corporate_actions(symbol, range);
    }

    std::vector<OrderBook> CsvDbClient::query_order_books(SymbolId, TimeRange) {
        return {};
    }
}  // namespace regimeflow::data
