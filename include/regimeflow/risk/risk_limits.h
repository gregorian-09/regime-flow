/**
 * @file risk_limits.h
 * @brief RegimeFlow regimeflow risk limits declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/engine/order.h"

#include <cmath>
#include <deque>
#include <memory>
#include <vector>
#include <unordered_map>

namespace regimeflow::risk {

/**
 * @brief Base interface for risk limits.
 */
class RiskLimit {
public:
    virtual ~RiskLimit() = default;
    /**
     * @brief Validate an order against the limit.
     * @param order Order to validate.
     * @param portfolio Current portfolio.
     * @return Ok if allowed, error otherwise.
     */
    virtual Result<void> validate(const engine::Order& order,
                                  const engine::Portfolio& portfolio) const = 0;
    /**
     * @brief Validate portfolio state against the limit.
     * @param portfolio Current portfolio.
     * @return Ok if allowed, error otherwise.
     */
    virtual Result<void> validate_portfolio(const engine::Portfolio& portfolio) const {
        (void)portfolio;
        return Ok();
    }
};

/**
 * @brief Limit on per-order notional size.
 */
class MaxNotionalLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum notional value.
     * @param max_notional Maximum notional.
     */
    explicit MaxNotionalLimit(double max_notional) : max_notional_(max_notional) {}

    /**
     * @brief Validate order notional.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        if (order.limit_price <= 0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for notional checks");
        }
        double price = order.limit_price > 0 ? order.limit_price : 0.0;
        if (price <= 0) {
            return Ok();
        }
        double order_notional = std::abs(order.quantity) * price;
        if (order_notional > max_notional_) {
            return Error(Error::Code::OutOfRange, "Order exceeds max notional limit");
        }
        if (order_notional > portfolio.equity()) {
            return Error(Error::Code::OutOfRange, "Order exceeds portfolio equity");
        }
        return Ok();
    }

private:
    double max_notional_ = 0.0;
};

/**
 * @brief Limit on absolute position size.
 */
class MaxPositionLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum quantity.
     * @param max_quantity Maximum absolute quantity.
     */
    explicit MaxPositionLimit(Quantity max_quantity) : max_quantity_(max_quantity) {}

    /**
     * @brief Validate projected position size.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        auto pos = portfolio.get_position(order.symbol);
        Quantity existing = pos ? pos->quantity : 0;
        Quantity projected = existing + order.quantity;
        if (std::abs(projected) > max_quantity_) {
            return Error(Error::Code::OutOfRange, "Order exceeds max position limit");
        }
        return Ok();
    }

    /**
     * @brief Validate existing positions against the limit.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        for (const auto& position : portfolio.get_all_positions()) {
            if (std::abs(position.quantity) > max_quantity_) {
                return Error(Error::Code::OutOfRange, "Position exceeds max position limit");
            }
        }
        return Ok();
    }

private:
    Quantity max_quantity_ = 0;
};

/**
 * @brief Limit on position size as a percentage of equity.
 */
class MaxPositionPctLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum percent.
     * @param max_pct Maximum fraction of equity.
     */
    explicit MaxPositionPctLimit(double max_pct) : max_pct_(max_pct) {}

    /**
     * @brief Validate projected position percentage.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        double price = order.limit_price;
        if (price <= 0.0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for pct checks");
        }
        if (price <= 0.0) {
            return Ok();
        }
        auto pos = portfolio.get_position(order.symbol);
        Quantity existing = pos ? pos->quantity : 0;
        Quantity projected = existing + order.quantity;
        double notional = std::abs(static_cast<double>(projected) * price);
        double pct = notional / equity;
        if (pct > max_pct_) {
            return Error(Error::Code::OutOfRange, "Order exceeds max position pct limit");
        }
        return Ok();
    }

    /**
     * @brief Validate existing positions against percent limit.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        for (const auto& position : portfolio.get_all_positions()) {
            if (position.current_price <= 0.0) {
                continue;
            }
            double notional = std::abs(position.quantity * position.current_price);
            double pct = notional / equity;
            if (pct > max_pct_) {
                return Error(Error::Code::OutOfRange, "Position exceeds max position pct limit");
            }
        }
        return Ok();
    }

private:
    double max_pct_ = 0.0;
};

/**
 * @brief Limit on maximum drawdown.
 */
class MaxDrawdownLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum drawdown.
     * @param max_drawdown Maximum drawdown fraction.
     */
    explicit MaxDrawdownLimit(double max_drawdown) : max_drawdown_(max_drawdown) {}

    /**
     * @brief Validate drawdown against limit.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        (void)order;
        double equity = portfolio.equity();
        if (equity <= 0) {
            return Ok();
        }
        if (peak_ < equity) {
            peak_ = equity;
        }
        double drawdown = (peak_ - equity) / peak_;
        if (drawdown > max_drawdown_) {
            return Error(Error::Code::OutOfRange, "Max drawdown limit exceeded");
        }
        return Ok();
    }

    /**
     * @brief Validate portfolio drawdown against limit.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        double equity = portfolio.equity();
        if (equity <= 0) {
            return Ok();
        }
        if (peak_ < equity) {
            peak_ = equity;
        }
        double drawdown = (peak_ - equity) / peak_;
        if (drawdown > max_drawdown_) {
            return Error(Error::Code::OutOfRange, "Max drawdown limit exceeded");
        }
        return Ok();
    }

private:
    mutable double peak_ = 0.0;
    double max_drawdown_ = 0.0;
};

/**
 * @brief Limit on gross exposure.
 */
class MaxGrossExposureLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum gross exposure.
     * @param max_gross_exposure Maximum exposure value.
     */
    explicit MaxGrossExposureLimit(double max_gross_exposure)
        : max_gross_exposure_(max_gross_exposure) {}

    /**
     * @brief Validate projected gross exposure.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        if (order.limit_price <= 0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for exposure checks");
        }
        double price = order.limit_price > 0 ? order.limit_price : 0.0;
        if (price <= 0) {
            return Ok();
        }
        double projected = portfolio.gross_exposure() + std::abs(order.quantity) * price;
        if (projected > max_gross_exposure_) {
            return Error(Error::Code::OutOfRange, "Order exceeds max gross exposure limit");
        }
        return Ok();
    }

    /**
     * @brief Validate current gross exposure.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        if (portfolio.gross_exposure() > max_gross_exposure_) {
            return Error(Error::Code::OutOfRange, "Max gross exposure limit exceeded");
        }
        return Ok();
    }

private:
    double max_gross_exposure_ = 0.0;
};

/**
 * @brief Limit on leverage.
 */
class MaxLeverageLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum leverage.
     * @param max_leverage Maximum leverage ratio.
     */
    explicit MaxLeverageLimit(double max_leverage) : max_leverage_(max_leverage) {}

    /**
     * @brief Validate projected leverage.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        if (order.limit_price <= 0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for leverage checks");
        }
        double price = order.limit_price > 0 ? order.limit_price : 0.0;
        if (price <= 0) {
            return Ok();
        }
        double equity = portfolio.equity();
        if (equity <= 0) {
            return Ok();
        }
        double projected = portfolio.gross_exposure() + std::abs(order.quantity) * price;
        double leverage = projected / equity;
        if (leverage > max_leverage_) {
            return Error(Error::Code::OutOfRange, "Order exceeds max leverage limit");
        }
        return Ok();
    }

    /**
     * @brief Validate current leverage.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        double equity = portfolio.equity();
        if (equity <= 0) {
            return Ok();
        }
        double leverage = portfolio.gross_exposure() / equity;
        if (leverage > max_leverage_) {
            return Error(Error::Code::OutOfRange, "Max leverage limit exceeded");
        }
        return Ok();
    }

private:
    double max_leverage_ = 0.0;
};

/**
 * @brief Limit on net exposure.
 */
class MaxNetExposureLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with a maximum net exposure.
     * @param max_net_exposure Maximum net exposure value.
     */
    explicit MaxNetExposureLimit(double max_net_exposure)
        : max_net_exposure_(max_net_exposure) {}

    /**
     * @brief Validate projected net exposure.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        double price = order.limit_price;
        if (price <= 0.0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for exposure checks");
        }
        if (price <= 0.0) {
            return Ok();
        }
        double projected = portfolio.net_exposure() + (order.quantity * price);
        if (std::abs(projected) > max_net_exposure_) {
            return Error(Error::Code::OutOfRange, "Order exceeds max net exposure limit");
        }
        return Ok();
    }

    /**
     * @brief Validate current net exposure.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        if (std::abs(portfolio.net_exposure()) > max_net_exposure_) {
            return Error(Error::Code::OutOfRange, "Max net exposure limit exceeded");
        }
        return Ok();
    }

private:
    double max_net_exposure_ = 0.0;
};

/**
 * @brief Aggregates multiple risk limits and validates orders/portfolio.
 */
class RiskManager {
public:
    /**
     * @brief Add a risk limit to the manager.
     * @param limit Risk limit instance.
     */
    void add_limit(std::unique_ptr<RiskLimit> limit);

    /**
     * @brief Validate an order against all limits.
     * @param order Order to validate.
     * @param portfolio Current portfolio.
     * @return Ok if all limits pass.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const;
    /**
     * @brief Validate portfolio against all limits.
     * @param portfolio Current portfolio.
     * @return Ok if all limits pass.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const;

    /**
     * @brief Set regime-specific limit sets.
     * @param limits Map of regime name to limits.
     */
    void set_regime_limits(
        std::unordered_map<std::string, std::vector<std::unique_ptr<RiskLimit>>> limits) {
        regime_limits_ = std::move(limits);
    }

private:
    std::vector<std::unique_ptr<RiskLimit>> limits_;
    std::unordered_map<std::string, std::vector<std::unique_ptr<RiskLimit>>> regime_limits_;
};

