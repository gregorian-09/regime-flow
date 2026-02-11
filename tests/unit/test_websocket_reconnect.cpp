#include <gtest/gtest.h>

#include "regimeflow/data/websocket_feed.h"

#include <thread>

namespace regimeflow::test {

#if defined(REGIMEFLOW_USE_BOOST_BEAST)
TEST(WebSocketFeedReconnect, CallbackReportsBackoffAndSuccess) {
    data::WebSocketFeed::Config cfg;
    cfg.url = "ws://example.com/feed";
    cfg.auto_reconnect = true;
    cfg.reconnect_initial_ms = 1;
    cfg.reconnect_max_ms = 2;

    int connect_calls = 0;
    cfg.connect_override = [&]() -> Result<void> {
        ++connect_calls;
        if (connect_calls < 3) {
            return Error(Error::Code::NetworkError, "simulated");
        }
        return Ok();
    };

    data::WebSocketFeed feed(cfg);
    std::vector<data::WebSocketFeed::ReconnectState> states;
    feed.on_reconnect([&](const data::WebSocketFeed::ReconnectState& state) {
        states.push_back(state);
    });

    feed.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    feed.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    feed.poll();

    ASSERT_GE(states.size(), 3u);
    EXPECT_FALSE(states[0].connected);
    EXPECT_EQ(states[0].attempts, 1);
    EXPECT_GT(states[0].backoff_ms, 0);
    EXPECT_FALSE(states[1].connected);
    EXPECT_EQ(states[1].attempts, 2);
    EXPECT_TRUE(states.back().connected);
}
#else
TEST(WebSocketFeedReconnect, SkippedWithoutBeast) {
    GTEST_SKIP() << "Boost.Beast not available";
}
#endif

}  // namespace regimeflow::test
