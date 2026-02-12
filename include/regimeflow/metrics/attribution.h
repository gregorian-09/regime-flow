/**
 * @file attribution.h
 * @brief RegimeFlow regimeflow attribution declarations.
 */

#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/engine/portfolio.h"

#include <unordered_map>

namespace regimeflow::metrics {

/**
 * @brief Attribution snapshot by symbol.
 */
struct AttributionSnapshot {
    Timestamp timestamp;
    std::unordered_map<SymbolId, double> pnl_by_symbol;
    double total_pnl = 0.0;
};

/**
 * @brief Tracks PnL attribution across symbols.
 */
class AttributionTracker {
public:
    /**
     * @brief Update attribution based on portfolio snapshot.
     * @param timestamp Update time.
     * @param portfolio Portfolio state.
     */
    void update(Timestamp timestamp, const engine::Portfolio& portfolio);

    /**
     * @brief Last attribution snapshot.
     */
    const AttributionSnapshot& last() const { return last_; }

private:
    AttributionSnapshot last_;
    std::unordered_map<SymbolId, double> last_values_;
};

}  // namespace regimeflow::metrics
