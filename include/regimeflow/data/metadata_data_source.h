/**
 * @file metadata_data_source.h
 * @brief RegimeFlow regimeflow metadata data source declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/symbol_metadata.h"

#include <memory>

namespace regimeflow::data {

/**
 * @brief Data source wrapper that overlays symbol metadata.
 */
class MetadataOverlayDataSource final : public DataSource {
public:
    /**
     * @brief Construct a metadata overlay.
     * @param inner Wrapped data source.
     * @param csv_metadata Metadata loaded from CSV.
     * @param config_metadata Metadata loaded from config.
     */
    MetadataOverlayDataSource(std::unique_ptr<DataSource> inner,
                              SymbolMetadataMap csv_metadata,
                              SymbolMetadataMap config_metadata);

    /**
     * @brief List available symbols with overlayed metadata.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Fetch bars for a symbol and range.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Fetch ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    /**
     * @brief Fetch order books for a symbol and range.
     */
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Create a bar iterator for multiple symbols.
     */
    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
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

private:
    std::unique_ptr<DataSource> inner_;
    SymbolMetadataMap csv_metadata_;
    SymbolMetadataMap config_metadata_;
};

}  // namespace regimeflow::data
