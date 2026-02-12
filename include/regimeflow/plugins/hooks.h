/**
 * @file hooks.h
 * @brief RegimeFlow regimeflow hooks declarations.
 */

#pragma once

#include "regimeflow/events/event.h"
#include "regimeflow/events/event_queue.h"
#include "regimeflow/engine/order.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/engine/market_data_cache.h"
#include "regimeflow/regime/types.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace regimeflow::engine {
struct BacktestResults;
}  // namespace regimeflow::engine

namespace regimeflow::plugins {

/**
 * @brief Hook return directive.
 */
enum class HookResult {
    Continue,
    Skip,
    Cancel
};

/**
 * @brief Supported hook types in the engine lifecycle.
 */
enum class HookType {
    BacktestStart,
    BacktestEnd,
    DayStart,
    DayEnd,
    Bar,
    Tick,
    Quote,
    Book,
    Timer,
    OrderSubmit,
    Fill,
    RegimeChange
};

/**
 * @brief Context object passed to hook callbacks.
 */
class HookContext {
public:
    /**
     * @brief Construct a hook context with system references.
     * @param portfolio Portfolio snapshot.
     * @param market Market data cache.
     * @param regime Current regime state.
     * @param queue Event queue for injections.
     * @param current_time Current simulated time.
     */
    HookContext(const engine::Portfolio* portfolio,
                const engine::MarketDataCache* market,
                const regime::RegimeState* regime,
                events::EventQueue* queue,
                Timestamp current_time)
        : portfolio_(portfolio),
          market_(market),
          regime_(regime),
          queue_(queue),
          current_time_(current_time) {}

    const engine::Portfolio& portfolio() const { return *portfolio_; }
    const engine::MarketDataCache& market() const { return *market_; }
    const regime::RegimeState& regime() const { return *regime_; }
    Timestamp current_time() const { return current_time_; }

    const data::Bar* bar() const { return bar_; }
    const data::Tick* tick() const { return tick_; }
    const data::Quote* quote() const { return quote_; }
    const data::OrderBook* book() const { return book_; }
    const engine::Fill* fill() const { return fill_; }
    const regime::RegimeTransition* regime_change() const { return regime_change_; }
    engine::Order* order() const { return order_; }
    const engine::BacktestResults* results() const { return results_; }
    const std::string& timer_id() const { return timer_id_; }

    /**
     * @brief Attach bar payload for bar hooks.
     */
    void set_bar(const data::Bar* bar) { bar_ = bar; }
    /**
     * @brief Attach tick payload for tick hooks.
     */
    void set_tick(const data::Tick* tick) { tick_ = tick; }
    /**
     * @brief Attach quote payload for quote hooks.
     */
    void set_quote(const data::Quote* quote) { quote_ = quote; }
    /**
     * @brief Attach order book payload for book hooks.
     */
    void set_book(const data::OrderBook* book) { book_ = book; }
    /**
     * @brief Attach fill payload for fill hooks.
     */
    void set_fill(const engine::Fill* fill) { fill_ = fill; }
    /**
     * @brief Attach regime transition payload.
     */
    void set_regime_change(const regime::RegimeTransition* change) { regime_change_ = change; }
    /**
     * @brief Attach mutable order for order-submit hooks.
     */
    void set_order(engine::Order* order) { order_ = order; }
    /**
     * @brief Attach results for backtest end hooks.
     */
    void set_results(const engine::BacktestResults* results) { results_ = results; }
    /**
     * @brief Attach timer identifier for timer hooks.
     */
    void set_timer_id(std::string id) { timer_id_ = std::move(id); }

    /**
     * @brief Replace the current order in context.
     * @param order Updated order.
     */
    void modify_order(const engine::Order& order) {
        if (order_) {
            *order_ = order;
        }
    }

