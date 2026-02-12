/**
 * @file types.h
 * @brief RegimeFlow regimeflow types declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace regimeflow {

/**
 * @brief Numeric identifier for a symbol in the registry.
 */
using SymbolId = uint32_t;
/**
 * @brief Price representation used in models and execution.
 */
using Price = double;
/**
 * @brief Quantity representation used in orders/positions.
 */
using Quantity = double;
/**
 * @brief Volume representation used in market data.
 */
using Volume = uint64_t;

/**
 * @brief Supported asset classes.
 */
enum class AssetClass : uint8_t {
    Equity,
    Futures,
    Forex,
    Crypto,
    Options,
    Other
};

/**
 * @brief Thread-safe registry mapping symbols to compact IDs.
 *
 * @details Used to deduplicate symbol strings and speed up lookups in
 * backtests and live engines.
 */
class SymbolRegistry {
public:
    /**
     * @brief Access the singleton registry.
     * @return Registry instance.
     */
    static SymbolRegistry& instance();

    /**
     * @brief Intern a symbol string and return its ID.
     * @param symbol Symbol string (e.g., "AAPL").
     * @return Stable SymbolId for the symbol.
     */
    SymbolId intern(std::string_view symbol);
    /**
     * @brief Lookup a symbol string by its ID.
     * @param id SymbolId.
     * @return Reference to the stored symbol string.
     * @throws std::out_of_range if the ID is invalid.
     */
    const std::string& lookup(SymbolId id) const;

private:
    SymbolRegistry() = default;

    std::unordered_map<std::string, SymbolId> symbol_to_id_;
    std::vector<std::string> id_to_symbol_;
    mutable std::mutex mutex_;
};

/**
 * @brief Inclusive time range with helper utilities.
 */
struct TimeRange {
    Timestamp start;
    Timestamp end;

    /**
     * @brief Check if a timestamp lies within the range (inclusive).
     * @param t Timestamp to test.
     * @return True if start <= t <= end.
     */
    bool contains(Timestamp t) const { return t >= start && t <= end; }
    /**
     * @brief Compute the duration of the range.
     * @return Duration between end and start.
     */
    Duration duration() const { return end - start; }
};

}  // namespace regimeflow
