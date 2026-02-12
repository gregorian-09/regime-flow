/**
 * @file portfolio.h
 * @brief RegimeFlow regimeflow portfolio declarations.
 */

#pragma once

#include "regimeflow/common/types.h"
#include "regimeflow/engine/order.h"

#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::engine {

/**
 * @brief Position state for a single symbol.
 */
struct Position {
    SymbolId symbol = 0;
    Quantity quantity = 0;
    Price avg_cost = 0;
    Price current_price = 0;
    Timestamp last_update;

    /**
     * @brief Current market value of the position.
     * @return Quantity * current_price.
     */
    double market_value() const { return quantity * current_price; }
    /**
     * @brief Unrealized PnL in currency units.
     * @return (current_price - avg_cost) * quantity.
     */
    double unrealized_pnl() const { return quantity * (current_price - avg_cost); }
    /**
     * @brief Unrealized PnL as a fraction of avg cost.
     * @return (current_price - avg_cost) / avg_cost or 0 if avg_cost is zero.
     */
    double unrealized_pnl_pct() const {
        return avg_cost != 0 ? (current_price - avg_cost) / avg_cost : 0;
    }
};

/**
 * @brief Portfolio state snapshot at a point in time.
 */
struct PortfolioSnapshot {
    Timestamp timestamp;
    double cash = 0;
    double equity = 0;
    double gross_exposure = 0;
    double net_exposure = 0;
    double leverage = 0;
    std::unordered_map<SymbolId, Position> positions;
};

/**
 * @brief Tracks positions, cash, and portfolio metrics.
 */
class Portfolio {
public:
    /**
     * @brief Construct a portfolio.
     * @param initial_capital Starting cash.
     * @param currency Base currency code.
     */
    explicit Portfolio(double initial_capital, std::string currency = "USD");

    /**
     * @brief Update the portfolio from a fill.
     * @param fill Fill to apply.
     */
    void update_position(const Fill& fill);
    /**
     * @brief Mark a single symbol to market.
     * @param symbol Symbol ID.
     * @param price Latest price.
     * @param timestamp Timestamp for valuation.
     */
    void mark_to_market(SymbolId symbol, Price price, Timestamp timestamp);
    /**
     * @brief Mark multiple symbols to market.
     * @param prices Map of symbol to price.
     * @param timestamp Timestamp for valuation.
     */
    void mark_to_market(const std::unordered_map<SymbolId, Price>& prices, Timestamp timestamp);
    /**
     * @brief Set cash balance.
     * @param cash New cash balance.
     * @param timestamp Timestamp for update.
     */
    void set_cash(double cash, Timestamp timestamp);
    /**
     * @brief Set a position explicitly.
     * @param symbol Symbol ID.
     * @param quantity Position quantity.
     * @param avg_cost Average cost basis.
     * @param current_price Latest price.
     * @param timestamp Timestamp for update.
     */
    void set_position(SymbolId symbol, Quantity quantity, Price avg_cost, Price current_price,
                      Timestamp timestamp);
    /**
     * @brief Replace all positions.
     * @param positions New positions map.
     * @param timestamp Timestamp for update.
     */
    void replace_positions(const std::unordered_map<SymbolId, Position>& positions,
                           Timestamp timestamp);

    /**
     * @brief Get a position for a symbol.
     * @param symbol Symbol ID.
     * @return Optional position.
     */
    std::optional<Position> get_position(SymbolId symbol) const;
    /**
     * @brief Get all positions.
     * @return Vector of positions.
     */
    std::vector<Position> get_all_positions() const;
    /**
     * @brief Get symbols currently held.
     * @return Vector of symbol IDs.
     */
    std::vector<SymbolId> get_held_symbols() const;

    /**
     * @brief Current cash balance.
     */
    double cash() const { return cash_; }
    /**
     * @brief Initial capital.
     */
    double initial_capital() const { return initial_capital_; }
    /**
     * @brief Base currency.
     */
    const std::string& currency() const { return currency_; }
    /**
     * @brief Total equity (cash + market value).
     */
    double equity() const;
    /**
     * @brief Gross exposure (sum of absolute positions).
     */
    double gross_exposure() const;
    /**
     * @brief Net exposure (sum of signed positions).
     */
    double net_exposure() const;
    /**
     * @brief Portfolio leverage.
     */
    double leverage() const;
    /**
     * @brief Total unrealized PnL across positions.
     */
    double total_unrealized_pnl() const;
    /**
     * @brief Total realized PnL across closed trades.
     */
    double total_realized_pnl() const { return realized_pnl_; }

    /**
     * @brief Snapshot the current portfolio state.
     * @return Snapshot.
     */
    PortfolioSnapshot snapshot() const;
    /**
     * @brief Snapshot the current portfolio state at a specific timestamp.
     * @param timestamp Snapshot time.
     * @return Snapshot.
     */
    PortfolioSnapshot snapshot(Timestamp timestamp) const;
    /**
     * @brief Equity curve history.
     * @return Vector of snapshots.
     */
    std::vector<PortfolioSnapshot> equity_curve() const { return snapshots_; }
    /**
     * @brief Record a snapshot at a timestamp.
     * @param timestamp Snapshot time.
     */
    void record_snapshot(Timestamp timestamp);

    /**
     * @brief All fills applied to the portfolio.
     * @return Vector of fills.
     */
    std::vector<Fill> get_fills() const { return all_fills_; }
    /**
     * @brief Fills for a symbol.
     * @param symbol Symbol ID.
     * @return Vector of fills.
     */
    std::vector<Fill> get_fills(SymbolId symbol) const;
    /**
     * @brief Fills within a time range.
     * @param range Time range.
     * @return Vector of fills.
     */
    std::vector<Fill> get_fills(TimeRange range) const;

    /**
     * @brief Register a callback for position changes.
     * @param callback Callback invoked with the updated position.
     */
    void on_position_change(std::function<void(const Position&)> callback);
    /**
     * @brief Register a callback for equity changes.
     * @param callback Callback invoked with new equity.
     */
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
