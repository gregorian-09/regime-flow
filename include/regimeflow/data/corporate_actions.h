#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/data/bar.h"

#include <map>
#include <vector>

namespace regimeflow::data {

enum class CorporateActionType : uint8_t {
    Split,
    Dividend,
    SymbolChange
};

struct CorporateAction {
    CorporateActionType type = CorporateActionType::Split;
    Timestamp effective_date;
    double factor = 1.0;
    double amount = 0.0;
    std::string new_symbol;
};

class CorporateActionAdjuster {
public:
    void add_actions(SymbolId symbol, std::vector<CorporateAction> actions);

    Bar adjust_bar(SymbolId symbol, const Bar& bar) const;
    SymbolId resolve_symbol(SymbolId symbol) const;
    SymbolId resolve_symbol(SymbolId symbol, Timestamp at) const;
    std::vector<SymbolId> aliases_for(SymbolId symbol) const;

private:
    std::map<SymbolId, std::vector<CorporateAction>> actions_;
};

}  // namespace regimeflow::data
