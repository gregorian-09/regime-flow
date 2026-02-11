#include "regimeflow/risk/position_sizer.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::risk {

Quantity FixedFractionalSizer::size(const PositionSizingContext& ctx) const {
    if (ctx.equity <= 0.0 || ctx.price <= 0.0 || ctx.stop_price <= 0.0) {
        return 0;
    }
    double risk_capital = ctx.equity * risk_per_trade_;
    double stop_distance = std::abs(ctx.price - ctx.stop_price);
    if (stop_distance <= 0.0) {
        return 0;
    }
    double qty = risk_capital / stop_distance;
    return static_cast<Quantity>(qty);
}

Quantity VolatilityTargetSizer::size(const PositionSizingContext& ctx) const {
    if (ctx.equity <= 0.0 || ctx.price <= 0.0 || ctx.volatility <= 0.0) {
        return 0;
    }
    double qty = (target_vol_ * ctx.equity) / (ctx.volatility * ctx.price);
    return static_cast<Quantity>(qty);
}

Quantity KellySizer::size(const PositionSizingContext& ctx) const {
    if (ctx.equity <= 0.0 || ctx.price <= 0.0 || ctx.win_loss_ratio <= 0.0) {
        return 0;
    }
    double win = ctx.win_rate;
    double loss = 1.0 - win;
    double kelly = (win * ctx.win_loss_ratio - loss) / ctx.win_loss_ratio;
    if (std::isnan(kelly) || std::isinf(kelly)) {
        return 0;
    }
    kelly = std::clamp(kelly, 0.0, max_fraction_);
    double qty = (kelly * ctx.equity) / ctx.price;
    return static_cast<Quantity>(qty);
}

Quantity RegimeScaledSizer::size(const PositionSizingContext& ctx) const {
    if (!base_) {
        return 0;
    }
    PositionSizingContext scaled = ctx;
    scaled.equity *= std::max(0.0, ctx.regime_scale);
    return base_->size(scaled);
}

}  // namespace regimeflow::risk
