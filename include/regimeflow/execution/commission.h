/**
 * @file commission.h
 * @brief RegimeFlow regimeflow commission declarations.
 */

#pragma once

#include "regimeflow/engine/order.h"

namespace regimeflow::execution {

/**
 * @brief Base class for commission models.
 */
class CommissionModel {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~CommissionModel() = default;
    /**
     * @brief Compute commission for a fill.
     * @param order Order being filled.
     * @param fill Fill details.
     * @return Commission amount.
     */
    virtual double commission(const engine::Order& order, const engine::Fill& fill) const = 0;
};

/**
 * @brief Commission model that always returns zero.
 */
class ZeroCommissionModel final : public CommissionModel {
public:
    /**
     * @brief Return zero commission.
     */
    double commission(const engine::Order&, const engine::Fill&) const override { return 0.0; }
};

/**
 * @brief Fixed commission per fill.
 */
class FixedPerFillCommissionModel final : public CommissionModel {
public:
    /**
     * @brief Construct with a fixed amount.
     * @param amount Commission per fill.
     */
    explicit FixedPerFillCommissionModel(double amount) : amount_(amount) {}
    /**
     * @brief Return the fixed commission.
     */
    double commission(const engine::Order&, const engine::Fill&) const override { return amount_; }

private:
    double amount_ = 0.0;
};

}  // namespace regimeflow::execution
