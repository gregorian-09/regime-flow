#include "regimeflow/risk/risk_limits.h"

namespace regimeflow::risk {

void RiskManager::add_limit(std::unique_ptr<RiskLimit> limit) {
    if (limit) {
        limits_.push_back(std::move(limit));
    }
}

Result<void> RiskManager::validate(const engine::Order& order,
                                   const engine::Portfolio& portfolio) const {
    auto it = order.metadata.find("risk_exit");
    if (it != order.metadata.end()) {
        return Ok();
    }
    auto regime_it = order.metadata.find("regime");
    if (regime_it != order.metadata.end()) {
        auto rl = regime_limits_.find(regime_it->second);
        if (rl != regime_limits_.end()) {
            for (const auto& limit : rl->second) {
                if (auto result = limit->validate(order, portfolio); result.is_err()) {
                    return result;
                }
            }
        }
    }
    for (const auto& limit : limits_) {
        if (auto result = limit->validate(order, portfolio); result.is_err()) {
            return result;
        }
    }
    return Ok();
}

Result<void> RiskManager::validate_portfolio(const engine::Portfolio& portfolio) const {
    for (const auto& [_, limits] : regime_limits_) {
        for (const auto& limit : limits) {
            if (auto result = limit->validate_portfolio(portfolio); result.is_err()) {
                return result;
            }
        }
    }
    for (const auto& limit : limits_) {
        if (auto result = limit->validate_portfolio(portfolio); result.is_err()) {
            return result;
        }
    }
    return Ok();
}

}  // namespace regimeflow::risk
