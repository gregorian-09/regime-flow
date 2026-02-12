/**
 * @file position_sizer.h
 * @brief RegimeFlow regimeflow position sizer declarations.
 */

#pragma once

#include "regimeflow/common/types.h"

#include <memory>

namespace regimeflow::risk {

/**
 * @brief Inputs for position sizing.
 */
struct PositionSizingContext {
    double equity = 0.0;
    double price = 0.0;
    double stop_price = 0.0;
    double volatility = 0.0;
    double win_rate = 0.0;
    double win_loss_ratio = 0.0;
    double regime_scale = 1.0;
    double risk_per_trade = 0.0;
};

/**
 * @brief Base interface for position sizing models.
 */
class PositionSizer {
public:
    virtual ~PositionSizer() = default;
    /**
     * @brief Compute position size for a context.
     * @param ctx Position sizing context.
     * @return Quantity to trade.
     */
    virtual Quantity size(const PositionSizingContext& ctx) const = 0;
};

/**
 * @brief Fixed-fractional position sizing.
 */
class FixedFractionalSizer final : public PositionSizer {
public:
    /**
     * @brief Construct with risk per trade fraction.
     * @param risk_per_trade Fraction of equity to risk.
     */
    explicit FixedFractionalSizer(double risk_per_trade) : risk_per_trade_(risk_per_trade) {}

    /**
     * @brief Compute size based on fixed risk per trade.
     */
    Quantity size(const PositionSizingContext& ctx) const override;

private:
    double risk_per_trade_ = 0.0;
};

/**
 * @brief Volatility targeting position sizing.
 */
class VolatilityTargetSizer final : public PositionSizer {
public:
    /**
     * @brief Construct with target volatility.
     * @param target_vol Target volatility level.
     */
    explicit VolatilityTargetSizer(double target_vol) : target_vol_(target_vol) {}

    /**
     * @brief Compute size to target volatility.
     */
    Quantity size(const PositionSizingContext& ctx) const override;

private:
    double target_vol_ = 0.0;
};

/**
 * @brief Kelly criterion position sizing.
 */
class KellySizer final : public PositionSizer {
public:
    /**
     * @brief Construct with maximum fraction cap.
     * @param max_fraction Max fraction of equity.
     */
    KellySizer(double max_fraction = 1.0) : max_fraction_(max_fraction) {}

    /**
     * @brief Compute size using Kelly fraction.
     */
    Quantity size(const PositionSizingContext& ctx) const override;

private:
    double max_fraction_ = 1.0;
};

/**
 * @brief Regime-scaled position sizing wrapper.
 */
class RegimeScaledSizer final : public PositionSizer {
public:
    /**
     * @brief Construct with a base sizer.
     * @param base Base position sizer.
     */
    explicit RegimeScaledSizer(std::unique_ptr<PositionSizer> base)
        : base_(std::move(base)) {}

    /**
     * @brief Compute size scaled by regime.
     */
    Quantity size(const PositionSizingContext& ctx) const override;

private:
    std::unique_ptr<PositionSizer> base_;
};

}  // namespace regimeflow::risk
