#include "regimeflow/engine/timer_service.h"

namespace regimeflow::engine {

TimerService::TimerService(events::EventQueue* queue) : queue_(queue) {}

void TimerService::schedule(const std::string& id, Duration interval, Timestamp start) {
    TimerEntry entry;
    entry.id = id;
    entry.interval = interval;
    entry.next_fire = start + interval;
    timers_[id] = entry;
}

void TimerService::cancel(const std::string& id) {
    timers_.erase(id);
}

void TimerService::on_time_advance(Timestamp now) {
    if (!queue_) {
        return;
    }
    for (auto& [id, entry] : timers_) {
        if (now >= entry.next_fire) {
            events::Event evt = events::make_system_event(
                events::SystemEventKind::Timer, entry.next_fire, 0, entry.id);
            queue_->push(std::move(evt));
            entry.next_fire = entry.next_fire + entry.interval;
        }
    }
}

}  // namespace regimeflow::engine
