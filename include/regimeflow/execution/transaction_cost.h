#pragma once

#include "regimeflow/engine/order.h"

#include <cmath>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace regimeflow::execution {

class TransactionCostModel {
public:
    virtual ~TransactionCostModel() = default;
    virtual double cost(const engine::Order& order, const engine::Fill& fill) const = 0;
};

class ZeroTransactionCostModel final : public TransactionCostModel {
public:
    double cost(const engine::Order&, const engine::Fill&) const override { return 0.0; }
};

class FixedBpsTransactionCostModel final : public TransactionCostModel {
public:
    explicit FixedBpsTransactionCostModel(double bps) : bps_(bps) {}
    double cost(const engine::Order&, const engine::Fill& fill) const override {
        return std::abs(fill.price * fill.quantity) * (bps_ / 10000.0);
    }

private:
    double bps_ = 0.0;
};

class PerShareTransactionCostModel final : public TransactionCostModel {
public:
    explicit PerShareTransactionCostModel(double rate_per_share) : rate_per_share_(rate_per_share) {}
    double cost(const engine::Order&, const engine::Fill& fill) const override {
        return std::abs(fill.quantity) * rate_per_share_;
    }

private:
    double rate_per_share_ = 0.0;
};

class PerOrderTransactionCostModel final : public TransactionCostModel {
public:
    explicit PerOrderTransactionCostModel(double cost_per_order) : cost_per_order_(cost_per_order) {}
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

struct TieredTransactionCostTier {
    double max_notional = 0.0;
    double bps = 0.0;
};

class TieredBpsTransactionCostModel final : public TransactionCostModel {
public:
    explicit TieredBpsTransactionCostModel(std::vector<TieredTransactionCostTier> tiers)
        : tiers_(std::move(tiers)) {}

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
