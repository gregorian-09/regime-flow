/**
 * @file strategy_factory.h
 * @brief RegimeFlow regimeflow strategy factory declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/strategy/strategy.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace regimeflow::strategy {

/**
 * @brief Factory for creating strategies by name.
 */
class StrategyFactory {
public:
    /**
     * @brief Strategy creator callback type.
     */
    using Creator = std::function<std::unique_ptr<Strategy>(const Config&)>;

    /**
     * @brief Access the singleton factory.
     */
    static StrategyFactory& instance();

    /**
     * @brief Register a strategy creator.
     * @param name Strategy name.
     * @param creator Factory function.
     */
    void register_creator(std::string name, Creator creator);
    /**
     * @brief Create a strategy from config (uses config name).
     * @param config Strategy config.
     * @return Strategy instance or nullptr.
     */
    std::unique_ptr<Strategy> create(const Config& config) const;

private:
    std::unordered_map<std::string, Creator> creators_;
};

/**
 * @brief Register built-in strategies with the factory.
 */
void register_builtin_strategies();

}  // namespace regimeflow::strategy
