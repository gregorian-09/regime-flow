#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/engine/order.h"

#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::engine {

struct Position {
    SymbolId symbol = 0;
    Quantity quantity = 0;
    Price avg_cost = 0;
    Price current_price = 0;
    Timestamp last_update;

    double market_value() const { return quantity * current_price; }
    double unrealized_pnl() const { return quantity * (current_price - avg_cost); }
    double unrealized_pnl_pct() const {
        return avg_cost != 0 ? (current_price - avg_cost) / avg_cost : 0;
    }
};

struct PortfolioSnapshot {
    Timestamp timestamp;
    double cash = 0;
    double equity = 0;
    double gross_exposure = 0;
    double net_exposure = 0;
    double leverage = 0;
    std::unordered_map<SymbolId, Position> positions;
};

class Portfolio {
public:
    explicit Portfolio(double initial_capital, std::string currency = "USD");

    void update_position(const Fill& fill);
    void mark_to_market(SymbolId symbol, Price price, Timestamp timestamp);
    void mark_to_market(const std::unordered_map<SymbolId, Price>& prices, Timestamp timestamp);
    void set_cash(double cash, Timestamp timestamp);
    void set_position(SymbolId symbol, Quantity quantity, Price avg_cost, Price current_price,
                      Timestamp timestamp);
    void replace_positions(const std::unordered_map<SymbolId, Position>& positions,
                           Timestamp timestamp);

    std::optional<Position> get_position(SymbolId symbol) const;
    std::vector<Position> get_all_positions() const;
    std::vector<SymbolId> get_held_symbols() const;

    double cash() const { return cash_; }
    double initial_capital() const { return initial_capital_; }
    const std::string& currency() const { return currency_; }
    double equity() const;
    double gross_exposure() const;
    double net_exposure() const;
    double leverage() const;
    double total_unrealized_pnl() const;
    double total_realized_pnl() const { return realized_pnl_; }

    PortfolioSnapshot snapshot() const;
    std::vector<PortfolioSnapshot> equity_curve() const { return snapshots_; }
    void record_snapshot(Timestamp timestamp);

    std::vector<Fill> get_fills() const { return all_fills_; }
    std::vector<Fill> get_fills(SymbolId symbol) const;
    std::vector<Fill> get_fills(TimeRange range) const;

    void on_position_change(std::function<void(const Position&)> callback);
    void on_equity_change(std::function<void(double)> callback);

private:
    void apply_fill(Position& position, const Fill& fill);
    void notify_position(const Position& position);
    void notify_equity(double equity_value);

    double initial_capital_ = 0;
    double cash_ = 0;
    std::string currency_;

    std::unordered_map<SymbolId, Position> positions_;
    std::vector<Fill> all_fills_;
    std::vector<PortfolioSnapshot> snapshots_;

    double realized_pnl_ = 0;

    std::vector<std::function<void(const Position&)>> position_callbacks_;
    std::vector<std::function<void(double)>> equity_callbacks_;
};

}  // namespace regimeflow::engine
