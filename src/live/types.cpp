#include "regimeflow/live/types.h"

namespace regimeflow::live
{
    Timestamp MarketDataUpdate::timestamp() const {
        return std::visit([](const auto& value) { return value.timestamp; }, data);
    }

    SymbolId MarketDataUpdate::symbol() const {
        return std::visit([](const auto& value) { return value.symbol; }, data);
    }

    events::Event to_engine_event(const MarketDataUpdate& update) {
        return std::visit([](const auto& value) { return events::make_market_event(value); }, update.data);
    }
}  // namespace regimeflow::live