/**
 * @brief Limit on sector exposure as a fraction of equity.
 */
class MaxSectorExposureLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with sector limits and symbol mapping.
     * @param limits Sector -> max pct of equity.
     * @param symbol_to_sector Symbol -> sector mapping.
     */
    MaxSectorExposureLimit(std::unordered_map<std::string, double> limits,
                           std::unordered_map<std::string, std::string> symbol_to_sector)
        : limits_(std::move(limits)), symbol_to_sector_(std::move(symbol_to_sector)) {}

    /**
     * @brief Validate sector exposure after a new order.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        auto sector = sector_for(order.symbol);
        if (sector.empty()) {
            return Ok();
        }
        auto it = limits_.find(sector);
        if (it == limits_.end()) {
            return Ok();
        }
        double limit = it->second;
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        double price = order.limit_price;
        if (price <= 0.0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for sector checks");
        }
        if (price <= 0.0) {
            return Ok();
        }
        double sector_notional = sector_exposure(portfolio, sector);
        double projected = sector_notional + std::abs(order.quantity) * price;
        if (projected / equity > limit) {
            return Error(Error::Code::OutOfRange, "Order exceeds sector exposure limit");
        }
        return Ok();
    }

    /**
     * @brief Validate current sector exposures.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        for (const auto& [sector, limit] : limits_) {
            double exposure = sector_exposure(portfolio, sector);
            if (exposure / equity > limit) {
                return Error(Error::Code::OutOfRange, "Sector exposure limit exceeded");
            }
        }
        return Ok();
    }

private:
    std::string sector_for(SymbolId symbol) const {
        auto name = SymbolRegistry::instance().lookup(symbol);
        auto it = symbol_to_sector_.find(name);
        if (it == symbol_to_sector_.end()) {
            return {};
        }
        return it->second;
    }

    double sector_exposure(const engine::Portfolio& portfolio, const std::string& sector) const {
        double exposure = 0.0;
        for (const auto& pos : portfolio.get_all_positions()) {
            auto s = sector_for(pos.symbol);
            if (s == sector && pos.current_price > 0.0) {
                exposure += std::abs(pos.quantity * pos.current_price);
            }
        }
        return exposure;
    }

    std::unordered_map<std::string, double> limits_;
    std::unordered_map<std::string, std::string> symbol_to_sector_;
};

/**
 * @brief Limit on industry exposure as a fraction of equity.
 */
class MaxIndustryExposureLimit final : public RiskLimit {
public:
    /**
     * @brief Construct with industry limits and symbol mapping.
     * @param limits Industry -> max pct of equity.
     * @param symbol_to_industry Symbol -> industry mapping.
     */
    MaxIndustryExposureLimit(std::unordered_map<std::string, double> limits,
                             std::unordered_map<std::string, std::string> symbol_to_industry)
        : limits_(std::move(limits)), symbol_to_industry_(std::move(symbol_to_industry)) {}

    /**
     * @brief Validate industry exposure after a new order.
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        auto industry = industry_for(order.symbol);
        if (industry.empty()) {
            return Ok();
        }
        auto it = limits_.find(industry);
        if (it == limits_.end()) {
            return Ok();
        }
        double limit = it->second;
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        double price = order.limit_price;
        if (price <= 0.0 && order.type == engine::OrderType::Limit) {
            return Error(Error::Code::InvalidArgument, "Limit price must be set for industry checks");
        }
        if (price <= 0.0) {
            return Ok();
        }
        double notional = industry_exposure(portfolio, industry) + std::abs(order.quantity) * price;
        if (notional / equity > limit) {
            return Error(Error::Code::OutOfRange, "Order exceeds industry exposure limit");
        }
        return Ok();
    }

    /**
     * @brief Validate current industry exposures.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        for (const auto& [industry, limit] : limits_) {
            double exposure = industry_exposure(portfolio, industry);
            if (exposure / equity > limit) {
                return Error(Error::Code::OutOfRange, "Industry exposure limit exceeded");
            }
        }
        return Ok();
    }

private:
    std::string industry_for(SymbolId symbol) const {
        auto name = SymbolRegistry::instance().lookup(symbol);
        auto it = symbol_to_industry_.find(name);
        if (it == symbol_to_industry_.end()) {
            return {};
        }
        return it->second;
    }

    double industry_exposure(const engine::Portfolio& portfolio,
                             const std::string& industry) const {
        double exposure = 0.0;
        for (const auto& pos : portfolio.get_all_positions()) {
            auto s = industry_for(pos.symbol);
            if (s == industry && pos.current_price > 0.0) {
                exposure += std::abs(pos.quantity * pos.current_price);
            }
        }
        return exposure;
    }

    std::unordered_map<std::string, double> limits_;
    std::unordered_map<std::string, std::string> symbol_to_industry_;
};

/**
 * @brief Limit on exposure to highly correlated pairs.
 */
class MaxCorrelationExposureLimit final : public RiskLimit {
public:
    /**
     * @brief Configuration for correlation exposure limit.
     */
    struct Config {
        size_t window = 50;
        double max_corr = 0.8;
        double max_pair_exposure_pct = 0.2;
    };

