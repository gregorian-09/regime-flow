/**
 * @file context.h
 * @brief RegimeFlow regimeflow context declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/common/time.h"
#include "regimeflow/common/config.h"
#include "regimeflow/engine/market_data_cache.h"
#include "regimeflow/engine/order_book_cache.h"
#include "regimeflow/engine/timer_service.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/regime/types.h"

#include <optional>
#include <vector>

namespace regimeflow::engine {
class EventLoop;
class RegimeTracker;
}  // namespace regimeflow::engine

namespace regimeflow::strategy {

namespace engine = ::regimeflow::engine;

/**
 * @brief Context passed to strategies for data and execution.
 */
class StrategyContext {
public:
    /**
     * @brief Construct a strategy context.
     * @param order_manager Order manager for submissions.
     * @param portfolio Portfolio access.
     * @param event_loop Event loop for time reference.
     * @param market_data Market data cache.
     * @param order_books Order book cache.
     * @param timer_service Timer service for callbacks.
     * @param regime_tracker Regime tracker.
     * @param config Strategy configuration.
     */
    StrategyContext(engine::OrderManager* order_manager,
                    engine::Portfolio* portfolio,
                    engine::EventLoop* event_loop,
                    engine::MarketDataCache* market_data,
                    engine::OrderBookCache* order_books,
                    engine::TimerService* timer_service,
                    engine::RegimeTracker* regime_tracker,
                    Config config = {});

    /**
     * @brief Submit an order.
     * @param order Order to submit.
     * @return Order ID on success.
     */
    Result<engine::OrderId> submit_order(engine::Order order);
    /**
     * @brief Cancel an order by ID.
     * @param id Order ID.
     */
    Result<void> cancel_order(engine::OrderId id);

    /**
     * @brief Strategy configuration.
     */
    const Config& config() const { return config_; }

    template<typename T>
    /**
     * @brief Get a typed config value.
     * @tparam T Requested type.
     * @param key Key or dotted path.
     * @return Optional value.
     */
    std::optional<T> get_as(const std::string& key) const {
        return config_.get_as<T>(key);
    }

    /**
     * @brief Access the portfolio.
     */
    engine::Portfolio& portfolio();
    /**
     * @brief Access the portfolio (const).
     */
    const engine::Portfolio& portfolio() const;

    /**
     * @brief Latest bar for a symbol.
     */
    std::optional<data::Bar> latest_bar(SymbolId symbol) const;
    /**
     * @brief Latest tick for a symbol.
     */
    std::optional<data::Tick> latest_tick(SymbolId symbol) const;
    /**
     * @brief Latest quote for a symbol.
     */
    std::optional<data::Quote> latest_quote(SymbolId symbol) const;
    /**
     * @brief Recent bars for a symbol.
     */
    std::vector<data::Bar> recent_bars(SymbolId symbol, size_t count) const;
    /**
     * @brief Latest order book for a symbol.
     */
    std::optional<data::OrderBook> latest_order_book(SymbolId symbol) const;
    /**
     * @brief Current regime state.
     */
    const regime::RegimeState& current_regime() const;

    /**
     * @brief Schedule a recurring timer.
     */
    void schedule_timer(const std::string& id, Duration interval);
    /**
     * @brief Cancel a timer.
     */
    void cancel_timer(const std::string& id);

    /**
     * @brief Current simulated time.
     */
    Timestamp current_time() const;

private:
    engine::OrderManager* order_manager_ = nullptr;
    engine::Portfolio* portfolio_ = nullptr;
    engine::EventLoop* event_loop_ = nullptr;
    engine::MarketDataCache* market_data_ = nullptr;
    engine::OrderBookCache* order_books_ = nullptr;
    engine::TimerService* timer_service_ = nullptr;
    engine::RegimeTracker* regime_tracker_ = nullptr;
    Config config_;
};

}  // namespace regimeflow::strategy
