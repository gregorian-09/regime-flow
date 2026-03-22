#include "regimeflow/engine/portfolio.h"

#include <algorithm>
#include <cmath>
#include <ranges>

namespace regimeflow::engine
{
    MarginProfile MarginProfile::from_config(const Config& config,
                                             const std::string& prefix) {
        return from_config(config, prefix, MarginProfile{});
    }

    MarginProfile MarginProfile::from_config(const Config& config,
                                             const std::string& prefix,
                                             const MarginProfile& defaults) {
        MarginProfile merged = defaults;
        const auto key = [&prefix](const std::string& suffix) {
            if (prefix.empty()) {
                return suffix;
            }
            return prefix + "." + suffix;
        };
        if (const auto configured = config.get_as<double>(key("initial_margin_ratio"))) {
            merged.initial_margin_ratio = *configured;
        }
        if (const auto configured = config.get_as<double>(key("maintenance_margin_ratio"))) {
            merged.maintenance_margin_ratio = *configured;
        }
        if (const auto configured = config.get_as<double>(key("stop_out_margin_level"))) {
            merged.stop_out_margin_level = *configured;
        }
        return merged;
    }

    Portfolio::Portfolio(const double initial_capital, std::string currency)
        : initial_capital_(initial_capital), cash_(initial_capital), currency_(std::move(currency)) {}

    void Portfolio::update_position(const Fill& fill) {
        if (fill.symbol == 0 || fill.quantity == 0) {
            return;
        }
        cash_ -= fill.price * fill.quantity;
        cash_ -= fill.commission;
        cash_ -= fill.transaction_cost;

        auto& position = positions_[fill.symbol];
        if (position.symbol == 0) {
            position.symbol = fill.symbol;
        }

        apply_fill(position, fill);
        position.current_price = fill.price;
        position.last_update = fill.timestamp;

        all_fills_.push_back(fill);

        notify_position(position);
        notify_equity(equity());
    }

    void Portfolio::mark_to_market(const SymbolId symbol, const Price price, const Timestamp timestamp) {
        const auto it = positions_.find(symbol);
        if (it == positions_.end()) {
            return;
        }
        it->second.current_price = price;
        it->second.last_update = timestamp;
        notify_position(it->second);
        notify_equity(equity());
    }

    void Portfolio::mark_to_market(const std::unordered_map<SymbolId, Price>& prices,
                                   const Timestamp timestamp) {
        for (const auto& [symbol, price] : prices) {
            auto it = positions_.find(symbol);
            if (it == positions_.end()) {
                continue;
            }
            it->second.current_price = price;
            it->second.last_update = timestamp;
            notify_position(it->second);
        }
        notify_equity(equity());
    }

    void Portfolio::set_cash(const double cash, const Timestamp timestamp) {
        cash_ = cash;
        notify_equity(equity());
        if (!snapshots_.empty()) {
            auto& snapshot = snapshots_.back();
            snapshot.timestamp = timestamp;
            apply_snapshot_fields(snapshot);
        }
    }

    void Portfolio::adjust_cash(const double delta, const Timestamp timestamp) {
        set_cash(cash_ + delta, timestamp);
    }

    void Portfolio::set_position(const SymbolId symbol, const Quantity quantity, const Price avg_cost,
                                 const Price current_price, const Timestamp timestamp) {
        if (symbol == 0) {
            return;
        }
        Position pos;
        pos.symbol = symbol;
        pos.quantity = quantity;
        pos.avg_cost = avg_cost;
        pos.current_price = current_price;
        pos.last_update = timestamp;
        positions_[symbol] = pos;
        notify_position(pos);
        notify_equity(equity());
    }

    void Portfolio::replace_positions(const std::unordered_map<SymbolId, Position>& positions,
                                      const Timestamp timestamp) {
        positions_.clear();
        for (const auto& [symbol, position] : positions) {
            Position updated = position;
            updated.last_update = timestamp;
            positions_[symbol] = updated;
        }
        for (const auto& position : positions_ | std::views::values) {
            notify_position(position);
        }
        notify_equity(equity());
    }

