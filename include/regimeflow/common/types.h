#pragma once

#include "regimeflow/common/time.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace regimeflow {

using SymbolId = uint32_t;
using Price = double;
using Quantity = double;
using Volume = uint64_t;

enum class AssetClass : uint8_t {
    Equity,
    Futures,
    Forex,
    Crypto,
    Options,
    Other
};

class SymbolRegistry {
public:
    static SymbolRegistry& instance();

    SymbolId intern(std::string_view symbol);
    const std::string& lookup(SymbolId id) const;

private:
    SymbolRegistry() = default;

    std::unordered_map<std::string, SymbolId> symbol_to_id_;
    std::vector<std::string> id_to_symbol_;
    mutable std::mutex mutex_;
};

struct TimeRange {
    Timestamp start;
    Timestamp end;

    bool contains(Timestamp t) const { return t >= start && t <= end; }
    Duration duration() const { return end - start; }
};

}  // namespace regimeflow
