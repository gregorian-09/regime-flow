#include <gtest/gtest.h>

#include "regimeflow/risk/position_sizer.h"

namespace regimeflow::test {

TEST(PositionSizers, FixedFractionalSizesByStopDistance) {
    regimeflow::risk::FixedFractionalSizer sizer(0.01);
    regimeflow::risk::PositionSizingContext ctx;
    ctx.equity = 100000;
    ctx.price = 100;
    ctx.stop_price = 95;
    auto qty = sizer.size(ctx);
    EXPECT_EQ(qty, 200); // risk 1% = 1000, stop distance = 5
}

TEST(PositionSizers, VolatilityTargetSizer) {
    regimeflow::risk::VolatilityTargetSizer sizer(0.15);
    regimeflow::risk::PositionSizingContext ctx;
    ctx.equity = 100000;
    ctx.price = 50;
    ctx.volatility = 0.25;
    auto qty = sizer.size(ctx);
    EXPECT_EQ(qty, 1200); // (0.15*100000)/(0.25*50)=1200
}

TEST(PositionSizers, KellySizerClamped) {
    regimeflow::risk::KellySizer sizer(0.25);
    regimeflow::risk::PositionSizingContext ctx;
    ctx.equity = 100000;
    ctx.price = 100;
    ctx.win_rate = 0.6;
    ctx.win_loss_ratio = 1.5;
    auto qty = sizer.size(ctx);
    // Kelly fraction = (0.6*1.5-0.4)/1.5 = 0.333..., clamped to 0.25
    EXPECT_EQ(qty, 250); // 0.25*100000/100
}

TEST(PositionSizers, RegimeScaledSizer) {
    auto base = std::make_unique<regimeflow::risk::FixedFractionalSizer>(0.01);
    regimeflow::risk::RegimeScaledSizer sizer(std::move(base));
    regimeflow::risk::PositionSizingContext ctx;
    ctx.equity = 100000;
    ctx.price = 100;
    ctx.stop_price = 95;
    ctx.regime_scale = 0.5;
    auto qty = sizer.size(ctx);
    EXPECT_EQ(qty, 100); // base 200 scaled by 0.5
}

}  // namespace regimeflow::test
