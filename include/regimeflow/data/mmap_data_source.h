/**
 * @file mmap_data_source.h
 * @brief RegimeFlow regimeflow mmap data source declarations.
 */

#pragma once

#include "regimeflow/common/lru_cache.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/corporate_actions.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/mmap_reader.h"

#include <memory>
#include <string>

namespace regimeflow::data {

/**
 * @brief Memory-mapped data source for efficient historical access.
 */
class MemoryMappedDataSource : public DataSource {
public:
    /**
     * @brief Memory-mapped data source configuration.
     */
    struct Config {
        /**
         * @brief Root directory for mmap files.
         */
        std::string data_directory;
        /**
         * @brief Preload index metadata at startup.
         */
        bool preload_index = true;
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
     * @brief Construct a memory-mapped data source.
     * @param config Configuration.
     */
    explicit MemoryMappedDataSource(const Config& config);

    /**
     * @brief List available symbols.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Get available range for a symbol.
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
     * @brief Create a bar iterator for multiple symbols.
     */
    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;

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
    std::shared_ptr<MemoryMappedDataFile> get_file(SymbolId symbol, BarType bar_type) const;
    static std::string bar_type_suffix(BarType type);

    Config config_;
    mutable LRUCache<std::string, std::shared_ptr<MemoryMappedDataFile>> file_cache_;
    mutable LRUCache<std::string, std::shared_ptr<std::vector<Bar>>> range_cache_;
    CorporateActionAdjuster adjuster_;
};

}  // namespace regimeflow::data
