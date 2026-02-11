#include "regimeflow/data/metadata_data_source.h"

namespace regimeflow::data {

MetadataOverlayDataSource::MetadataOverlayDataSource(std::unique_ptr<DataSource> inner,
                                                     SymbolMetadataMap csv_metadata,
                                                     SymbolMetadataMap config_metadata)
    : inner_(std::move(inner)),
      csv_metadata_(std::move(csv_metadata)),
      config_metadata_(std::move(config_metadata)) {}

std::vector<SymbolInfo> MetadataOverlayDataSource::get_available_symbols() const {
    auto symbols = inner_->get_available_symbols();
    apply_symbol_metadata(symbols, csv_metadata_, false);
    apply_symbol_metadata(symbols, config_metadata_, true);
    return symbols;
}

TimeRange MetadataOverlayDataSource::get_available_range(SymbolId symbol) const {
    return inner_->get_available_range(symbol);
}

std::vector<Bar> MetadataOverlayDataSource::get_bars(SymbolId symbol, TimeRange range,
                                                     BarType bar_type) {
    return inner_->get_bars(symbol, range, bar_type);
}

std::vector<Tick> MetadataOverlayDataSource::get_ticks(SymbolId symbol, TimeRange range) {
    return inner_->get_ticks(symbol, range);
}

std::vector<OrderBook> MetadataOverlayDataSource::get_order_books(SymbolId symbol,
                                                                  TimeRange range) {
    return inner_->get_order_books(symbol, range);
}

std::unique_ptr<DataIterator> MetadataOverlayDataSource::create_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range,
    BarType bar_type) {
    return inner_->create_iterator(symbols, range, bar_type);
}

std::unique_ptr<TickIterator> MetadataOverlayDataSource::create_tick_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range) {
    return inner_->create_tick_iterator(symbols, range);
}

std::unique_ptr<OrderBookIterator> MetadataOverlayDataSource::create_book_iterator(
    const std::vector<SymbolId>& symbols,
    TimeRange range) {
    return inner_->create_book_iterator(symbols, range);
}

std::vector<CorporateAction> MetadataOverlayDataSource::get_corporate_actions(SymbolId symbol,
                                                                              TimeRange range) {
    return inner_->get_corporate_actions(symbol, range);
}

}  // namespace regimeflow::data
