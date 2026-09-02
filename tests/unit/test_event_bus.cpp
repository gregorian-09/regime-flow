#include <gtest/gtest.h>

#include "regimeflow/live/event_bus.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

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

    TEST(EventBus, RejectsPublicationAfterStopBegins) {
        regimeflow::live::EventBus bus;
        bus.start();
        bus.stop();

        regimeflow::live::LiveMessage message;
        message.topic = regimeflow::live::LiveTopic::System;
        message.payload = std::string("after-stop");

        EXPECT_FALSE(bus.try_publish(std::move(message)));
    }

    TEST(EventBus, SerializesConcurrentPublishersDuringShutdown) {
        regimeflow::live::EventBus bus;
        bus.start();

        std::atomic<bool> keep_publishing{true};
        std::vector<std::thread> publishers;
        for (size_t index = 0; index < 4; ++index) {
            publishers.emplace_back([&] {
                while (keep_publishing.load(std::memory_order_relaxed)) {
                    regimeflow::live::LiveMessage message;
                    message.topic = regimeflow::live::LiveTopic::System;
                    message.payload = std::string("concurrent");
                    static_cast<void>(bus.try_publish(std::move(message)));
                }
            });
        }

        bus.stop();
        keep_publishing.store(false, std::memory_order_relaxed);
        for (auto& publisher : publishers) {
            publisher.join();
        }

        regimeflow::live::LiveMessage message;
        message.topic = regimeflow::live::LiveTopic::System;
        message.payload = std::string("after-stop");
        EXPECT_FALSE(bus.try_publish(std::move(message)));
    }
}  // namespace regimeflow::test
