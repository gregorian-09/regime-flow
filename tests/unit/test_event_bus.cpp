#include <gtest/gtest.h>

#include "regimeflow/live/event_bus.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace regimeflow::test {

TEST(EventBus, DeliversMarketDataMessages) {
    regimeflow::live::EventBus bus;
    bus.start();

    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> received{0};

    auto sub = bus.subscribe(regimeflow::live::LiveTopic::MarketData, [&](const regimeflow::live::LiveMessage& msg) {
        auto payload = std::get_if<regimeflow::live::MarketDataUpdate>(&msg.payload);
        if (payload) {
            received.fetch_add(1);
            std::lock_guard<std::mutex> lock(mutex);
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
    cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return received.load() > 0; });

    EXPECT_EQ(received.load(), 1);

    bus.unsubscribe(sub);
    bus.stop();
}

TEST(EventBus, UnsubscribeStopsDelivery) {
    regimeflow::live::EventBus bus;
    bus.start();

    std::atomic<int> received{0};
    auto sub = bus.subscribe(regimeflow::live::LiveTopic::System, [&](const regimeflow::live::LiveMessage&) {
        received.fetch_add(1);
    });

    bus.unsubscribe(sub);
    regimeflow::live::LiveMessage msg;
    msg.topic = regimeflow::live::LiveTopic::System;
    msg.payload = std::string("ping");
    bus.publish(std::move(msg));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(received.load(), 0);

    bus.stop();
}

}  // namespace regimeflow::test
