#include "regimeflow/risk/risk_limits.h"

#include <ranges>

namespace regimeflow::risk
{
    void RiskManager::add_limit(std::unique_ptr<RiskLimit> limit) {
        if (limit) {
            limits_.push_back(std::move(limit));
        }
    }

    Result<void> RiskManager::validate(const engine::Order& order,
                                       const engine::Portfolio& portfolio) const {
        if (const auto it = order.metadata.find("risk_exit"); it != order.metadata.end()) {
            return Ok();
        }
        if (const auto regime_it = order.metadata.find("regime"); regime_it != order.metadata.end()) {
            if (const auto rl = regime_limits_.find(regime_it->second); rl != regime_limits_.end()) {
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
        for (const auto& limits : regime_limits_ | std::views::values) {
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
