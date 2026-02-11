#include "regimeflow/execution/fill_simulator.h"

namespace regimeflow::execution {

FillSimulator::FillSimulator(std::shared_ptr<SlippageModel> slippage_model)
    : slippage_model_(std::move(slippage_model)) {}

engine::Fill FillSimulator::simulate(const engine::Order& order, Price reference_price,
                                     Timestamp timestamp, bool is_maker) const {
    engine::Fill fill;
    fill.order_id = order.id;
    fill.symbol = order.symbol;
    fill.quantity = order.quantity * (order.side == engine::OrderSide::Buy ? 1.0 : -1.0);
    fill.timestamp = timestamp;
    fill.is_maker = is_maker;
    fill.price = slippage_model_ ? slippage_model_->execution_price(order, reference_price)
                                 : reference_price;
    fill.slippage = fill.price - reference_price;
    return fill;
}

}  // namespace regimeflow::execution