    /**
     * @brief Inject a new event into the event queue.
     * @param event Event to enqueue.
     */
    void inject_event(events::Event event) {
        if (queue_) {
            queue_->push(std::move(event));
        }
    }

private:
    const engine::Portfolio* portfolio_ = nullptr;
    const engine::MarketDataCache* market_ = nullptr;
    const regime::RegimeState* regime_ = nullptr;
    events::EventQueue* queue_ = nullptr;
    Timestamp current_time_;
    const data::Bar* bar_ = nullptr;
    const data::Tick* tick_ = nullptr;
    const data::Quote* quote_ = nullptr;
    const data::OrderBook* book_ = nullptr;
    const engine::Fill* fill_ = nullptr;
    const regime::RegimeTransition* regime_change_ = nullptr;
    engine::Order* order_ = nullptr;
    const engine::BacktestResults* results_ = nullptr;
    std::string timer_id_;
};

/**
 * @brief Manages hook registration and invocation.
 */
class HookManager {
public:
    using Hook = std::function<HookResult(HookContext&)>;
    using BacktestStartHook = std::function<HookResult(HookContext&)>;
    using BacktestEndHook = std::function<HookResult(HookContext&, const engine::BacktestResults&)>;
    using DayStartHook = std::function<HookResult(HookContext&, Timestamp)>;
    using DayEndHook = std::function<HookResult(HookContext&, Timestamp)>;
    using BarHook = std::function<HookResult(HookContext&, const data::Bar&)>;
    using TickHook = std::function<HookResult(HookContext&, const data::Tick&)>;
    using QuoteHook = std::function<HookResult(HookContext&, const data::Quote&)>;
    using BookHook = std::function<HookResult(HookContext&, const data::OrderBook&)>;
    using TimerHook = std::function<HookResult(HookContext&, const std::string&)>;
    using OrderSubmitHook = std::function<HookResult(HookContext&, engine::Order&)>;
    using FillHook = std::function<HookResult(HookContext&, const engine::Fill&)>;
    using RegimeChangeHook = std::function<HookResult(HookContext&, const regime::RegimeTransition&)>;

    /**
     * @brief Register a hook for a type.
     * @param type Hook type.
     * @param hook Hook function.
     * @param priority Lower values run first.
     */
    void register_hook(HookType type, Hook hook, int priority = 100);
    /**
     * @brief Register a backtest start hook.
     */
    void register_backtest_start(BacktestStartHook hook, int priority = 100);
    /**
     * @brief Register a backtest end hook.
     */
    void register_backtest_end(BacktestEndHook hook, int priority = 100);
    /**
     * @brief Register a day start hook.
     */
    void register_day_start(DayStartHook hook, int priority = 100);
    /**
     * @brief Register a day end hook.
     */
    void register_day_end(DayEndHook hook, int priority = 100);
    /**
     * @brief Register a bar hook.
     */
    void register_on_bar(BarHook hook, int priority = 100);
    /**
     * @brief Register a tick hook.
     */
    void register_on_tick(TickHook hook, int priority = 100);
    /**
     * @brief Register a quote hook.
     */
    void register_on_quote(QuoteHook hook, int priority = 100);
    /**
     * @brief Register an order book hook.
     */
    void register_on_book(BookHook hook, int priority = 100);
    /**
     * @brief Register a timer hook.
     */
    void register_on_timer(TimerHook hook, int priority = 100);
    /**
     * @brief Register an order submit hook.
     */
    void register_order_submit(OrderSubmitHook hook, int priority = 100);
    /**
     * @brief Register a fill hook.
     */
    void register_on_fill(FillHook hook, int priority = 100);
    /**
     * @brief Register a regime change hook.
     */
    void register_regime_change(RegimeChangeHook hook, int priority = 100);
    /**
     * @brief Invoke hooks for a type.
     */
    HookResult invoke(HookType type, HookContext& ctx) const;
    /**
     * @brief Remove all hooks.
     */
    void clear_all_hooks();
    /**
     * @brief Disable hook execution.
     */
    void disable_hooks();
    /**
     * @brief Enable hook execution.
     */
    void enable_hooks();

private:
    /**
     * @brief Hook entry with priority and sequencing.
     */
    struct Entry {
        Hook hook;
        int priority = 100;
        size_t sequence = 0;
    };

    std::vector<Entry>& hooks_for(HookType type);
    const std::vector<Entry>& hooks_for(HookType type) const;
    void sort_hooks(std::vector<Entry>& hooks) const;

    std::map<HookType, std::vector<Entry>> hooks_;
    size_t next_sequence_ = 0;
    bool hooks_enabled_ = true;
};

/**
 * @brief Lightweight hook system for event lifecycle.
 */
class HookSystem {
public:
    using EventHook = std::function<void(const events::Event&)>;
    using SimpleHook = std::function<void()>;

    /**
     * @brief Add a pre-event hook.
     */
    void add_pre_event_hook(EventHook hook);
    /**
     * @brief Add a post-event hook.
     */
    void add_post_event_hook(EventHook hook);
    /**
     * @brief Add a start hook.
     */
    void add_on_start(SimpleHook hook);
    /**
     * @brief Add a stop hook.
     */
    void add_on_stop(SimpleHook hook);

    /**
     * @brief Run pre-event hooks.
     */
    void run_pre_event(const events::Event& event) const;
    /**
     * @brief Run post-event hooks.
     */
    void run_post_event(const events::Event& event) const;
    /**
     * @brief Run start hooks.
     */
    void run_start() const;
    /**
     * @brief Run stop hooks.
     */
    void run_stop() const;

private:
    std::vector<EventHook> pre_event_;
    std::vector<EventHook> post_event_;
    std::vector<SimpleHook> on_start_;
    std::vector<SimpleHook> on_stop_;
};

}  // namespace regimeflow::plugins
