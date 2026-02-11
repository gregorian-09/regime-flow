#include "regimeflow/plugins/hooks.h"

#include <algorithm>

namespace regimeflow::plugins {

void HookManager::register_hook(HookType type, Hook hook, int priority) {
    auto& list = hooks_for(type);
    list.push_back(Entry{std::move(hook), priority, next_sequence_++});
    sort_hooks(list);
}

void HookManager::register_backtest_start(BacktestStartHook hook, int priority) {
    register_hook(HookType::BacktestStart, std::move(hook), priority);
}

void HookManager::register_backtest_end(BacktestEndHook hook, int priority) {
    register_hook(HookType::BacktestEnd,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* results = ctx.results();
                      if (!results) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *results);
                  },
                  priority);
}

void HookManager::register_day_start(DayStartHook hook, int priority) {
    register_hook(HookType::DayStart,
                  [hook = std::move(hook)](HookContext& ctx) {
                      return hook(ctx, ctx.current_time());
                  },
                  priority);
}

void HookManager::register_day_end(DayEndHook hook, int priority) {
    register_hook(HookType::DayEnd,
                  [hook = std::move(hook)](HookContext& ctx) {
                      return hook(ctx, ctx.current_time());
                  },
                  priority);
}

void HookManager::register_on_bar(BarHook hook, int priority) {
    register_hook(HookType::Bar,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* bar = ctx.bar();
                      if (!bar) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *bar);
                  },
                  priority);
}

void HookManager::register_on_tick(TickHook hook, int priority) {
    register_hook(HookType::Tick,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* tick = ctx.tick();
                      if (!tick) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *tick);
                  },
                  priority);
}

void HookManager::register_on_quote(QuoteHook hook, int priority) {
    register_hook(HookType::Quote,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* quote = ctx.quote();
                      if (!quote) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *quote);
                  },
                  priority);
}

void HookManager::register_on_book(BookHook hook, int priority) {
    register_hook(HookType::Book,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* book = ctx.book();
                      if (!book) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *book);
                  },
                  priority);
}

void HookManager::register_on_timer(TimerHook hook, int priority) {
    register_hook(HookType::Timer,
                  [hook = std::move(hook)](HookContext& ctx) {
                      return hook(ctx, ctx.timer_id());
                  },
                  priority);
}

void HookManager::register_order_submit(OrderSubmitHook hook, int priority) {
    register_hook(HookType::OrderSubmit,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* order = ctx.order();
                      if (!order) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *order);
                  },
                  priority);
}

void HookManager::register_on_fill(FillHook hook, int priority) {
    register_hook(HookType::Fill,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* fill = ctx.fill();
                      if (!fill) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *fill);
                  },
                  priority);
}

void HookManager::register_regime_change(RegimeChangeHook hook, int priority) {
    register_hook(HookType::RegimeChange,
                  [hook = std::move(hook)](HookContext& ctx) {
                      auto* change = ctx.regime_change();
                      if (!change) {
                          return HookResult::Continue;
                      }
                      return hook(ctx, *change);
                  },
                  priority);
}

HookResult HookManager::invoke(HookType type, HookContext& ctx) const {
    if (!hooks_enabled_) {
        return HookResult::Continue;
    }
    const auto& list = hooks_for(type);
    for (const auto& entry : list) {
        auto result = entry.hook(ctx);
        if (result == HookResult::Skip) {
            break;
        }
        if (result == HookResult::Cancel) {
            return HookResult::Cancel;
        }
    }
    return HookResult::Continue;
}

void HookManager::clear_all_hooks() {
    hooks_.clear();
}

void HookManager::disable_hooks() {
    hooks_enabled_ = false;
}

void HookManager::enable_hooks() {
    hooks_enabled_ = true;
}

std::vector<HookManager::Entry>& HookManager::hooks_for(HookType type) {
    return hooks_[type];
}

const std::vector<HookManager::Entry>& HookManager::hooks_for(HookType type) const {
    static const std::vector<Entry> empty;
    auto it = hooks_.find(type);
    if (it == hooks_.end()) {
        return empty;
    }
    return it->second;
}

void HookManager::sort_hooks(std::vector<Entry>& hooks) const {
    std::stable_sort(hooks.begin(), hooks.end(),
                     [](const Entry& a, const Entry& b) {
                         if (a.priority != b.priority) {
                             return a.priority < b.priority;
                         }
                         return a.sequence < b.sequence;
                     });
}

void HookSystem::add_pre_event_hook(EventHook hook) {
    pre_event_.push_back(std::move(hook));
}

void HookSystem::add_post_event_hook(EventHook hook) {
    post_event_.push_back(std::move(hook));
}

void HookSystem::add_on_start(SimpleHook hook) {
    on_start_.push_back(std::move(hook));
}

void HookSystem::add_on_stop(SimpleHook hook) {
    on_stop_.push_back(std::move(hook));
}

void HookSystem::run_pre_event(const events::Event& event) const {
    for (const auto& hook : pre_event_) {
        hook(event);
    }
}

void HookSystem::run_post_event(const events::Event& event) const {
    for (const auto& hook : post_event_) {
        hook(event);
    }
}

void HookSystem::run_start() const {
    for (const auto& hook : on_start_) {
        hook();
    }
}

void HookSystem::run_stop() const {
    for (const auto& hook : on_stop_) {
        hook();
    }
}

}  // namespace regimeflow::plugins
