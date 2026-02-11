#include "regimeflow/live/types.h"

namespace regimeflow::live {

Timestamp MarketDataUpdate::timestamp() const {
    return std::visit([](const auto& value) { return value.timestamp; }, data);
}

SymbolId MarketDataUpdate::symbol() const {
    return std::visit([](const auto& value) { return value.symbol; }, data);
}

}  // namespace regimeflow::live
