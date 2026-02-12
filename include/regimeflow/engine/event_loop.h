/**
 * @file event_loop.h
 * @brief RegimeFlow regimeflow event loop declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/events/dispatcher.h"
#include "regimeflow/events/event_queue.h"

#include <functional>
#include <vector>

namespace regimeflow::engine {

/**
 * @brief Event loop for deterministic backtest execution.
 */
class EventLoop {
public:
    /**
     * @brief Hook called before/after dispatching an event.
     */
    using Hook = std::function<void(const events::Event&)>;
    /**
     * @brief Progress callback for reporting processing counts.
     */
    using ProgressCallback = std::function<void(size_t processed, size_t remaining)>;

    /**
     * @brief Construct an event loop bound to a queue.
     * @param queue Event queue.
     */
    explicit EventLoop(events::EventQueue* queue);

    /**
     * @brief Set the event dispatcher.
     * @param dispatcher Dispatcher to use.
     */
    void set_dispatcher(events::EventDispatcher* dispatcher);
    /**
     * @brief Register a pre-dispatch hook.
     * @param hook Hook to invoke before dispatch.
     */
    void add_pre_hook(Hook hook);
    /**
     * @brief Register a post-dispatch hook.
     * @param hook Hook to invoke after dispatch.
     */
    void add_post_hook(Hook hook);
    /**
     * @brief Register a progress callback.
     * @param callback Progress callback.
     */
    void set_progress_callback(ProgressCallback callback);

    /**
     * @brief Run until the queue is exhausted or stop() is called.
     */
    void run();
    /**
     * @brief Run until a target time is reached.
     * @param end_time Stop time (inclusive).
     */
    void run_until(Timestamp end_time);
    /**
     * @brief Process a single event.
     * @return True if an event was processed.
     */
    bool step();
    /**
     * @brief Request the loop to stop.
     */
    void stop();

    /**
     * @brief Current event loop time.
     * @return Timestamp of last processed event.
     */
    Timestamp current_time() const { return current_time_; }

private:
    events::EventQueue* queue_ = nullptr;
    events::EventDispatcher* dispatcher_ = nullptr;
    std::vector<Hook> pre_hooks_;
    std::vector<Hook> post_hooks_;
    ProgressCallback progress_callback_;
    Timestamp current_time_;
    bool running_ = false;
    size_t processed_ = 0;
};

}  // namespace regimeflow::engine
