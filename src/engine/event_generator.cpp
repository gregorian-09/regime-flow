#include "regimeflow/engine/event_generator.h"

#include <algorithm>

namespace regimeflow::engine {

EventGenerator::EventGenerator(std::unique_ptr<data::DataIterator> iterator,
                               events::EventQueue* queue)
    : bar_iterator_(std::move(iterator)), queue_(queue), config_() {}

EventGenerator::EventGenerator(std::unique_ptr<data::DataIterator> iterator,
                               events::EventQueue* queue,
                               Config config)
    : bar_iterator_(std::move(iterator)), queue_(queue), config_(config) {}

EventGenerator::EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator,
                               std::unique_ptr<data::TickIterator> tick_iterator,
                               std::unique_ptr<data::OrderBookIterator> book_iterator,
                               events::EventQueue* queue)
    : bar_iterator_(std::move(bar_iterator)),
      tick_iterator_(std::move(tick_iterator)),
      book_iterator_(std::move(book_iterator)),
      queue_(queue),
      config_() {}

EventGenerator::EventGenerator(std::unique_ptr<data::DataIterator> bar_iterator,
                               std::unique_ptr<data::TickIterator> tick_iterator,
                               std::unique_ptr<data::OrderBookIterator> book_iterator,
                               events::EventQueue* queue,
                               Config config)
    : bar_iterator_(std::move(bar_iterator)),
      tick_iterator_(std::move(tick_iterator)),
      book_iterator_(std::move(book_iterator)),
      queue_(queue),
      config_(config) {}

void EventGenerator::enqueue_all() {
    if (!queue_) {
        return;
    }

    std::vector<events::Event> events;

    if (bar_iterator_) {
        bar_iterator_->reset();
        while (bar_iterator_->has_next()) {
            data::Bar bar = bar_iterator_->next();
            events.push_back(events::make_market_event(bar));
        }
    }

    if (tick_iterator_) {
        tick_iterator_->reset();
        while (tick_iterator_->has_next()) {
            data::Tick tick = tick_iterator_->next();
            events.push_back(events::make_market_event(tick));
        }
    }

    if (book_iterator_) {
        book_iterator_->reset();
        while (book_iterator_->has_next()) {
            data::OrderBook book = book_iterator_->next();
            events.push_back(events::make_market_event(book));
        }
    }

    std::sort(events.begin(), events.end(), [](const events::Event& a, const events::Event& b) {
        if (a.timestamp != b.timestamp) {
            return a.timestamp < b.timestamp;
        }
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        if (a.symbol != b.symbol) {
            return a.symbol < b.symbol;
        }
        auto a_payload = std::get_if<events::MarketEventPayload>(&a.payload);
        auto b_payload = std::get_if<events::MarketEventPayload>(&b.payload);
        if (a_payload && b_payload && a_payload->kind != b_payload->kind) {
            return a_payload->kind < b_payload->kind;
        }
        return false;
    });

    if (!events.empty()) {
        std::vector<events::Event> system_events;
        std::string current_day = events.front().timestamp.to_string("%Y%m%d");
        Timestamp last_ts = events.front().timestamp;
        bool first_day = true;
        for (const auto& evt : events) {
            auto day = evt.timestamp.to_string("%Y%m%d");
            if (first_day || day != current_day) {
                if (!first_day && config_.emit_end_of_day) {
                    system_events.push_back(events::make_system_event(
                        events::SystemEventKind::EndOfDay, last_ts));
                }
                if (config_.emit_start_of_day) {
                    system_events.push_back(events::make_system_event(
                        events::SystemEventKind::DayStart, evt.timestamp));
                }
                current_day = day;
                first_day = false;
            }
            last_ts = evt.timestamp;
        }
        if (!events.empty() && config_.emit_end_of_day) {
            system_events.push_back(events::make_system_event(
                events::SystemEventKind::EndOfDay, last_ts));
        }

        if (config_.emit_regime_check &&
            config_.regime_check_interval.total_microseconds() > 0) {
            Timestamp cursor = events.front().timestamp + config_.regime_check_interval;
            Timestamp end = events.back().timestamp;
            while (cursor <= end) {
                system_events.push_back(events::make_system_event(
                    events::SystemEventKind::Timer, cursor, 0, "regime_check"));
                cursor = cursor + config_.regime_check_interval;
            }
        }

        if (!system_events.empty()) {
            events.insert(events.end(),
                          std::make_move_iterator(system_events.begin()),
                          std::make_move_iterator(system_events.end()));
            std::sort(events.begin(), events.end(), [](const events::Event& a,
                                                      const events::Event& b) {
                if (a.timestamp != b.timestamp) {
                    return a.timestamp < b.timestamp;
                }
                if (a.priority != b.priority) {
                    return a.priority < b.priority;
                }
                if (a.symbol != b.symbol) {
                    return a.symbol < b.symbol;
                }
                auto a_payload = std::get_if<events::MarketEventPayload>(&a.payload);
                auto b_payload = std::get_if<events::MarketEventPayload>(&b.payload);
                if (a_payload && b_payload && a_payload->kind != b_payload->kind) {
                    return a_payload->kind < b_payload->kind;
                }
                return false;
            });
        }
    }

    for (auto& evt : events) {
        queue_->push(std::move(evt));
    }
}

}  // namespace regimeflow::engine
