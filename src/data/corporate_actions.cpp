#include "regimeflow/data/corporate_actions.h"

#include <algorithm>

namespace regimeflow::data
{
    void CorporateActionAdjuster::add_actions(const SymbolId symbol, std::vector<CorporateAction> actions) {
        auto& list = actions_[symbol];
        list.insert(list.end(), std::make_move_iterator(actions.begin()),
                    std::make_move_iterator(actions.end()));
        std::ranges::sort(list, [](const CorporateAction& a, const CorporateAction& b) {
            return a.effective_date < b.effective_date;
        });
    }

    Bar CorporateActionAdjuster::adjust_bar(const SymbolId symbol, const Bar& bar) const {
        Bar adjusted = bar;
        const auto it = actions_.find(symbol);
        if (it == actions_.end()) {
            return adjusted;
        }
        for (const auto& [type, effective_date, factor, amount, new_symbol] : it->second) {
            if (bar.timestamp < effective_date) {
                if (type == CorporateActionType::Split) {
                    if (factor > 0) {
                        adjusted.open /= factor;
                        adjusted.high /= factor;
                        adjusted.low /= factor;
                        adjusted.close /= factor;
                        adjusted.volume = static_cast<Volume>(static_cast<double>(adjusted.volume) * factor);
                    }
                } else if (type == CorporateActionType::Dividend) {
                    double r_factor = factor;
                    if ((r_factor <= 0.0 || r_factor == 1.0) && amount > 0.0 &&
                        adjusted.close > 0.0) {
                        r_factor = (adjusted.close - amount) / adjusted.close;
                        }
                    if (r_factor > 0.0) {
                        adjusted.open *= r_factor;
                        adjusted.high *= r_factor;
                        adjusted.low *= r_factor;
                        adjusted.close *= r_factor;
                    }
                }
            } else if (type == CorporateActionType::SymbolChange) {
                if (!new_symbol.empty() && bar.timestamp >= effective_date) {
                    adjusted.symbol = SymbolRegistry::instance().intern(new_symbol);
                }
            }
        }
        return adjusted;
    }

    SymbolId CorporateActionAdjuster::resolve_symbol(const SymbolId symbol) const {
        if (actions_.contains(symbol)) {
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

    SymbolId CorporateActionAdjuster::resolve_symbol(const SymbolId symbol, Timestamp) const {
        return resolve_symbol(symbol);
    }

    std::vector<SymbolId> CorporateActionAdjuster::aliases_for(const SymbolId symbol) const {
        std::vector<SymbolId> aliases;
        aliases.push_back(symbol);
        const auto it = actions_.find(symbol);
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
