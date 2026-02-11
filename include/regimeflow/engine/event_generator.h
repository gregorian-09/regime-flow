#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/events/event.h"
#include "regimeflow/events/event_queue.h"

#include <memory>
#include <vector>

namespace regimeflow::engine {

class EventGenerator {
public:
    struct Config {
        bool emit_start_of_day = true;
        bool emit_end_of_day = true;
        bool emit_regime_check = false;
        Duration regime_check_interval = Duration::minutes(5);
    };

    EventGenerator(std::unique_ptr<data::DataIterator> iterator,
                   events::EventQueue* queue);
    EventGenerator(std::unique_ptr<data::DataIterator> iterator,
                   events::EventQueue* queue,
                   Config config);
    EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator,
                   std::unique_ptr<data::TickIterator> tick_iterator,
                   std::unique_ptr<data::OrderBookIterator> book_iterator,
                   events::EventQueue* queue);
    EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator,
                   std::unique_ptr<data::TickIterator> tick_iterator,
                   std::unique_ptr<data::OrderBookIterator> book_iterator,
                   events::EventQueue* queue,
                   Config config);

    void enqueue_all();

private:
    std::unique_ptr<data::DataIterator> bar_iterator_;
    std::unique_ptr<data::TickIterator> tick_iterator_;
    std::unique_ptr<data::OrderBookIterator> book_iterator_;
    events::EventQueue* queue_ = nullptr;
    Config config_{};
};

}  // namespace regimeflow::engine
