#include "regimeflow/engine/portfolio.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::engine {

Portfolio::Portfolio(double initial_capital, std::string currency)
    : initial_capital_(initial_capital), cash_(initial_capital), currency_(std::move(currency)) {}

void Portfolio::update_position(const Fill& fill) {
    if (fill.symbol == 0 || fill.quantity == 0) {
        return;
    }
    cash_ -= fill.price * fill.quantity;
    cash_ -= fill.commission;

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

void Portfolio::mark_to_market(SymbolId symbol, Price price, Timestamp timestamp) {
    auto it = positions_.find(symbol);
    if (it == positions_.end()) {
        return;
    }
    it->second.current_price = price;
    it->second.last_update = timestamp;
    notify_position(it->second);
    notify_equity(equity());
}

void Portfolio::mark_to_market(const std::unordered_map<SymbolId, Price>& prices,
                               Timestamp timestamp) {
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

void Portfolio::set_cash(double cash, Timestamp timestamp) {
    cash_ = cash;
    notify_equity(equity());
    if (!snapshots_.empty()) {
        snapshots_.back().timestamp = timestamp;
        snapshots_.back().cash = cash_;
        snapshots_.back().equity = equity();
    }
}

void Portfolio::set_position(SymbolId symbol, Quantity quantity, Price avg_cost,
                             Price current_price, Timestamp timestamp) {
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
                                  Timestamp timestamp) {
    positions_.clear();
    for (const auto& [symbol, position] : positions) {
        Position updated = position;
        updated.last_update = timestamp;
        positions_[symbol] = updated;
    }
    for (const auto& [_, position] : positions_) {
        notify_position(position);
    }
    notify_equity(equity());
}

std::optional<Position> Portfolio::get_position(SymbolId symbol) const {
    auto it = positions_.find(symbol);
    if (it == positions_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<Position> Portfolio::get_all_positions() const {
    std::vector<Position> result;
    result.reserve(positions_.size());
    for (const auto& [symbol, position] : positions_) {
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
    for (const auto& [symbol, position] : positions_) {
        total += position.market_value();
    }
    return total;
}

double Portfolio::gross_exposure() const {
    double total = 0;
    for (const auto& [symbol, position] : positions_) {
        total += std::abs(position.market_value());
    }
    return total;
}

double Portfolio::net_exposure() const {
    double total = 0;
    for (const auto& [symbol, position] : positions_) {
        total += position.market_value();
    }
    return total;
}

double Portfolio::leverage() const {
    double eq = equity();
    if (eq == 0) {
        return 0;
    }
    return gross_exposure() / eq;
}

double Portfolio::total_unrealized_pnl() const {
    double total = 0;
    for (const auto& [symbol, position] : positions_) {
        total += position.unrealized_pnl();
    }
    return total;
}

PortfolioSnapshot Portfolio::snapshot() const {
    PortfolioSnapshot snap;
    snap.timestamp = Timestamp::now();
    snap.cash = cash_;
    snap.equity = equity();
    snap.gross_exposure = gross_exposure();
    snap.net_exposure = net_exposure();
    snap.leverage = snap.equity != 0 ? snap.gross_exposure / snap.equity : 0;
    snap.positions = positions_;
    return snap;
}

PortfolioSnapshot Portfolio::snapshot(Timestamp timestamp) const {
    PortfolioSnapshot snap;
    snap.timestamp = timestamp;
    snap.cash = cash_;
    snap.equity = equity();
    snap.gross_exposure = gross_exposure();
    snap.net_exposure = net_exposure();
    snap.leverage = snap.equity != 0 ? snap.gross_exposure / snap.equity : 0;
    snap.positions = positions_;
    return snap;
}

void Portfolio::record_snapshot(Timestamp timestamp) {
    PortfolioSnapshot snap;
    snap.timestamp = timestamp;
    snap.cash = cash_;
    snap.equity = equity();
    snap.gross_exposure = gross_exposure();
    snap.net_exposure = net_exposure();
    snap.leverage = snap.equity != 0 ? snap.gross_exposure / snap.equity : 0;
    snap.positions = positions_;
    snapshots_.push_back(std::move(snap));
}

std::vector<Fill> Portfolio::get_fills(SymbolId symbol) const {
    std::vector<Fill> fills;
    for (const auto& fill : all_fills_) {
        if (fill.symbol == symbol) {
            fills.push_back(fill);
        }
    }
    return fills;
}

std::vector<Fill> Portfolio::get_fills(TimeRange range) const {
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
        double total_cost = position.avg_cost * old_qty + fill.price * qty_delta;
        position.quantity = new_qty;
        position.avg_cost = new_qty != 0 ? total_cost / new_qty : 0;
        return;
    }

    double closing_qty = std::min(std::abs(qty_delta), std::abs(old_qty));
    double sign = old_qty > 0 ? 1.0 : -1.0;
    realized_pnl_ += closing_qty * (fill.price - position.avg_cost) * sign;

    position.quantity = new_qty;
    if (new_qty == 0) {
        position.avg_cost = 0;
    } else if ((old_qty > 0 && new_qty < 0) || (old_qty < 0 && new_qty > 0)) {
        position.avg_cost = fill.price;
    }
}

void Portfolio::notify_position(const Position& position) {
    for (const auto& cb : position_callbacks_) {
        cb(position);
    }
}

void Portfolio::notify_equity(double equity_value) {
    for (const auto& cb : equity_callbacks_) {
        cb(equity_value);
    }
}

}  // namespace regimeflow::engine
