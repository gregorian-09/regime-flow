#include "regimeflow/common/types.h"

#include <stdexcept>

namespace regimeflow {

SymbolRegistry& SymbolRegistry::instance() {
    static SymbolRegistry registry;
    return registry;
}

SymbolId SymbolRegistry::intern(std::string_view symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = symbol_to_id_.find(std::string(symbol));
    if (it != symbol_to_id_.end()) {
        return it->second;
    }
    SymbolId id = static_cast<SymbolId>(id_to_symbol_.size());
    id_to_symbol_.emplace_back(symbol);
    symbol_to_id_.emplace(id_to_symbol_.back(), id);
    return id;
}

const std::string& SymbolRegistry::lookup(SymbolId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (id >= id_to_symbol_.size()) {
        throw std::out_of_range("SymbolId out of range");
    }
    return id_to_symbol_[id];
}

}  // namespace regimeflow
