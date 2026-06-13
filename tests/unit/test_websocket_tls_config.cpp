#include <gtest/gtest.h>

#include "regimeflow/data/websocket_feed.h"

namespace regimeflow::test
{
#if defined(REGIMEFLOW_USE_BOOST_BEAST) && defined(REGIMEFLOW_USE_OPENSSL)
    TEST(WebSocketFeedTlsConfig, InvalidCaBundlePathFails) {
        data::WebSocketFeed::Config cfg;
        cfg.url = "wss://example.com/feed";
        cfg.verify_tls = true;
        cfg.ca_bundle_path = "/nonexistent/ca_bundle.pem";

        const data::WebSocketFeed feed(cfg);
        const auto res = feed.validate_tls_config();
        EXPECT_TRUE(res.is_err());
    }

    TEST(WebSocketFeedTlsConfig, VerifyDisabledPassesWithoutBundle) {
        data::WebSocketFeed::Config cfg;
        cfg.url = "wss://example.com/feed";
        cfg.verify_tls = false;

        const data::WebSocketFeed feed(cfg);
        const auto res = feed.validate_tls_config();
        EXPECT_TRUE(res.is_ok());
    }
#else
    TEST(WebSocketFeedTlsConfig, SkippedWithoutDependencies) {
        GTEST_SKIP()
            << "Skipped because REGIMEFLOW_USE_BOOST_BEAST or REGIMEFLOW_USE_OPENSSL is not defined; "
               "configure with -DENABLE_WEBSOCKET=ON -DENABLE_OPENSSL=ON and provide Boost.Beast/OpenSSL.";
    }
#endif
}  // namespace regimeflow::test
