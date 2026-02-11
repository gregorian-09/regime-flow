#include <gtest/gtest.h>

#include "regimeflow/data/websocket_feed.h"

namespace regimeflow::test {

#if defined(REGIMEFLOW_USE_BOOST_BEAST) && defined(REGIMEFLOW_USE_OPENSSL)
TEST(WebSocketFeedTlsConfig, InvalidCaBundlePathFails) {
    data::WebSocketFeed::Config cfg;
    cfg.url = "wss://example.com/feed";
    cfg.verify_tls = true;
    cfg.ca_bundle_path = "/nonexistent/ca_bundle.pem";

    data::WebSocketFeed feed(cfg);
    auto res = feed.validate_tls_config();
    EXPECT_TRUE(res.is_err());
}

TEST(WebSocketFeedTlsConfig, VerifyDisabledPassesWithoutBundle) {
    data::WebSocketFeed::Config cfg;
    cfg.url = "wss://example.com/feed";
    cfg.verify_tls = false;

    data::WebSocketFeed feed(cfg);
    auto res = feed.validate_tls_config();
    EXPECT_TRUE(res.is_ok());
}
#else
TEST(WebSocketFeedTlsConfig, SkippedWithoutDependencies) {
    GTEST_SKIP() << "Boost.Beast/OpenSSL not available";
}
#endif

}  // namespace regimeflow::test
