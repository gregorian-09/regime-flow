/**
 * @file timer_service.h
 * @brief RegimeFlow regimeflow timer service declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/events/event_queue.h"

#include <string>
#include <unordered_map>

namespace regimeflow::engine {

/**
 * @brief Schedules recurring timer events into an event queue.
 */
class TimerService {
public:
    /**
     * @brief Construct a timer service.
     * @param queue Event queue to schedule into.
     */
    explicit TimerService(events::EventQueue* queue);

    /**
     * @brief Schedule a recurring timer.
     * @param id Timer identifier.
     * @param interval Interval between firings.
     * @param start First fire time.
     */
    void schedule(const std::string& id, Duration interval, Timestamp start);
    /**
     * @brief Cancel a scheduled timer.
     * @param id Timer identifier.
     */
    void cancel(const std::string& id);
    /**
     * @brief Notify the timer service of time advancement.
     * @param now Current time.
     */
    void on_time_advance(Timestamp now);

private:
    /**
     * @brief Internal timer entry.
     */
    struct TimerEntry {
        std::string id;
        Duration interval{Duration::microseconds(0)};
        Timestamp next_fire;
    };

    events::EventQueue* queue_ = nullptr;
    std::unordered_map<std::string, TimerEntry> timers_;
};

}  // namespace regimeflow::engine
