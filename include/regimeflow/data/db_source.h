#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/db_client.h"
#include "regimeflow/data/validation_config.h"

namespace regimeflow::data {

class DatabaseDataSource final : public DataSource {
public:
    struct Config {
        std::string connection_string;
        std::string bars_table = "market_bars";
        std::string ticks_table = "market_ticks";
        std::string actions_table = "corporate_actions";
        std::string order_books_table = "order_books";
        std::string symbols_table;
        int connection_pool_size = 4;
        bool bars_has_bar_type = true;
        ValidationConfig validation;
        bool collect_validation_report = false;
        bool fill_missing_bars = false;
    };

    explicit DatabaseDataSource(const Config& config);
    void set_client(std::shared_ptr<DbClient> client);
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols,
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
    const ValidationReport& last_report() const { return last_report_; }

private:
    Config config_;
    std::shared_ptr<DbClient> client_;
    CorporateActionAdjuster adjuster_;
    mutable ValidationReport last_report_;
};

}  // namespace regimeflow::data
