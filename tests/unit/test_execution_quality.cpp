#include <gtest/gtest.h>
#include "test_time.h"

#include "regimeflow/live/execution_quality.h"
#include "regimeflow/live/live_order_manager.h"

namespace regimeflow::test
{
    TEST(ExecutionQualityTracker, AggregatesLatencyAndSlippage) {
        regimeflow::live::ExecutionQualityTracker tracker;

        regimeflow::live::LiveOrder order;
        order.internal_id = 7;
        order.broker_order_id = "BRK-7";
        order.symbol = "AAPL";
        order.side = regimeflow::engine::OrderSide::Buy;
        order.quantity = 10.0;
        order.limit_price = 100.0;
        order.created_at = fixed_timestamp();
        order.submitted_at = fixed_timestamp(1'000);

        tracker.record_submitted(order);

        regimeflow::live::ExecutionReport ack;
        ack.broker_order_id = order.broker_order_id;
        ack.symbol = order.symbol;
        ack.quantity = 0.0;
        ack.price = 0.0;
        ack.status = regimeflow::live::LiveOrderStatus::New;
        ack.timestamp = fixed_timestamp(6'000);
        tracker.record_execution_report(order, ack);

        regimeflow::live::ExecutionReport fill;
        fill.broker_order_id = order.broker_order_id;
        fill.symbol = order.symbol;
        fill.quantity = 10.0;
        fill.price = 100.25;
        fill.status = regimeflow::live::LiveOrderStatus::Filled;
        fill.timestamp = fixed_timestamp(11'000);
        tracker.record_execution_report(order, fill);

        const auto& snapshot = tracker.snapshot();
        EXPECT_EQ(snapshot.submitted, 1u);
        EXPECT_EQ(snapshot.acknowledged, 1u);
        EXPECT_EQ(snapshot.filled, 1u);
        EXPECT_EQ(snapshot.slippage_observations, 1u);
        EXPECT_DOUBLE_EQ(snapshot.average_ack_latency_ms, 5.0);
        EXPECT_DOUBLE_EQ(snapshot.average_fill_latency_ms, 10.0);
        EXPECT_DOUBLE_EQ(snapshot.average_signed_slippage_bps, 25.0);
        EXPECT_DOUBLE_EQ(snapshot.average_absolute_slippage_bps, 25.0);
        ASSERT_EQ(tracker.samples().size(), 2u);
        EXPECT_EQ(tracker.samples().back().broker_order_id, "BRK-7");
    }

    TEST(ExecutionQualityTracker, ComputesSellSlippageAndRejectionRate) {
        regimeflow::live::ExecutionQualityTracker tracker;

        regimeflow::live::LiveOrder order;
        order.internal_id = 8;
        order.broker_order_id = "BRK-8";
        order.symbol = "MSFT";
        order.side = regimeflow::engine::OrderSide::Sell;
        order.quantity = 5.0;
        order.limit_price = 200.0;
        order.created_at = fixed_timestamp();
        order.submitted_at = fixed_timestamp();

        tracker.record_submitted(order);
        tracker.record_submit_rejected(fixed_timestamp(1'000));

        regimeflow::live::ExecutionReport fill;
        fill.broker_order_id = order.broker_order_id;
        fill.symbol = order.symbol;
        fill.quantity = 5.0;
        fill.price = 199.0;
        fill.status = regimeflow::live::LiveOrderStatus::Filled;
        fill.timestamp = fixed_timestamp(2'000);
        tracker.record_execution_report(order, fill);

        const auto& snapshot = tracker.snapshot();
        EXPECT_EQ(snapshot.submitted, 2u);
        EXPECT_EQ(snapshot.submit_rejected, 1u);
        EXPECT_EQ(snapshot.filled, 1u);
        EXPECT_DOUBLE_EQ(snapshot.rejection_rate, 0.5);
        EXPECT_DOUBLE_EQ(snapshot.average_signed_slippage_bps, 50.0);
    }

    TEST(ExecutionQualityTracker, AttributesEffectiveSpreadFromReferenceQuote) {
        regimeflow::live::ExecutionQualityTracker tracker;

        regimeflow::live::LiveOrder order;
        order.internal_id = 9;
        order.broker_order_id = "BRK-9";
        order.symbol = "AAPL";
        order.side = regimeflow::engine::OrderSide::Buy;
        order.quantity = 10.0;
        order.created_at = fixed_timestamp();
        order.submitted_at = fixed_timestamp();

        data::Quote quote;
        quote.symbol = SymbolRegistry::instance().intern("AAPL");
        quote.timestamp = fixed_timestamp();
        quote.bid = 99.9;
        quote.ask = 100.1;
        tracker.record_reference_quote(order.internal_id, quote);
        tracker.record_submitted(order);

        regimeflow::live::ExecutionReport fill;
        fill.broker_order_id = order.broker_order_id;
        fill.symbol = order.symbol;
        fill.quantity = 10.0;
        fill.price = 100.1;
        fill.status = regimeflow::live::LiveOrderStatus::Filled;
        fill.timestamp = fixed_timestamp(1'000);
        tracker.record_execution_report(order, fill);

        const auto& snapshot = tracker.snapshot();
        EXPECT_EQ(snapshot.spread_observations, 1u);
        EXPECT_NEAR(snapshot.average_effective_spread_bps, 10.0, 1e-9);
        ASSERT_EQ(tracker.samples().size(), 1u);
        EXPECT_DOUBLE_EQ(tracker.samples().back().reference_mid_price, 100.0);
        EXPECT_NEAR(tracker.samples().back().reference_spread_bps, 20.0, 1e-9);
    }

    TEST(ExecutionQualityTracker, AggregatesVenueRollups) {
        regimeflow::live::ExecutionQualityTracker tracker;

        regimeflow::live::LiveOrder order;
        order.internal_id = 10;
        order.broker_order_id = "BRK-10";
        order.symbol = "AAPL";
        order.venue = "lit_primary";
        order.side = regimeflow::engine::OrderSide::Buy;
        order.quantity = 10.0;
        order.limit_price = 100.0;
        order.created_at = fixed_timestamp();
        order.submitted_at = fixed_timestamp();

        regimeflow::live::ExecutionReport fill;
        fill.broker_order_id = order.broker_order_id;
        fill.symbol = order.symbol;
        fill.quantity = 10.0;
        fill.price = 100.2;
        fill.status = regimeflow::live::LiveOrderStatus::Filled;
        fill.timestamp = fixed_timestamp(5'000);

        tracker.record_submitted(order);
        tracker.record_execution_report(order, fill);

        order.internal_id = 11;
        order.broker_order_id = "BRK-11";
        order.venue = "dark_pool";
        fill.broker_order_id = order.broker_order_id;
        fill.price = 99.9;
        fill.timestamp = fixed_timestamp(8'000);
        tracker.record_submitted(order);
        tracker.record_execution_report(order, fill);

        const auto& venues = tracker.snapshot().venue_summaries;
        ASSERT_EQ(venues.size(), 2u);
        EXPECT_EQ(venues[0].venue, "dark_pool");
        EXPECT_EQ(venues[0].fills, 1u);
        EXPECT_DOUBLE_EQ(venues[0].quantity, 10.0);
        EXPECT_EQ(venues[1].venue, "lit_primary");
        EXPECT_EQ(venues[1].fills, 1u);
        EXPECT_NEAR(venues[1].average_signed_slippage_bps, 20.0, 1e-9);
    }

}  // namespace regimeflow::test
