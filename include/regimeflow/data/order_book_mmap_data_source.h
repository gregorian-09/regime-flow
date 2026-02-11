#pragma once

#include "regimeflow/common/lru_cache.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/corporate_actions.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/order_book_mmap.h"

#include <memory>
#include <string>

namespace regimeflow::data {

class OrderBookMmapDataSource final : public DataSource {
public:
    struct Config {
        std::string data_directory;
        size_t max_cached_files = 100;
        size_t max_cached_ranges = 0;
    };

    explicit OrderBookMmapDataSource(const Config& config);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
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
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

private:
    std::shared_ptr<OrderBookMmapFile> get_file(SymbolId symbol) const;

    Config config_;
    mutable LRUCache<std::string, std::shared_ptr<OrderBookMmapFile>> file_cache_;
    mutable LRUCache<std::string, std::shared_ptr<std::vector<OrderBook>>> range_cache_;
    CorporateActionAdjuster adjuster_;
};

}  // namespace regimeflow::data
