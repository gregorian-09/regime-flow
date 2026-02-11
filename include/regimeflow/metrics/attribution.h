#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/engine/portfolio.h"

#include <unordered_map>

namespace regimeflow::metrics {

struct AttributionSnapshot {
    Timestamp timestamp;
    std::unordered_map<SymbolId, double> pnl_by_symbol;
    double total_pnl = 0.0;
};

class AttributionTracker {
public:
    void update(Timestamp timestamp, const engine::Portfolio& portfolio);

    const AttributionSnapshot& last() const { return last_; }

private:
    AttributionSnapshot last_;
    std::unordered_map<SymbolId, double> last_values_;
};

}  // namespace regimeflow::metrics
