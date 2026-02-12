/**
 * @file slippage.h
 * @brief RegimeFlow regimeflow slippage declarations.
 */

#pragma once

#include "regimeflow/engine/order.h"
#include "regimeflow/regime/types.h"

#include <memory>
#include <optional>
#include <unordered_map>

namespace regimeflow::execution {

/**
 * @brief Base class for slippage models.
 */
class SlippageModel {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~SlippageModel() = default;
    /**
     * @brief Compute execution price after slippage.
     * @param order Order being executed.
     * @param reference_price Reference price.
     * @return Execution price.
     */
    virtual Price execution_price(const engine::Order& order, Price reference_price) const = 0;
};

/**
 * @brief Slippage model that returns the reference price.
 */
class ZeroSlippageModel final : public SlippageModel {
public:
    /**
     * @brief Return the reference price.
     */
    Price execution_price(const engine::Order& order, Price reference_price) const override;
};

/**
 * @brief Fixed slippage in basis points.
 */
class FixedBpsSlippageModel final : public SlippageModel {
public:
    /**
     * @brief Construct with fixed basis points.
     * @param bps Slippage in basis points.
     */
    explicit FixedBpsSlippageModel(double bps);

    /**
     * @brief Apply fixed slippage.
     */
    Price execution_price(const engine::Order& order, Price reference_price) const override;

private:
    double bps_ = 0.0;
};

/**
 * @brief Regime-specific slippage in basis points.
 */
class RegimeBpsSlippageModel final : public SlippageModel {
public:
    /**
     * @brief Construct with default and per-regime bps mapping.
     * @param default_bps Default slippage in basis points.
     * @param bps_map Mapping of regime type to bps.
     */
    RegimeBpsSlippageModel(double default_bps,
                           std::unordered_map<regime::RegimeType, double> bps_map);

    /**
     * @brief Apply regime-dependent slippage.
     */
    Price execution_price(const engine::Order& order, Price reference_price) const override;

private:
    static std::optional<regime::RegimeType> parse_regime(const std::string& value);

    double default_bps_ = 0.0;
    std::unordered_map<regime::RegimeType, double> bps_map_;
};

}  // namespace regimeflow::execution
