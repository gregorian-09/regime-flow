/**
 * @file tick_mmap_data_source.h
 * @brief RegimeFlow regimeflow tick mmap data source declarations.
 */

#pragma once

#include "regimeflow/common/lru_cache.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/corporate_actions.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/tick_mmap.h"

#include <memory>
#include <string>

namespace regimeflow::data {

/**
 * @brief Data source for memory-mapped tick data.
 */
class TickMmapDataSource final : public DataSource {
public:
    /**
     * @brief Configuration for tick mmap data source.
     */
    struct Config {
        /**
         * @brief Root directory for tick files.
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
    explicit TickMmapDataSource(const Config& config);

    /**
     * @brief List available symbols.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Bars not supported; returns empty.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Load ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Bars not supported; returns empty iterator.
     */
    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;
    /**
     * @brief Create a tick iterator for multiple symbols.
     */
    std::unique_ptr<TickIterator> create_tick_iterator(
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
    std::shared_ptr<TickMmapFile> get_file(SymbolId symbol) const;

    Config config_;
    mutable LRUCache<std::string, std::shared_ptr<TickMmapFile>> file_cache_;
    mutable LRUCache<std::string, std::shared_ptr<std::vector<Tick>>> range_cache_;
    CorporateActionAdjuster adjuster_;
};

}  // namespace regimeflow::data