    std::optional<Position> Portfolio::get_position(const SymbolId symbol) const {
        const auto it = positions_.find(symbol);
        if (it == positions_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<Position> Portfolio::get_all_positions() const {
        std::vector<Position> result;
        result.reserve(positions_.size());
        for (const auto& position : positions_ | std::views::values) {
            result.push_back(position);
        }
        return result;
    }

    std::vector<SymbolId> Portfolio::get_held_symbols() const {
        std::vector<SymbolId> symbols;
        for (const auto& [symbol, position] : positions_) {
            if (position.quantity != 0) {
                symbols.push_back(symbol);
            }
        }
        return symbols;
    }

    double Portfolio::equity() const {
        double total = cash_;
        for (const auto& position : positions_ | std::views::values) {
            total += position.market_value();
        }
        return total;
    }

    double Portfolio::gross_exposure() const {
        double total = 0;
        for (const auto& position : positions_ | std::views::values) {
            total += std::abs(position.market_value());
        }
        return total;
    }

    double Portfolio::net_exposure() const {
        double total = 0;
        for (const auto& position : positions_ | std::views::values) {
            total += position.market_value();
        }
        return total;
    }

    double Portfolio::leverage() const {
        const double eq = equity();
        if (eq == 0) {
            return 0;
        }
        return gross_exposure() / eq;
    }

    double Portfolio::total_unrealized_pnl() const {
        double total = 0;
        for (const auto& position : positions_ | std::views::values) {
            total += position.unrealized_pnl();
        }
        return total;
    }

    void Portfolio::configure_margin(MarginProfile profile) {
        if (profile.initial_margin_ratio < 0.0) {
            profile.initial_margin_ratio = 0.0;
        }
        if (profile.maintenance_margin_ratio < 0.0) {
            profile.maintenance_margin_ratio = 0.0;
        }
        if (profile.stop_out_margin_level < 0.0) {
            profile.stop_out_margin_level = 0.0;
        }
        margin_profile_ = profile;
    }

    MarginSnapshot Portfolio::margin_snapshot() const {
        return build_margin_snapshot(equity(), gross_exposure());
    }

    PortfolioSnapshot Portfolio::snapshot() const {
        PortfolioSnapshot snap;
        snap.timestamp = Timestamp::now();
        apply_snapshot_fields(snap);
        return snap;
    }

    PortfolioSnapshot Portfolio::snapshot(const Timestamp timestamp) const {
        PortfolioSnapshot snap;
        snap.timestamp = timestamp;
        apply_snapshot_fields(snap);
        return snap;
    }

    void Portfolio::record_snapshot(const Timestamp timestamp) {
        PortfolioSnapshot snap;
        snap.timestamp = timestamp;
        apply_snapshot_fields(snap);
        snapshots_.push_back(std::move(snap));
    }

    std::vector<Fill> Portfolio::get_fills(const SymbolId symbol) const {
        std::vector<Fill> fills;
        for (const auto& fill : all_fills_) {
            if (fill.symbol == symbol) {
                fills.push_back(fill);
            }
        }
        return fills;
    }

    std::vector<Fill> Portfolio::get_fills(const TimeRange range) const {
        std::vector<Fill> fills;
        for (const auto& fill : all_fills_) {
            if (range.contains(fill.timestamp)) {
                fills.push_back(fill);
            }
        }
        return fills;
    }

    void Portfolio::on_position_change(std::function<void(const Position&)> callback) {
        position_callbacks_.push_back(std::move(callback));
    }

    void Portfolio::on_equity_change(std::function<void(double)> callback) {
        equity_callbacks_.push_back(std::move(callback));
    }

    void Portfolio::apply_fill(Position& position, const Fill& fill) {
        const double qty_delta = fill.quantity;
        const double old_qty = position.quantity;
        const double new_qty = old_qty + qty_delta;

        if (old_qty == 0 || (old_qty > 0 && qty_delta > 0) || (old_qty < 0 && qty_delta < 0)) {
            const double total_cost = position.avg_cost * old_qty + fill.price * qty_delta;
            position.quantity = new_qty;
            position.avg_cost = new_qty != 0 ? total_cost / new_qty : 0;
            return;
        }

        const double closing_qty = std::min(std::abs(qty_delta), std::abs(old_qty));
        const double sign = old_qty > 0 ? 1.0 : -1.0;
        realized_pnl_ += closing_qty * (fill.price - position.avg_cost) * sign;

        position.quantity = new_qty;
        if (new_qty == 0) {
            position.avg_cost = 0;
        } else if ((old_qty > 0 && new_qty < 0) || (old_qty < 0 && new_qty > 0)) {
            position.avg_cost = fill.price;
        }
    }

    MarginSnapshot Portfolio::build_margin_snapshot(const double equity_value,
                                                    const double gross_exposure_value) const {
        MarginSnapshot snapshot;
        snapshot.initial_margin = gross_exposure_value * margin_profile_.initial_margin_ratio;
        snapshot.maintenance_margin = gross_exposure_value * margin_profile_.maintenance_margin_ratio;
        snapshot.available_funds = equity_value - snapshot.initial_margin;
        snapshot.margin_excess = equity_value - snapshot.maintenance_margin;
        if (margin_profile_.initial_margin_ratio > 0.0) {
            snapshot.buying_power =
                std::max(snapshot.available_funds / margin_profile_.initial_margin_ratio, 0.0);
        } else {
            snapshot.buying_power = std::max(snapshot.available_funds, 0.0);
        }
        snapshot.margin_call = snapshot.margin_excess < 0.0;
        if (snapshot.maintenance_margin > 0.0) {
            const double margin_level = equity_value / snapshot.maintenance_margin;
            snapshot.stop_out = margin_level <= margin_profile_.stop_out_margin_level;
        }
        return snapshot;
    }

    void Portfolio::apply_snapshot_fields(PortfolioSnapshot& snapshot) const {
        snapshot.cash = cash_;
        snapshot.equity = equity();
        snapshot.gross_exposure = gross_exposure();
        snapshot.net_exposure = net_exposure();
        snapshot.leverage = snapshot.equity != 0 ? snapshot.gross_exposure / snapshot.equity : 0.0;
        const MarginSnapshot margin = build_margin_snapshot(snapshot.equity, snapshot.gross_exposure);
        snapshot.initial_margin = margin.initial_margin;
        snapshot.maintenance_margin = margin.maintenance_margin;
        snapshot.available_funds = margin.available_funds;
        snapshot.margin_excess = margin.margin_excess;
        snapshot.buying_power = margin.buying_power;
        snapshot.margin_call = margin.margin_call;
        snapshot.stop_out = margin.stop_out;
        snapshot.positions = positions_;
    }

    void Portfolio::notify_position(const Position& position) const
    {
        for (const auto& cb : position_callbacks_) {
            cb(position);
        }
    }

    void Portfolio::notify_equity(const double equity_value) const
    {
        for (const auto& cb : equity_callbacks_) {
            cb(equity_value);
        }
    }
}  // namespace regimeflow::engine
