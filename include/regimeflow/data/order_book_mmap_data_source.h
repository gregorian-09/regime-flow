/**
 * @file order_book_mmap_data_source.h
 * @brief RegimeFlow regimeflow order book mmap data source declarations.
 */

#pragma once

#include "regimeflow/common/lru_cache.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/corporate_actions.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/order_book_mmap.h"

#include <memory>
#include <string>

namespace regimeflow::data {

/**
 * @brief Data source for memory-mapped order book snapshots.
 */
class OrderBookMmapDataSource final : public DataSource {
public:
    /**
     * @brief Configuration for order book mmap data source.
     */
    struct Config {
        /**
         * @brief Root directory for order book files.
         */
        std::string data_directory;
        /**
         * @brief Maximum cached files in LRU.
         */
        size_t max_cached_files = 100;
        /**
         * @brief Maximum cached ranges in LRU (0 disables).
         */
        size_t max_cached_ranges = 0;
    };

    /**
     * @brief Construct with configuration.
     * @param config Data source configuration.
     */
    explicit OrderBookMmapDataSource(const Config& config);

    /**
     * @brief List available symbols.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Bars are not supported; returns empty.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Ticks are not supported; returns empty.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    /**
     * @brief Load order books for a symbol and range.
     */
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Bars are not supported; returns empty iterator.
     */
    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;
    /**
     * @brief Ticks are not supported; returns empty iterator.
     */
    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;
    /**
     * @brief Create order book iterator for multiple symbols.
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
     * @brief Inject corporate actions programmatically.
     */
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

private:
    std::shared_ptr<OrderBookMmapFile> get_file(SymbolId symbol) const;

    Config config_;
    mutable LRUCache<std::string, std::shared_ptr<OrderBookMmapFile>> file_cache_;
    mutable LRUCache<std::string, std::shared_ptr<std::vector<OrderBook>>> range_cache_;
    CorporateActionAdjuster adjuster_;
};

}  // namespace regimeflow::data
