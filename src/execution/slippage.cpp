#include "regimeflow/execution/slippage.h"

namespace regimeflow::execution {

Price ZeroSlippageModel::execution_price(const engine::Order&, Price reference_price) const {
    return reference_price;
}

FixedBpsSlippageModel::FixedBpsSlippageModel(double bps) : bps_(bps) {}

Price FixedBpsSlippageModel::execution_price(const engine::Order& order, Price reference_price) const {
    const double sign = (order.side == engine::OrderSide::Buy) ? 1.0 : -1.0;
    const double factor = 1.0 + sign * (bps_ / 10000.0);
    return reference_price * factor;
}

RegimeBpsSlippageModel::RegimeBpsSlippageModel(
    double default_bps, std::unordered_map<regime::RegimeType, double> bps_map)
    : default_bps_(default_bps), bps_map_(std::move(bps_map)) {}

std::optional<regime::RegimeType> RegimeBpsSlippageModel::parse_regime(
    const std::string& value) {
    if (value == "bull") return regime::RegimeType::Bull;
    if (value == "neutral") return regime::RegimeType::Neutral;
    if (value == "bear") return regime::RegimeType::Bear;
    if (value == "crisis") return regime::RegimeType::Crisis;
    return std::nullopt;
}

Price RegimeBpsSlippageModel::execution_price(const engine::Order& order,
                                              Price reference_price) const {
    double bps = default_bps_;
    auto it = order.metadata.find("regime");
    if (it != order.metadata.end()) {
        if (auto parsed = parse_regime(it->second)) {
            auto map_it = bps_map_.find(*parsed);
            if (map_it != bps_map_.end()) {
                bps = map_it->second;
            }
        }
    }
    const double sign = (order.side == engine::OrderSide::Buy) ? 1.0 : -1.0;
    const double factor = 1.0 + sign * (bps / 10000.0);
    return reference_price * factor;
}

}  // namespace regimeflow::execution
