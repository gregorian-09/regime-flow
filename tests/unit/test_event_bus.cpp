#include <gtest/gtest.h>

#include "regimeflow/live/event_bus.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace regimeflow::test
{
    TEST(EventBus, DeliversMarketDataMessages) {
        regimeflow::live::EventBus bus;
        bus.start();

        std::mutex mutex;
        std::condition_variable cv;
        int received = 0;

        const auto sub = bus.subscribe(regimeflow::live::LiveTopic::MarketData, [&](const regimeflow::live::LiveMessage& msg) {
            if (std::get_if<regimeflow::live::MarketDataUpdate>(&msg.payload)) {
                std::lock_guard<std::mutex> lock(mutex);
                ++received;
                cv.notify_one();
            }
        });

        regimeflow::data::Tick tick;
        tick.symbol = regimeflow::SymbolRegistry::instance().intern("AAA");
        tick.timestamp = regimeflow::Timestamp(123);
        tick.price = 10.0;
        tick.quantity = 1.0;

        regimeflow::live::MarketDataUpdate update;
        update.data = tick;

        regimeflow::live::LiveMessage msg;
        msg.topic = regimeflow::live::LiveTopic::MarketData;
        msg.payload = update;
        bus.publish(std::move(msg));

        std::unique_lock<std::mutex> lock(mutex);
        ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(5), [&] { return received > 0; }));

        EXPECT_EQ(received, 1);

        bus.unsubscribe(sub);
        bus.stop();
    }

    TEST(EventBus, UnsubscribeStopsDelivery) {
        regimeflow::live::EventBus bus;
        bus.start();

        std::atomic<int> received{0};
        const auto sub = bus.subscribe(regimeflow::live::LiveTopic::System, [&](const regimeflow::live::LiveMessage&) {
            received.fetch_add(1);
        });

        bus.unsubscribe(sub);
        regimeflow::live::LiveMessage msg;
        msg.topic = regimeflow::live::LiveTopic::System;
        msg.payload = std::string("ping");
        bus.publish(std::move(msg));

        bus.stop();
        EXPECT_EQ(received.load(), 0);
    }
}  // namespace regimeflow::test
