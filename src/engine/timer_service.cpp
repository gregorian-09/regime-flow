#include "regimeflow/engine/timer_service.h"

#include <ranges>

namespace regimeflow::engine
{
    TimerService::TimerService(events::EventQueue* queue) : queue_(queue) {}

    void TimerService::schedule(const std::string& id, const Duration interval, const Timestamp start) {
        TimerEntry entry;
        entry.id = id;
        entry.interval = interval;
        entry.next_fire = start + interval;
        timers_[id] = entry;
    }

    void TimerService::cancel(const std::string& id) {
        timers_.erase(id);
    }

    void TimerService::on_time_advance(const Timestamp now) {
        if (!queue_) {
            return;
        }
        for (auto& [id, interval, next_fire] : timers_ | std::views::values) {
            if (now >= next_fire) {
                events::Event evt = events::make_system_event(
                    events::SystemEventKind::Timer, next_fire, 0, id);
                queue_->push(std::move(evt));
                next_fire = next_fire + interval;
            }
        }
    }
}  // namespace regimeflow::engine
