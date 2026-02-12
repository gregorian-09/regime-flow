/**
 * @file corporate_actions.h
 * @brief RegimeFlow regimeflow corporate actions declarations.
 */

#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/data/bar.h"

#include <map>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Corporate action types supported for adjustments.
 */
enum class CorporateActionType : uint8_t {
    Split,
    Dividend,
    SymbolChange
};

/**
 * @brief Corporate action metadata.
 */
struct CorporateAction {
    CorporateActionType type = CorporateActionType::Split;
    Timestamp effective_date;
    double factor = 1.0;
    double amount = 0.0;
    std::string new_symbol;
};

/**
 * @brief Applies corporate actions to market data and symbol mappings.
 */
class CorporateActionAdjuster {
public:
    /**
     * @brief Add actions for a symbol.
     * @param symbol Symbol ID.
     * @param actions Vector of corporate actions.
     */
    void add_actions(SymbolId symbol, std::vector<CorporateAction> actions);

    /**
     * @brief Adjust a bar for splits/dividends.
     * @param symbol Symbol ID.
     * @param bar Input bar.
     * @return Adjusted bar.
     */
    Bar adjust_bar(SymbolId symbol, const Bar& bar) const;
    /**
     * @brief Resolve the latest symbol after symbol changes.
     * @param symbol Symbol ID.
     * @return Resolved symbol ID.
     */
    SymbolId resolve_symbol(SymbolId symbol) const;
    /**
     * @brief Resolve symbol at a point in time.
     * @param symbol Symbol ID.
     * @param at Timestamp for resolution.
     * @return Resolved symbol ID.
     */
    SymbolId resolve_symbol(SymbolId symbol, Timestamp at) const;
    /**
     * @brief Get all aliases for a symbol.
     * @param symbol Symbol ID.
     * @return Vector of alias IDs.
     */
    std::vector<SymbolId> aliases_for(SymbolId symbol) const;

private:
    std::map<SymbolId, std::vector<CorporateAction>> actions_;
};

}  // namespace regimeflow::data
