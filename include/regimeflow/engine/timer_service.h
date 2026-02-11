#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/events/event_queue.h"

#include <string>
#include <unordered_map>

namespace regimeflow::engine {

class TimerService {
public:
    explicit TimerService(events::EventQueue* queue);

    void schedule(const std::string& id, Duration interval, Timestamp start);
    void cancel(const std::string& id);
    void on_time_advance(Timestamp now);

private:
    struct TimerEntry {
        std::string id;
        Duration interval{Duration::microseconds(0)};
        Timestamp next_fire;
    };

    events::EventQueue* queue_ = nullptr;
    std::unordered_map<std::string, TimerEntry> timers_;
};

}  // namespace regimeflow::engine
