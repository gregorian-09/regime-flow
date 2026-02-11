#include <gtest/gtest.h>

#include "regimeflow/engine/portfolio.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/risk/risk_limits.h"

#include <unordered_map>

namespace regimeflow::test {

TEST(RiskLimits, MaxGrossExposureLimitBlocksLargeOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        regimeflow::SymbolRegistry::instance().intern("AAA"),
        regimeflow::engine::OrderSide::Buy, 100, 100);

    regimeflow::risk::MaxGrossExposureLimit limit(5000);
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, MaxLeverageLimitBlocksOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        regimeflow::SymbolRegistry::instance().intern("AAA"),
        regimeflow::engine::OrderSide::Buy, 200, 100);

    regimeflow::risk::MaxLeverageLimit limit(0.1); // leverage cap 0.1
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, MaxDrawdownLimitBlocksOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    regimeflow::engine::Fill fill;
    fill.symbol = symbol;
    fill.price = 100.0;
    fill.quantity = 100.0;
    fill.timestamp = regimeflow::Timestamp(1);
    portfolio.update_position(fill);

    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        symbol, regimeflow::engine::OrderSide::Buy, 10, 100);

    regimeflow::risk::MaxDrawdownLimit limit(0.03); // 3%
    auto ok = limit.validate(order, portfolio);
    EXPECT_TRUE(ok.is_ok());

    portfolio.mark_to_market(symbol, 50.0, regimeflow::Timestamp(2));
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, MaxPositionPctLimitBlocksOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        symbol, regimeflow::engine::OrderSide::Buy, 200, 100);

    regimeflow::risk::MaxPositionPctLimit limit(0.1); // 10% of equity
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, MaxNetExposureLimitBlocksOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        symbol, regimeflow::engine::OrderSide::Buy, 200, 100);

    regimeflow::risk::MaxNetExposureLimit limit(5000); // $5k net exposure cap
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, SectorExposureLimitBlocksOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    std::unordered_map<std::string, double> limits{{"Tech", 0.1}};
    std::unordered_map<std::string, std::string> map{{"AAA", "Tech"}};
    regimeflow::risk::MaxSectorExposureLimit limit(limits, map);

    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        symbol, regimeflow::engine::OrderSide::Buy, 200, 100);
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, IndustryExposureLimitBlocksOrder) {
    regimeflow::engine::Portfolio portfolio(100000);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");

    std::unordered_map<std::string, double> limits{{"Software", 0.1}};
    std::unordered_map<std::string, std::string> map{{"AAA", "Software"}};
    regimeflow::risk::MaxIndustryExposureLimit limit(limits, map);

    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        symbol, regimeflow::engine::OrderSide::Buy, 200, 100);
    auto result = limit.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, CorrelationExposureLimitBlocksPortfolio) {
    regimeflow::engine::Portfolio portfolio(100000);
    auto symA = regimeflow::SymbolRegistry::instance().intern("AAA");
    auto symB = regimeflow::SymbolRegistry::instance().intern("BBB");

    regimeflow::engine::Fill fillA;
    fillA.symbol = symA;
    fillA.price = 100.0;
    fillA.quantity = 100.0;
    fillA.timestamp = regimeflow::Timestamp(1);
    portfolio.update_position(fillA);

    regimeflow::engine::Fill fillB;
    fillB.symbol = symB;
    fillB.price = 50.0;
    fillB.quantity = 100.0;
    fillB.timestamp = regimeflow::Timestamp(1);
    portfolio.update_position(fillB);

    regimeflow::risk::MaxCorrelationExposureLimit::Config cfg;
    cfg.window = 5;
    cfg.max_corr = 0.5;
    cfg.max_pair_exposure_pct = 0.1;
    regimeflow::risk::MaxCorrelationExposureLimit limit(cfg);

    for (int i = 0; i < 6; ++i) {
        portfolio.mark_to_market(symA, 100.0 + i, regimeflow::Timestamp(2 + i));
        portfolio.mark_to_market(symB, 50.0 + i * 0.5, regimeflow::Timestamp(2 + i));
        limit.validate_portfolio(portfolio);
    }

    auto result = limit.validate_portfolio(portfolio);
    EXPECT_TRUE(result.is_err());
}

TEST(RiskLimits, RegimeAwareLimitsBlockOrder) {
    regimeflow::risk::RiskManager manager;
    std::unordered_map<std::string, std::vector<std::unique_ptr<regimeflow::risk::RiskLimit>>> map;
    std::vector<std::unique_ptr<regimeflow::risk::RiskLimit>> limits;
    limits.push_back(std::make_unique<regimeflow::risk::MaxNotionalLimit>(1000.0));
    map["bull"] = std::move(limits);
    manager.set_regime_limits(std::move(map));

    regimeflow::engine::Portfolio portfolio(100000);
    auto symbol = regimeflow::SymbolRegistry::instance().intern("AAA");
    regimeflow::engine::Order order = regimeflow::engine::Order::limit(
        symbol, regimeflow::engine::OrderSide::Buy, 100, 100);
    order.metadata["regime"] = "bull";

    auto result = manager.validate(order, portfolio);
    EXPECT_TRUE(result.is_err());
}

}  // namespace regimeflow::test
