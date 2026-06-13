#include "regimeflow/risk/risk_limits.h"

#include <ranges>
#include <utility>

namespace regimeflow::risk
{
    RegimeRiskOverlayLimit::RegimeRiskOverlayLimit(
        std::unordered_map<std::string, RegimeRiskOverlayProfile> profiles)
        : profiles_(std::move(profiles)) {}

    Result<void> RegimeRiskOverlayLimit::validate(const engine::Order& order,
                                                  const engine::Portfolio& portfolio) const {
        const auto regime = order.metadata.find("regime");
        if (regime == order.metadata.end()) {
            return Ok();
        }
        const auto profile_it = profiles_.find(regime->second);
        if (profile_it == profiles_.end()) {
            return Ok();
        }

        const auto& profile = profile_it->second;
        if (!profile.allow_market_orders
            && (order.type == engine::OrderType::Market
                || order.type == engine::OrderType::MarketOnOpen
                || order.type == engine::OrderType::MarketOnClose)) {
            return Result<void>(Error(Error::Code::OutOfRange,
                                      "Regime risk overlay blocks market orders"));
        }
        if (!profile.allow_aggressive_tif
            && (order.tif == engine::TimeInForce::IOC || order.tif == engine::TimeInForce::FOK)) {
            return Result<void>(Error(Error::Code::OutOfRange,
                                      "Regime risk overlay blocks aggressive time-in-force"));
        }

        const double price = reference_price(order);
        if (price <= 0.0) {
            if (order.type == engine::OrderType::Limit || order.type == engine::OrderType::StopLimit) {
                return Result<void>(Error(Error::Code::InvalidArgument,
                                          "Limit price must be set for regime risk overlay"));
            }
            return Ok();
        }

        const Quantity order_qty = signed_quantity(order);
        const auto position = portfolio.get_position(order.symbol);
        const Quantity existing_qty = position ? position->quantity : 0.0;
        const Quantity projected_qty = existing_qty + order_qty;
        const bool increases_exposure = std::abs(projected_qty) > std::abs(existing_qty);
        if (!profile.allow_new_exposure && increases_exposure) {
            return Result<void>(Error(Error::Code::OutOfRange,
                                      "Regime risk overlay blocks new exposure"));
        }

        const double order_notional = std::abs(order.quantity) * price;
        if (profile.max_order_notional > 0.0 && order_notional > profile.max_order_notional) {
            return Result<void>(Error(Error::Code::OutOfRange,
                                      "Order exceeds regime max notional"));
        }

        const double equity = portfolio.equity();
        if (profile.max_position_pct > 0.0 && equity > 0.0) {
            const double projected_notional = std::abs(projected_qty * price);
            if (projected_notional / equity > profile.max_position_pct) {
                return Result<void>(Error(Error::Code::OutOfRange,
                                          "Order exceeds regime max position pct"));
            }
        }
        return Ok();
    }

    Quantity RegimeRiskOverlayLimit::signed_quantity(const engine::Order& order) {
        const Quantity qty = std::abs(order.quantity);
        return order.side == engine::OrderSide::Buy ? qty : -qty;
    }

    double RegimeRiskOverlayLimit::reference_price(const engine::Order& order) {
        if (order.limit_price > 0.0) {
            return order.limit_price;
        }
        if (order.stop_price > 0.0) {
            return order.stop_price;
        }
        return 0.0;
    }

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
