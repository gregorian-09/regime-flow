/**
 * @file transaction_cost.h
 * @brief RegimeFlow regimeflow transaction cost declarations.
 */

#pragma once

#include "regimeflow/engine/order.h"

#include <cmath>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace regimeflow::execution {

/**
 * @brief Base class for transaction cost models.
 */
class TransactionCostModel {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~TransactionCostModel() = default;
    /**
     * @brief Compute transaction cost for a fill.
     * @param order Order being filled.
     * @param fill Fill details.
     * @return Cost in currency units.
     */
    virtual double cost(const engine::Order& order, const engine::Fill& fill) const = 0;
};

/**
 * @brief Zero transaction cost model.
 */
class ZeroTransactionCostModel final : public TransactionCostModel {
public:
    /**
     * @brief Return zero cost.
     */
    double cost(const engine::Order&, const engine::Fill&) const override { return 0.0; }
};

/**
 * @brief Fixed basis-points transaction cost.
 */
class FixedBpsTransactionCostModel final : public TransactionCostModel {
public:
    /**
     * @brief Construct with fixed bps.
     * @param bps Cost in basis points.
     */
    explicit FixedBpsTransactionCostModel(double bps) : bps_(bps) {}
    /**
     * @brief Compute bps-based cost.
     */
    double cost(const engine::Order&, const engine::Fill& fill) const override {
        return std::abs(fill.price * fill.quantity) * (bps_ / 10000.0);
    }

private:
    double bps_ = 0.0;
};

/**
 * @brief Per-share transaction cost model.
 */
class PerShareTransactionCostModel final : public TransactionCostModel {
public:
    /**
     * @brief Construct with a per-share rate.
     * @param rate_per_share Cost per share.
     */
    explicit PerShareTransactionCostModel(double rate_per_share) : rate_per_share_(rate_per_share) {}
    /**
     * @brief Compute cost from quantity.
     */
    double cost(const engine::Order&, const engine::Fill& fill) const override {
        return std::abs(fill.quantity) * rate_per_share_;
    }

private:
    double rate_per_share_ = 0.0;
};

/**
 * @brief Per-order transaction cost model.
 */
class PerOrderTransactionCostModel final : public TransactionCostModel {
public:
    /**
     * @brief Construct with a per-order cost.
     * @param cost_per_order Cost per unique order.
     */
    explicit PerOrderTransactionCostModel(double cost_per_order) : cost_per_order_(cost_per_order) {}
    /**
     * @brief Charge once per order ID.
     */
    double cost(const engine::Order& order, const engine::Fill&) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (charged_orders_.insert(order.id).second) {
            return cost_per_order_;
        }
        return 0.0;
    }

private:
    double cost_per_order_ = 0.0;
    mutable std::mutex mutex_;
    mutable std::unordered_set<engine::OrderId> charged_orders_;
};

/**
 * @brief Single tier entry for tiered bps costs.
 */
struct TieredTransactionCostTier {
    double max_notional = 0.0;
    double bps = 0.0;
};

/**
 * @brief Tiered basis-points transaction cost model.
 */
class TieredBpsTransactionCostModel final : public TransactionCostModel {
public:
    /**
     * @brief Construct with tier definitions.
     * @param tiers Ordered tiers by increasing max_notional.
     */
    explicit TieredBpsTransactionCostModel(std::vector<TieredTransactionCostTier> tiers)
        : tiers_(std::move(tiers)) {}

    /**
     * @brief Compute tiered bps cost based on notional.
     */
    double cost(const engine::Order&, const engine::Fill& fill) const override {
        if (tiers_.empty()) {
            return 0.0;
        }
        double notional = std::abs(fill.price * fill.quantity);
        double bps = tiers_.back().bps;
        for (const auto& tier : tiers_) {
            if (tier.max_notional <= 0.0 || notional <= tier.max_notional) {
                bps = tier.bps;
                break;
            }
        }
        return notional * (bps / 10000.0);
    }

private:
    std::vector<TieredTransactionCostTier> tiers_;
};

}  // namespace regimeflow::execution
