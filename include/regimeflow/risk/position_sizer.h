#pragma once

#include "regimeflow/common/types.h"

#include <memory>

namespace regimeflow::risk {

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

class PositionSizer {
public:
    virtual ~PositionSizer() = default;
    virtual Quantity size(const PositionSizingContext& ctx) const = 0;
};

class FixedFractionalSizer final : public PositionSizer {
public:
    explicit FixedFractionalSizer(double risk_per_trade) : risk_per_trade_(risk_per_trade) {}

    Quantity size(const PositionSizingContext& ctx) const override;

private:
    double risk_per_trade_ = 0.0;
};

class VolatilityTargetSizer final : public PositionSizer {
public:
    explicit VolatilityTargetSizer(double target_vol) : target_vol_(target_vol) {}

    Quantity size(const PositionSizingContext& ctx) const override;

private:
    double target_vol_ = 0.0;
};

class KellySizer final : public PositionSizer {
public:
    KellySizer(double max_fraction = 1.0) : max_fraction_(max_fraction) {}

    Quantity size(const PositionSizingContext& ctx) const override;

private:
    double max_fraction_ = 1.0;
};

class RegimeScaledSizer final : public PositionSizer {
public:
    explicit RegimeScaledSizer(std::unique_ptr<PositionSizer> base)
        : base_(std::move(base)) {}

    Quantity size(const PositionSizingContext& ctx) const override;

private:
    std::unique_ptr<PositionSizer> base_;
};

}  // namespace regimeflow::risk
