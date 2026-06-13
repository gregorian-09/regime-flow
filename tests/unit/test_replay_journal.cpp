#include "regimeflow/engine/replay_journal.h"
#include "regimeflow/live/types.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace regimeflow::tests
{
    namespace {
        std::filesystem::path temp_journal_path(const char* name) {
            return std::filesystem::temp_directory_path() / name;
        }
    }  // namespace

    TEST(ReplayJournal, RoundTripsBarEvents) {
        data::Bar bar;
        bar.timestamp = Timestamp(1'700'000'000'000'123);
        bar.symbol = 42;
        bar.open = 100.0;
        bar.high = 102.0;
        bar.low = 99.5;
        bar.close = 101.25;
        bar.volume = 1000;
        bar.trade_count = 12;
        bar.vwap = 100.75;

        auto event = events::make_market_event(bar);
        event.sequence = 7;
        auto encoded = engine::serialize_replay_event(event);
        ASSERT_TRUE(encoded.is_ok()) << encoded.error().to_string();

        auto decoded = engine::parse_replay_event(encoded.value());
        ASSERT_TRUE(decoded.is_ok()) << decoded.error().to_string();
        EXPECT_EQ(decoded.value().timestamp.microseconds(), event.timestamp.microseconds());
        EXPECT_EQ(decoded.value().type, events::EventType::Market);
        EXPECT_EQ(decoded.value().sequence, 7U);
        const auto* payload = std::get_if<events::MarketEventPayload>(&decoded.value().payload);
        ASSERT_NE(payload, nullptr);
        ASSERT_EQ(payload->kind, events::MarketEventKind::Bar);
        const auto* decoded_bar = std::get_if<data::Bar>(&payload->data);
        ASSERT_NE(decoded_bar, nullptr);
        EXPECT_EQ(decoded_bar->symbol, bar.symbol);
        EXPECT_DOUBLE_EQ(decoded_bar->close, bar.close);
        EXPECT_EQ(decoded_bar->volume, bar.volume);
    }

    TEST(ReplayJournal, WritesAndReadsJsonlJournal) {
        const auto path = temp_journal_path("regimeflow_replay_journal_test.jsonl");
        std::filesystem::remove(path);
        {
            engine::ReplayJournalWriter writer(path.string());
            auto system = events::make_system_event(events::SystemEventKind::TradingHalt,
                                                    Timestamp(10), 99, "risk_gate");
            ASSERT_TRUE(writer.append(system).is_ok());
            auto fill = events::make_order_event(events::OrderEventKind::Fill,
                                                 Timestamp(11), 123, 456, 10.0, 101.5,
                                                 42, 1.25, true, 0.5, 12, "SIM");
            ASSERT_TRUE(writer.append(fill).is_ok());
        }

        auto events = engine::read_replay_journal(path.string());
        ASSERT_TRUE(events.is_ok()) << events.error().to_string();
        ASSERT_EQ(events.value().size(), 2U);
        EXPECT_EQ(events.value()[0].type, events::EventType::System);
        EXPECT_EQ(events.value()[1].type, events::EventType::Order);
        const auto* order = std::get_if<events::OrderEventPayload>(&events.value()[1].payload);
        ASSERT_NE(order, nullptr);
        EXPECT_EQ(order->kind, events::OrderEventKind::Fill);
        EXPECT_EQ(order->venue, "SIM");
        EXPECT_DOUBLE_EQ(order->transaction_cost, 0.5);
        std::filesystem::remove(path);
    }

    TEST(ReplayJournal, ConvertsLiveMarketDataToEngineEvent) {
        data::Quote quote;
        quote.timestamp = Timestamp(77);
        quote.symbol = 9;
        quote.bid = 10.0;
        quote.ask = 10.2;
        quote.bid_size = 100.0;
        quote.ask_size = 120.0;
        live::MarketDataUpdate update;
        update.data = quote;

        const auto event = live::to_engine_event(update);
        EXPECT_EQ(event.timestamp.microseconds(), quote.timestamp.microseconds());
        EXPECT_EQ(event.symbol, quote.symbol);
        const auto* payload = std::get_if<events::MarketEventPayload>(&event.payload);
        ASSERT_NE(payload, nullptr);
        EXPECT_EQ(payload->kind, events::MarketEventKind::Quote);
        const auto* decoded_quote = std::get_if<data::Quote>(&payload->data);
        ASSERT_NE(decoded_quote, nullptr);
        EXPECT_DOUBLE_EQ(decoded_quote->ask, quote.ask);
    }
}
