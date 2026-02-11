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

enum class HookResult {
    Continue,
    Skip,
    Cancel
};

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

class HookContext {
public:
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

    void set_bar(const data::Bar* bar) { bar_ = bar; }
    void set_tick(const data::Tick* tick) { tick_ = tick; }
    void set_quote(const data::Quote* quote) { quote_ = quote; }
    void set_book(const data::OrderBook* book) { book_ = book; }
    void set_fill(const engine::Fill* fill) { fill_ = fill; }
    void set_regime_change(const regime::RegimeTransition* change) { regime_change_ = change; }
    void set_order(engine::Order* order) { order_ = order; }
    void set_results(const engine::BacktestResults* results) { results_ = results; }
    void set_timer_id(std::string id) { timer_id_ = std::move(id); }

    void modify_order(const engine::Order& order) {
        if (order_) {
            *order_ = order;
        }
    }

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

    void register_hook(HookType type, Hook hook, int priority = 100);
    void register_backtest_start(BacktestStartHook hook, int priority = 100);
    void register_backtest_end(BacktestEndHook hook, int priority = 100);
    void register_day_start(DayStartHook hook, int priority = 100);
    void register_day_end(DayEndHook hook, int priority = 100);
    void register_on_bar(BarHook hook, int priority = 100);
    void register_on_tick(TickHook hook, int priority = 100);
    void register_on_quote(QuoteHook hook, int priority = 100);
    void register_on_book(BookHook hook, int priority = 100);
    void register_on_timer(TimerHook hook, int priority = 100);
    void register_order_submit(OrderSubmitHook hook, int priority = 100);
    void register_on_fill(FillHook hook, int priority = 100);
    void register_regime_change(RegimeChangeHook hook, int priority = 100);
    HookResult invoke(HookType type, HookContext& ctx) const;
    void clear_all_hooks();
    void disable_hooks();
    void enable_hooks();

private:
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

class HookSystem {
public:
    using EventHook = std::function<void(const events::Event&)>;
    using SimpleHook = std::function<void()>;

    void add_pre_event_hook(EventHook hook);
    void add_post_event_hook(EventHook hook);
    void add_on_start(SimpleHook hook);
    void add_on_stop(SimpleHook hook);

    void run_pre_event(const events::Event& event) const;
    void run_post_event(const events::Event& event) const;
    void run_start() const;
    void run_stop() const;

private:
    std::vector<EventHook> pre_event_;
    std::vector<EventHook> post_event_;
    std::vector<SimpleHook> on_start_;
    std::vector<SimpleHook> on_stop_;
};

}  // namespace regimeflow::plugins
