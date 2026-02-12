/**
 * @file db_source.h
 * @brief RegimeFlow regimeflow db source declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/db_client.h"
#include "regimeflow/data/validation_config.h"

namespace regimeflow::data {

/**
 * @brief Data source backed by a database client.
 */
class DatabaseDataSource final : public DataSource {
public:
    /**
     * @brief Database data source configuration.
     */
    struct Config {
        /**
         * @brief Connection string to the database.
         */
        std::string connection_string;
        /**
         * @brief Bars table name.
         */
        std::string bars_table = "market_bars";
        /**
         * @brief Ticks table name.
         */
        std::string ticks_table = "market_ticks";
        /**
         * @brief Corporate actions table name.
         */
        std::string actions_table = "corporate_actions";
        /**
         * @brief Order books table name.
         */
        std::string order_books_table = "order_books";
        /**
         * @brief Symbols metadata table name.
         */
        std::string symbols_table;
        /**
         * @brief Connection pool size.
         */
        int connection_pool_size = 4;
        /**
         * @brief Whether bars table includes bar_type column.
         */
        bool bars_has_bar_type = true;
        /**
         * @brief Validation configuration.
         */
        ValidationConfig validation;
        /**
         * @brief Collect validation report if true.
         */
        bool collect_validation_report = false;
        /**
         * @brief Fill missing bars if possible.
         */
        bool fill_missing_bars = false;
    };

    /**
     * @brief Construct a database data source.
     * @param config Database configuration.
     */
    explicit DatabaseDataSource(const Config& config);
    /**
     * @brief Inject a database client.
     * @param client Database client instance.
     */
    void set_client(std::shared_ptr<DbClient> client);
    /**
     * @brief Inject corporate actions programmatically.
     * @param symbol Symbol ID.
     * @param actions Action list.
     */
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

    /**
     * @brief List available symbols.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Determine available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Load bars for a symbol and range.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Load ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    /**
     * @brief Load order books for a symbol and range.
     */
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Create a bar iterator for multiple symbols.
     */
    std::unique_ptr<DataIterator> create_iterator(const std::vector<SymbolId>& symbols,
                                                   TimeRange range,
                                                   BarType bar_type) override;
    /**
     * @brief Create a tick iterator.
     */
    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;
    /**
     * @brief Create an order book iterator.
     */
    std::unique_ptr<OrderBookIterator> create_book_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;

    /**
     * @brief Fetch corporate actions for a symbol.
     */
    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;
    /**
     * @brief Last validation report.
     */
    const ValidationReport& last_report() const { return last_report_; }

private:
    Config config_;
    std::shared_ptr<DbClient> client_;
    CorporateActionAdjuster adjuster_;
    mutable ValidationReport last_report_;
};

}  // namespace regimeflow::data
