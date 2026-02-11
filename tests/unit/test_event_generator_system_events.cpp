#include <gtest/gtest.h>

#include "regimeflow/common/time.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/engine/event_generator.h"
#include "regimeflow/events/event_queue.h"

namespace regimeflow::test {

TEST(EventGeneratorSystemEvents, EmitsEndOfDayEvents) {
    auto sym = SymbolRegistry::instance().intern("AAA");

    data::Bar bar1;
    bar1.symbol = sym;
    bar1.timestamp = Timestamp::from_date(2024, 1, 1);
    bar1.open = 10.0;
    bar1.high = 11.0;
    bar1.low = 9.5;
    bar1.close = 10.5;
    bar1.volume = 100;

    data::Bar bar2 = bar1;
    bar2.timestamp = Timestamp::from_date(2024, 1, 2);

    auto iterator = std::make_unique<data::VectorBarIterator>(
        std::vector<data::Bar>{bar1, bar2});

    events::EventQueue queue;
    engine::EventGenerator::Config cfg;
    cfg.emit_start_of_day = false;
    cfg.emit_end_of_day = true;
    cfg.emit_regime_check = false;
    engine::EventGenerator generator(std::move(iterator), &queue, cfg);
    generator.enqueue_all();

    int eod_count = 0;
    while (auto evt = queue.pop()) {
        auto payload = std::get_if<events::SystemEventPayload>(&evt->payload);
        if (payload && payload->kind == events::SystemEventKind::EndOfDay) {
            ++eod_count;
        }
    }

    EXPECT_EQ(eod_count, 2);
}

TEST(EventGeneratorSystemEvents, EmitsRegimeCheckTimers) {
    auto sym = SymbolRegistry::instance().intern("AAA");

    data::Bar bar1;
    bar1.symbol = sym;
    bar1.timestamp = Timestamp(0);
    bar1.open = 10.0;
    bar1.high = 11.0;
    bar1.low = 9.5;
    bar1.close = 10.5;
    bar1.volume = 100;

    data::Bar bar2 = bar1;
    bar2.timestamp = Timestamp(Duration::minutes(10).total_microseconds());

    auto iterator = std::make_unique<data::VectorBarIterator>(
        std::vector<data::Bar>{bar1, bar2});

    events::EventQueue queue;
    engine::EventGenerator::Config cfg;
    cfg.emit_start_of_day = false;
    cfg.emit_end_of_day = false;
    cfg.emit_regime_check = true;
    cfg.regime_check_interval = Duration::minutes(5);
    engine::EventGenerator generator(std::move(iterator), &queue, cfg);
    generator.enqueue_all();

    int timer_count = 0;
    while (auto evt = queue.pop()) {
        auto payload = std::get_if<events::SystemEventPayload>(&evt->payload);
        if (payload && payload->kind == events::SystemEventKind::Timer &&
            payload->id == "regime_check") {
            ++timer_count;
        }
    }

    EXPECT_EQ(timer_count, 2);
}

}  // namespace regimeflow::test
