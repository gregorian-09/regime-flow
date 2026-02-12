/**
 * @file event_generator.h
 * @brief RegimeFlow regimeflow event generator declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/events/event.h"
#include "regimeflow/events/event_queue.h"

#include <memory>
#include <vector>

namespace regimeflow::engine {

/**
 * @brief Generates events from data iterators into an event queue.
 */
class EventGenerator {
public:
    /**
     * @brief Event generation options.
     */
    struct Config {
        /**
         * @brief Emit start-of-day markers.
         */
        bool emit_start_of_day = true;
        /**
         * @brief Emit end-of-day markers.
         */
        bool emit_end_of_day = true;
        /**
         * @brief Emit periodic regime-check events.
         */
        bool emit_regime_check = false;
        /**
         * @brief Interval between regime checks.
         */
        Duration regime_check_interval = Duration::minutes(5);
    };

    /**
     * @brief Construct with a single data iterator.
     * @param iterator Data iterator.
     * @param queue Destination event queue.
     */
    EventGenerator(std::unique_ptr<data::DataIterator> iterator,
                   events::EventQueue* queue);
    /**
     * @brief Construct with a single data iterator and config.
     * @param iterator Data iterator.
     * @param queue Destination event queue.
     * @param config Generator config.
     */
    EventGenerator(std::unique_ptr<data::DataIterator> iterator,
                   events::EventQueue* queue,
                   Config config);
    /**
     * @brief Construct with bar, tick, and order book iterators.
     * @param bar_iterator Bar iterator.
     * @param tick_iterator Tick iterator.
     * @param book_iterator Order book iterator.
     * @param queue Destination event queue.
     */
    EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator,
                   std::unique_ptr<data::TickIterator> tick_iterator,
                   std::unique_ptr<data::OrderBookIterator> book_iterator,
                   events::EventQueue* queue);
    /**
     * @brief Construct with bar, tick, and order book iterators and config.
     * @param bar_iterator Bar iterator.
     * @param tick_iterator Tick iterator.
     * @param book_iterator Order book iterator.
     * @param queue Destination event queue.
     * @param config Generator config.
     */
    EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator,
                   std::unique_ptr<data::TickIterator> tick_iterator,
                   std::unique_ptr<data::OrderBookIterator> book_iterator,
                   events::EventQueue* queue,
                   Config config);

    /**
     * @brief Enqueue all events from the iterators.
     */
    void enqueue_all();

private:
    std::unique_ptr<data::DataIterator> bar_iterator_;
    std::unique_ptr<data::TickIterator> tick_iterator_;
    std::unique_ptr<data::OrderBookIterator> book_iterator_;
    events::EventQueue* queue_ = nullptr;
    Config config_{};
};

}  // namespace regimeflow::engine
