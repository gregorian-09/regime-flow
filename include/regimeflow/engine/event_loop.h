#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/events/dispatcher.h"
#include "regimeflow/events/event_queue.h"

#include <functional>
#include <vector>

namespace regimeflow::engine {

class EventLoop {
public:
    using Hook = std::function<void(const events::Event&)>;
    using ProgressCallback = std::function<void(size_t processed, size_t remaining)>;

    explicit EventLoop(events::EventQueue* queue);

    void set_dispatcher(events::EventDispatcher* dispatcher);
    void add_pre_hook(Hook hook);
    void add_post_hook(Hook hook);
    void set_progress_callback(ProgressCallback callback);

    void run();
    void run_until(Timestamp end_time);
    bool step();
    void stop();

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
