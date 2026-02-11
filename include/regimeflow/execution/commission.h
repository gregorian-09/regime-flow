#pragma once

#include "regimeflow/engine/order.h"

namespace regimeflow::execution {

class CommissionModel {
public:
    virtual ~CommissionModel() = default;
    virtual double commission(const engine::Order& order, const engine::Fill& fill) const = 0;
};

class ZeroCommissionModel final : public CommissionModel {
public:
    double commission(const engine::Order&, const engine::Fill&) const override { return 0.0; }
};

class FixedPerFillCommissionModel final : public CommissionModel {
public:
    explicit FixedPerFillCommissionModel(double amount) : amount_(amount) {}
    double commission(const engine::Order&, const engine::Fill&) const override { return amount_; }

private:
    double amount_ = 0.0;
};

}  // namespace regimeflow::execution
