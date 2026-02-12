/**
 * @file fill_simulator.h
 * @brief RegimeFlow regimeflow fill simulator declarations.
 */

#pragma once

#include "regimeflow/execution/slippage.h"

#include <memory>

namespace regimeflow::execution {

/**
 * @brief Simulates fills with slippage.
 */
class FillSimulator {
public:
    /**
     * @brief Construct with a slippage model.
     * @param slippage_model Slippage model instance.
     */
    explicit FillSimulator(std::shared_ptr<SlippageModel> slippage_model);

    /**
     * @brief Simulate a single fill.
     * @param order Order to fill.
     * @param reference_price Reference price.
     * @param timestamp Fill timestamp.
     * @param is_maker Whether the fill is maker-side.
     * @return Simulated fill.
     */
    engine::Fill simulate(const engine::Order& order, Price reference_price, Timestamp timestamp,
                          bool is_maker = false) const;

private:
    std::shared_ptr<SlippageModel> slippage_model_;
};

}  // namespace regimeflow::execution
