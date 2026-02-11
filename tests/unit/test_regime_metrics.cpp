#include <gtest/gtest.h>

#include "regimeflow/metrics/metrics_tracker.h"
#include "regimeflow/engine/portfolio.h"

namespace regimeflow::test {

TEST(RegimeMetrics, TracksRegimePerformanceAndTransitions) {
    metrics::MetricsTracker tracker;
    engine::Portfolio portfolio(100.0);

    auto symbol = SymbolRegistry::instance().intern("AAA");
    engine::Fill fill;
    fill.symbol = symbol;
    fill.quantity = 1.0;
    fill.price = 100.0;
    fill.timestamp = Timestamp(1);
    portfolio.update_position(fill);

    tracker.update(Timestamp(1), portfolio, regime::RegimeType::Bull);
    portfolio.mark_to_market(symbol, 110.0, Timestamp(2));
    tracker.update(Timestamp(2), portfolio, regime::RegimeType::Bull);
    portfolio.mark_to_market(symbol, 100.0, Timestamp(3));
    tracker.update(Timestamp(3), portfolio, regime::RegimeType::Bear);

    const auto& regimes = tracker.regime_attribution().results();
    ASSERT_TRUE(regimes.find(regime::RegimeType::Bull) != regimes.end());
    ASSERT_TRUE(regimes.find(regime::RegimeType::Bear) != regimes.end());
    EXPECT_EQ(regimes.at(regime::RegimeType::Bull).observations, 2);
    EXPECT_EQ(regimes.at(regime::RegimeType::Bear).observations, 1);

    const auto& transitions = tracker.transition_metrics().results();
    auto key = std::make_pair(regime::RegimeType::Bull, regime::RegimeType::Bear);
    ASSERT_TRUE(transitions.find(key) != transitions.end());
    EXPECT_EQ(transitions.at(key).observations, 1);
}

}  // namespace regimeflow::test
