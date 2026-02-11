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

class StrategyContext {
public:
    StrategyContext(engine::OrderManager* order_manager,
                    engine::Portfolio* portfolio,
                    engine::EventLoop* event_loop,
                    engine::MarketDataCache* market_data,
                    engine::OrderBookCache* order_books,
                    engine::TimerService* timer_service,
                    engine::RegimeTracker* regime_tracker,
                    Config config = {});

    Result<engine::OrderId> submit_order(engine::Order order);
    Result<void> cancel_order(engine::OrderId id);

    const Config& config() const { return config_; }

    template<typename T>
    std::optional<T> get_as(const std::string& key) const {
        return config_.get_as<T>(key);
    }

    engine::Portfolio& portfolio();
    const engine::Portfolio& portfolio() const;

    std::optional<data::Bar> latest_bar(SymbolId symbol) const;
    std::optional<data::Tick> latest_tick(SymbolId symbol) const;
    std::optional<data::Quote> latest_quote(SymbolId symbol) const;
    std::vector<data::Bar> recent_bars(SymbolId symbol, size_t count) const;
    std::optional<data::OrderBook> latest_order_book(SymbolId symbol) const;
    const regime::RegimeState& current_regime() const;

    void schedule_timer(const std::string& id, Duration interval);
    void cancel_timer(const std::string& id);

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
