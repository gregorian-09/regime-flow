#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/symbol_metadata.h"

#include <memory>

namespace regimeflow::data {

class MetadataOverlayDataSource final : public DataSource {
public:
    MetadataOverlayDataSource(std::unique_ptr<DataSource> inner,
                              SymbolMetadataMap csv_metadata,
                              SymbolMetadataMap config_metadata);

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

private:
    std::unique_ptr<DataSource> inner_;
    SymbolMetadataMap csv_metadata_;
    SymbolMetadataMap config_metadata_;
};

}  // namespace regimeflow::data
