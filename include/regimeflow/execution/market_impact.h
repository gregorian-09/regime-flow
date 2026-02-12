/**
 * @file market_impact.h
 * @brief RegimeFlow regimeflow market impact declarations.
 */

#pragma once

#include "regimeflow/data/order_book.h"
#include "regimeflow/engine/order.h"

namespace regimeflow::execution {

/**
 * @brief Base class for market impact models.
 */
class MarketImpactModel {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~MarketImpactModel() = default;
    /**
     * @brief Estimate impact in basis points.
     * @param order Order being executed.
     * @param book Optional order book snapshot.
     * @return Impact in basis points.
     */
    virtual double impact_bps(const engine::Order& order,
                              const data::OrderBook* book) const = 0;
};

/**
 * @brief Market impact model that returns zero.
 */
class ZeroMarketImpactModel final : public MarketImpactModel {
public:
    /**
     * @brief Return zero impact.
     */
    double impact_bps(const engine::Order&, const data::OrderBook*) const override { return 0.0; }
};

/**
 * @brief Fixed impact in basis points.
 */
class FixedMarketImpactModel final : public MarketImpactModel {
public:
    /**
     * @brief Construct with fixed basis points.
     * @param bps Impact in basis points.
     */
    explicit FixedMarketImpactModel(double bps) : bps_(bps) {}
    /**
     * @brief Return fixed impact.
     */
    double impact_bps(const engine::Order&, const data::OrderBook*) const override { return bps_; }

private:
    double bps_ = 0.0;
};

/**
 * @brief Impact model that uses order book depth to cap impact.
 */
class OrderBookImpactModel final : public MarketImpactModel {
public:
    /**
     * @brief Construct with a maximum impact cap.
     * @param max_bps Maximum impact in basis points.
     */
    explicit OrderBookImpactModel(double max_bps = 50.0) : max_bps_(max_bps) {}

    /**
     * @brief Estimate impact from the order book.
     * @param order Order being executed.
     * @param book Order book snapshot.
     * @return Impact in basis points.
     */
    double impact_bps(const engine::Order& order,
                      const data::OrderBook* book) const override;

private:
    double max_bps_ = 50.0;
};

}  // namespace regimeflow::execution
