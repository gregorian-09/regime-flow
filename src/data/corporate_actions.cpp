#include "regimeflow/data/corporate_actions.h"

#include <algorithm>

namespace regimeflow::data {

void CorporateActionAdjuster::add_actions(SymbolId symbol, std::vector<CorporateAction> actions) {
    auto& list = actions_[symbol];
    list.insert(list.end(), std::make_move_iterator(actions.begin()),
                std::make_move_iterator(actions.end()));
    std::sort(list.begin(), list.end(), [](const CorporateAction& a, const CorporateAction& b) {
        return a.effective_date < b.effective_date;
    });
}

Bar CorporateActionAdjuster::adjust_bar(SymbolId symbol, const Bar& bar) const {
    Bar adjusted = bar;
    auto it = actions_.find(symbol);
    if (it == actions_.end()) {
        return adjusted;
    }
    for (const auto& action : it->second) {
        if (bar.timestamp < action.effective_date) {
            if (action.type == CorporateActionType::Split) {
                if (action.factor > 0) {
                    adjusted.open /= action.factor;
                    adjusted.high /= action.factor;
                    adjusted.low /= action.factor;
                    adjusted.close /= action.factor;
                    adjusted.volume = static_cast<Volume>(adjusted.volume * action.factor);
                }
            } else if (action.type == CorporateActionType::Dividend) {
                double factor = action.factor;
                if ((factor <= 0.0 || factor == 1.0) && action.amount > 0.0 &&
                    adjusted.close > 0.0) {
                    factor = (adjusted.close - action.amount) / adjusted.close;
                }
                if (factor > 0.0) {
                    adjusted.open *= factor;
                    adjusted.high *= factor;
                    adjusted.low *= factor;
                    adjusted.close *= factor;
                }
            }
        } else if (action.type == CorporateActionType::SymbolChange) {
            if (!action.new_symbol.empty() && bar.timestamp >= action.effective_date) {
                adjusted.symbol = SymbolRegistry::instance().intern(action.new_symbol);
            }
        }
    }
    return adjusted;
}

SymbolId CorporateActionAdjuster::resolve_symbol(SymbolId symbol) const {
    if (actions_.find(symbol) != actions_.end()) {
        return symbol;
    }
    const auto& name = SymbolRegistry::instance().lookup(symbol);
    if (name.empty()) {
        return symbol;
    }
    for (const auto& [base_symbol, actions] : actions_) {
        for (const auto& action : actions) {
            if (action.type == CorporateActionType::SymbolChange &&
                action.new_symbol == name) {
                return base_symbol;
            }
        }
    }
    return symbol;
}

SymbolId CorporateActionAdjuster::resolve_symbol(SymbolId symbol, Timestamp) const {
    return resolve_symbol(symbol);
}

std::vector<SymbolId> CorporateActionAdjuster::aliases_for(SymbolId symbol) const {
    std::vector<SymbolId> aliases;
    aliases.push_back(symbol);
    auto it = actions_.find(symbol);
    if (it == actions_.end()) {
        return aliases;
    }
    for (const auto& action : it->second) {
        if (action.type == CorporateActionType::SymbolChange && !action.new_symbol.empty()) {
            aliases.push_back(SymbolRegistry::instance().intern(action.new_symbol));
        }
    }
    return aliases;
}

}  // namespace regimeflow::data