    /**
     * @brief Construct with configuration and optional sector mapping.
     * @param cfg Correlation config.
     * @param symbol_to_sector Symbol -> sector mapping.
     */
    MaxCorrelationExposureLimit(Config cfg,
                                std::unordered_map<std::string, std::string> symbol_to_sector = {})
        : config_(cfg), symbol_to_sector_(std::move(symbol_to_sector)) {}

    /**
     * @brief Validate portfolio correlation exposure (order-independent).
     */
    Result<void> validate(const engine::Order& order,
                          const engine::Portfolio& portfolio) const override {
        (void)order;
        return validate_portfolio(portfolio);
    }

    /**
     * @brief Validate portfolio pair exposure against correlation limits.
     */
    Result<void> validate_portfolio(const engine::Portfolio& portfolio) const override {
        update_history(portfolio);
        double equity = portfolio.equity();
        if (equity <= 0.0) {
            return Ok();
        }
        auto symbols = portfolio.get_held_symbols();
        for (size_t i = 0; i < symbols.size(); ++i) {
            for (size_t j = i + 1; j < symbols.size(); ++j) {
                double corr = correlation(symbols[i], symbols[j]);
                if (std::abs(corr) < config_.max_corr) {
                    continue;
                }
                double exposure = pair_exposure(portfolio, symbols[i], symbols[j]);
                if (exposure / equity > config_.max_pair_exposure_pct) {
                    return Error(Error::Code::OutOfRange, "Correlation exposure limit exceeded");
                }
            }
        }
        return Ok();
    }

private:
    void update_history(const engine::Portfolio& portfolio) const {
        for (const auto& pos : portfolio.get_all_positions()) {
            if (pos.current_price <= 0.0) {
                continue;
            }
            auto& series = price_history_[pos.symbol];
            series.push_back(pos.current_price);
            if (series.size() > config_.window + 1) {
                series.pop_front();
            }
        }
    }

    double correlation(SymbolId a, SymbolId b) const {
        auto itA = price_history_.find(a);
        auto itB = price_history_.find(b);
        if (itA == price_history_.end() || itB == price_history_.end()) {
            return 0.0;
        }
        const auto& sa = itA->second;
        const auto& sb = itB->second;
        if (sa.size() < 2 || sb.size() < 2 || sa.size() != sb.size()) {
            return 0.0;
        }
        std::vector<double> ra;
        std::vector<double> rb;
        ra.reserve(sa.size() - 1);
        rb.reserve(sb.size() - 1);
        for (size_t i = 1; i < sa.size(); ++i) {
            double r1 = (sa[i] - sa[i - 1]) / sa[i - 1];
            double r2 = (sb[i] - sb[i - 1]) / sb[i - 1];
            ra.push_back(r1);
            rb.push_back(r2);
        }
        double mean_a = 0.0;
        double mean_b = 0.0;
        for (size_t i = 0; i < ra.size(); ++i) {
            mean_a += ra[i];
            mean_b += rb[i];
        }
        mean_a /= ra.size();
        mean_b /= rb.size();
        double num = 0.0;
        double den_a = 0.0;
        double den_b = 0.0;
        for (size_t i = 0; i < ra.size(); ++i) {
            double da = ra[i] - mean_a;
            double db = rb[i] - mean_b;
            num += da * db;
            den_a += da * da;
            den_b += db * db;
        }
        if (den_a <= 0.0 || den_b <= 0.0) {
            return 0.0;
        }
        return num / std::sqrt(den_a * den_b);
    }

    double pair_exposure(const engine::Portfolio& portfolio, SymbolId a, SymbolId b) const {
        double exposure = 0.0;
        for (const auto& pos : portfolio.get_all_positions()) {
            if (pos.symbol == a || pos.symbol == b) {
                exposure += std::abs(pos.quantity * pos.current_price);
            }
        }
        return exposure;
    }

    Config config_;
    std::unordered_map<std::string, std::string> symbol_to_sector_;
    mutable std::unordered_map<SymbolId, std::deque<double>> price_history_;
};

}  // namespace regimeflow::risk
