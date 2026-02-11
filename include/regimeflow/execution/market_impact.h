#pragma once

#include "regimeflow/data/order_book.h"
#include "regimeflow/engine/order.h"

namespace regimeflow::execution {

class MarketImpactModel {
public:
    virtual ~MarketImpactModel() = default;
    virtual double impact_bps(const engine::Order& order,
                              const data::OrderBook* book) const = 0;
};

class ZeroMarketImpactModel final : public MarketImpactModel {
public:
    double impact_bps(const engine::Order&, const data::OrderBook*) const override { return 0.0; }
};

class FixedMarketImpactModel final : public MarketImpactModel {
public:
    explicit FixedMarketImpactModel(double bps) : bps_(bps) {}
    double impact_bps(const engine::Order&, const data::OrderBook*) const override { return bps_; }

private:
    double bps_ = 0.0;
};

class OrderBookImpactModel final : public MarketImpactModel {
public:
    explicit OrderBookImpactModel(double max_bps = 50.0) : max_bps_(max_bps) {}

    double impact_bps(const engine::Order& order,
                      const data::OrderBook* book) const override;

private:
    double max_bps_ = 50.0;
};

}  // namespace regimeflow::execution
