#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::engine {

class MarketDataCache {
public:
    void update(const data::Bar& bar);
    void update(const data::Tick& tick);
    void update(const data::Quote& quote);

    std::optional<data::Bar> latest_bar(SymbolId symbol) const;
    std::optional<data::Tick> latest_tick(SymbolId symbol) const;
    std::optional<data::Quote> latest_quote(SymbolId symbol) const;

    std::vector<data::Bar> recent_bars(SymbolId symbol, size_t count) const;

private:
    std::unordered_map<SymbolId, data::Bar> latest_bars_;
    std::unordered_map<SymbolId, data::Tick> latest_ticks_;
    std::unordered_map<SymbolId, data::Quote> latest_quotes_;
    std::unordered_map<SymbolId, std::vector<data::Bar>> bar_history_;
    size_t max_history_ = 1024;
};

}  // namespace regimeflow::engine
