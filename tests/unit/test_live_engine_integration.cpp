#include <gtest/gtest.h>

#include "regimeflow/live/live_engine.h"
#include "regimeflow/engine/replay_journal.h"
#include "regimeflow/strategy/strategy_factory.h"
#include "regimeflow/strategy/strategy.h"

#include "test_time.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>
#include <thread>

namespace regimeflow::test
{
    namespace {
        template <typename Predicate>
        bool wait_until(Predicate&& predicate,
                        const std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            do {
                if (predicate()) {
                    return true;
                }
                std::this_thread::yield();
            } while (std::chrono::steady_clock::now() < deadline);
            return predicate();
        }

        std::string fresh_log_dir(const std::string_view name) {
            const auto path = std::filesystem::temp_directory_path() / name;
            std::filesystem::remove_all(path);
            return path.string();
        }
    }  // namespace

    class MockBrokerAdapter final : public live::BrokerAdapter {
    public:
        Result<void> connect() override {
            connected_ = true;
            return Ok();
        }

        Result<void> disconnect() override {
            connected_ = false;
            return Ok();
        }

        bool is_connected() const override { return connected_; }

        void subscribe_market_data(const std::vector<std::string>& symbols) override {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                subscribed_ = symbols;
            }
            cv_.notify_all();
        }

        void unsubscribe_market_data(const std::vector<std::string>&) override {}

        Result<std::string> submit_order(const engine::Order& order) override {
            std::string broker_id;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++submit_count_;
                last_submitted_order_ = order;
                broker_id = "B" + std::to_string(submit_count_);
                if (auto_fill_) {
                    live::ExecutionReport report;
                    report.broker_order_id = broker_id;
                    report.symbol = SymbolRegistry::instance().lookup(order.symbol);
                    report.side = order.side;
                    report.quantity = std::abs(order.quantity);
                    report.price = last_price_ > 0 ? last_price_ : 100.0;
                    report.status = live::LiveOrderStatus::Filled;
                    report.timestamp = fixed_timestamp(submit_count_);
                    pending_execs_.push_back(report);
                } else {
                    live::ExecutionReport report;
                    report.broker_order_id = broker_id;
                    report.symbol = SymbolRegistry::instance().lookup(order.symbol);
                    report.side = order.side;
                    report.quantity = std::abs(order.quantity);
                    report.price = order.limit_price > 0.0 ? order.limit_price : (last_price_ > 0 ? last_price_ : 100.0);
                    report.status = live::LiveOrderStatus::New;
                    report.timestamp = fixed_timestamp(submit_count_);
                    open_orders_.push_back(report);
                }
            }
            cv_.notify_all();
            return Result<std::string>(broker_id);
        }

        Result<void> cancel_order(const std::string& broker_order_id) override {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++cancel_count_;
                std::erase_if(open_orders_, [&](const live::ExecutionReport& report) {
                    return report.broker_order_id == broker_order_id;
                });
            }
            cv_.notify_all();
            return Ok();
        }
        Result<void> modify_order(const std::string&, const engine::OrderModification&) override {
            return Ok();
        }

        live::AccountInfo get_account_info() override {
            account_calls_.fetch_add(1);
            std::lock_guard<std::mutex> lock(mutex_);
            auto info = account_info_;
            cv_.notify_all();
            return info;
        }

        std::vector<live::Position> get_positions() override {
            positions_calls_.fetch_add(1);
            std::lock_guard<std::mutex> lock(mutex_);
            auto positions = positions_;
            cv_.notify_all();
            return positions;
        }
        std::vector<live::ExecutionReport> get_open_orders() override {
            std::lock_guard<std::mutex> lock(mutex_);
            return open_orders_;
        }

        void on_market_data(std::function<void(const live::MarketDataUpdate&)> cb) override {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                market_cb_ = std::move(cb);
            }
            cv_.notify_all();
        }

        void on_execution_report(std::function<void(const live::ExecutionReport&)> cb) override {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                exec_cb_ = std::move(cb);
            }
            cv_.notify_all();
        }

        void on_position_update(std::function<void(const live::Position&)>) override {}

        int max_orders_per_second() const override { return 5; }
        int max_messages_per_second() const override { return 1000; }
        bool supports_tif(engine::OrderType, engine::TimeInForce) const override { return true; }

        void poll() override {
            std::vector<live::ExecutionReport> pending;
            std::function<void(const live::ExecutionReport&)> exec_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending.swap(pending_execs_);
                exec_cb = exec_cb_;
            }
            if (exec_cb) {
                for (const auto& report : pending) {
                    exec_cb(report);
                }
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                polled_exec_count_ += static_cast<int>(pending.size());
            }
            cv_.notify_all();
        }

        void emit_bar(const data::Bar& bar) {
            std::function<void(const live::MarketDataUpdate&)> market_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                last_price_ = bar.close;
                market_cb = market_cb_;
            }
            if (market_cb) {
                live::MarketDataUpdate update;
                update.data = bar;
                market_cb(update);
            }
        }

        int submit_count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return submit_count_;
        }

        int cancel_count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return cancel_count_;
        }

        std::optional<engine::Order> last_submitted_order() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return last_submitted_order_;
        }

        void set_account_info(const live::AccountInfo& info) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                account_info_ = info;
            }
            cv_.notify_all();
        }

        void set_positions(std::vector<live::Position> positions) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                positions_ = std::move(positions);
            }
            cv_.notify_all();
        }

        void set_open_orders(std::vector<live::ExecutionReport> open_orders) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                open_orders_ = std::move(open_orders);
            }
            cv_.notify_all();
        }

        int account_calls() const { return account_calls_.load(); }
        int positions_calls() const { return positions_calls_.load(); }

        void set_auto_fill(const bool value) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto_fill_ = value;
            }
            cv_.notify_all();
        }

        void emit_execution_report(const live::ExecutionReport& report) {
            std::function<void(const live::ExecutionReport&)> exec_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                exec_cb = exec_cb_;
            }
            if (exec_cb) {
                exec_cb(report);
            }
        }

        bool wait_for_callbacks(const std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) const {
            std::unique_lock<std::mutex> lock(mutex_);
            return cv_.wait_for(lock, timeout, [&] {
                return static_cast<bool>(market_cb_) && static_cast<bool>(exec_cb_);
            });
        }

        bool wait_for_submit_count(const int expected,
                                   const std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) const {
            std::unique_lock<std::mutex> lock(mutex_);
            return cv_.wait_for(lock, timeout, [&] { return submit_count_ >= expected; });
        }

        bool wait_for_reconcile_calls(const int expected_account_calls,
                                      const int expected_position_calls,
                                      const std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) const {
            std::unique_lock<std::mutex> lock(mutex_);
            return cv_.wait_for(lock, timeout, [&] {
                return account_calls_.load() >= expected_account_calls
                       && positions_calls_.load() >= expected_position_calls;
            });
        }

        bool wait_for_polled_exec_count(
            const int expected,
            const std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) const {
            std::unique_lock<std::mutex> lock(mutex_);
            return cv_.wait_for(lock, timeout, [&] { return polled_exec_count_ >= expected; });
        }

    private:
        std::atomic<bool> connected_{false};
        std::vector<std::string> subscribed_;
        std::function<void(const live::MarketDataUpdate&)> market_cb_;
        std::function<void(const live::ExecutionReport&)> exec_cb_;
        mutable std::mutex mutex_;
        mutable std::condition_variable cv_;
        int submit_count_ = 0;
        int cancel_count_ = 0;
        int polled_exec_count_ = 0;
        double last_price_ = 0.0;
        std::vector<live::ExecutionReport> pending_execs_;
        std::vector<live::ExecutionReport> open_orders_;
        live::AccountInfo account_info_{100000.0, 100000.0, 100000.0};
        std::vector<live::Position> positions_;
        std::atomic<int> account_calls_{0};
        std::atomic<int> positions_calls_{0};
        bool auto_fill_ = true;
        std::optional<engine::Order> last_submitted_order_;
    };

    class LiveBuyOnceStrategy final : public strategy::Strategy {
    public:
        void initialize(strategy::StrategyContext&) override {}

        void on_bar(const data::Bar& bar) override {
            if (sent_) {
                return;
            }
            engine::Order order = engine::Order::market(bar.symbol, engine::OrderSide::Buy, 1.0);
            sent_ = ctx_->submit_order(std::move(order)).is_ok();
        }

    private:
        bool sent_ = false;
    };

    class LiveTwoOrderStrategy final : public strategy::Strategy {
    public:
        void initialize(strategy::StrategyContext&) override {}

        void on_bar(const data::Bar& bar) override {
            if (sent_) {
                return;
            }
            engine::Order order1 = engine::Order::market(bar.symbol, engine::OrderSide::Buy, 1.0);
            engine::Order order2 = engine::Order::market(bar.symbol, engine::OrderSide::Buy, 1.0);
            ctx_->submit_order(std::move(order1));
            ctx_->submit_order(std::move(order2));
            sent_ = true;
        }

    private:
        bool sent_ = false;
    };

    class LiveNoopStrategy final : public strategy::Strategy {
    public:
        void initialize(strategy::StrategyContext&) override {}
    };

    static data::Bar make_bar(SymbolId symbol, double price) {
        data::Bar bar{};
        bar.symbol = symbol;
        bar.timestamp = fixed_timestamp(static_cast<int64_t>(symbol));
        bar.open = bar.high = bar.low = bar.close = price;
        bar.volume = 100;
        return bar;
    }

    TEST(LiveEngineIntegration, FeedToStrategyToOrderToAudit) {
        strategy::StrategyFactory::instance().register_creator(
            "buy_once", [](const Config&) { return std::make_unique<LiveBuyOnceStrategy>(); });
        {
            Config cfg_check;
            cfg_check.set("type", "buy_once");
            ASSERT_NE(strategy::StrategyFactory::instance().create(cfg_check), nullptr);
        }

        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "buy_once";
        cfg.strategy_config.set("type", "buy_once");
        cfg.symbols = {"LIVETEST"};
        cfg.enable_regime_updates = true;
        cfg.max_orders_per_minute = 10;
        cfg.max_order_value = 100000;
        cfg.log_dir = fresh_log_dir("regimeflow_audit_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        SymbolRegistry::instance().intern("DUMMY_LIVE");
        auto symbol = SymbolRegistry::instance().intern("LIVETEST");
        ASSERT_NE(symbol, 0u);
        broker_ptr->emit_bar(make_bar(symbol, 100.0));

        EXPECT_TRUE(broker_ptr->wait_for_submit_count(1));
        EXPECT_TRUE(broker_ptr->wait_for_polled_exec_count(1));

        engine->stop();

        std::filesystem::path log_path = std::filesystem::path(cfg.log_dir) / "audit.log";
        std::ifstream in(log_path);
        ASSERT_TRUE(in.good());
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("OrderSubmitted"), std::string::npos);
        EXPECT_NE(content.find("OrderFilled"), std::string::npos);
    }

    TEST(LiveEngineIntegration, CapturesMarketDataReplayJournal) {
        strategy::StrategyFactory::instance().register_creator(
            "replay_noop", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });

        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "replay_noop";
        cfg.strategy_config.set("type", "replay_noop");
        cfg.symbols = {"REPLAYLIVE"};
        cfg.log_dir = fresh_log_dir("regimeflow_live_replay_test");
        cfg.replay_journal_path = (std::filesystem::path(cfg.log_dir) / "live_replay.jsonl").string();

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        ASSERT_TRUE(engine->start().is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        const auto symbol = SymbolRegistry::instance().intern("REPLAYLIVE");
        broker_ptr->emit_bar(make_bar(symbol, 101.25));

        ASSERT_TRUE(wait_until([&] {
            auto events = engine::read_replay_journal(cfg.replay_journal_path);
            return events.is_ok() && !events.value().empty();
        }));
        engine->stop();

        auto events = engine::read_replay_journal(cfg.replay_journal_path);
        ASSERT_TRUE(events.is_ok()) << events.error().to_string();
        ASSERT_EQ(events.value().size(), 1U);
        const auto* payload = std::get_if<events::MarketEventPayload>(&events.value()[0].payload);
        ASSERT_NE(payload, nullptr);
        EXPECT_EQ(payload->kind, events::MarketEventKind::Bar);
    }

    TEST(LiveEngineIntegration, CapturesReplayJournalOrderDecisions) {
        strategy::StrategyFactory::instance().register_creator(
            "replay_buy_once", [](const Config&) { return std::make_unique<LiveBuyOnceStrategy>(); });

        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "replay_buy_once";
        cfg.strategy_config.set("type", "replay_buy_once");
        cfg.symbols = {"REPLAYORDER"};
        cfg.dry_run_orders = true;
        cfg.log_dir = fresh_log_dir("regimeflow_live_replay_order_test");
        cfg.replay_journal_path = (std::filesystem::path(cfg.log_dir) / "live_replay.jsonl").string();

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        ASSERT_TRUE(engine->start().is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        const auto symbol = SymbolRegistry::instance().intern("REPLAYORDER");
        broker_ptr->emit_bar(make_bar(symbol, 101.25));

        ASSERT_TRUE(wait_until([&] {
            auto events = engine::read_replay_journal(cfg.replay_journal_path);
            return events.is_ok() && events.value().size() >= 2;
        }));
        engine->stop();

        auto events = engine::read_replay_journal(cfg.replay_journal_path);
        ASSERT_TRUE(events.is_ok()) << events.error().to_string();
        const auto found_risk_or_dry_run = std::ranges::any_of(events.value(), [](const events::Event& event) {
            const auto* payload = std::get_if<events::SystemEventPayload>(&event.payload);
            return payload != nullptr && payload->kind == events::SystemEventKind::TradingHalt
                   && payload->id.rfind("dry_run:", 0) == 0;
        });
        EXPECT_TRUE(found_risk_or_dry_run);
    }

    TEST(LiveEngineIntegration, DryRunOrdersAreAuditedWithoutBrokerSubmit) {
        strategy::StrategyFactory::instance().register_creator(
            "buy_once_dry_run", [](const Config&) { return std::make_unique<LiveBuyOnceStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "buy_once_dry_run";
        cfg.strategy_config.set("type", "buy_once_dry_run");
        cfg.symbols = {"DRYRUN"};
        cfg.max_order_value = 100000;
        cfg.dry_run_orders = true;
        cfg.log_dir = fresh_log_dir("regimeflow_dry_run_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        ASSERT_TRUE(engine->start().is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        const auto symbol = SymbolRegistry::instance().intern("DRYRUN");
        broker_ptr->emit_bar(make_bar(symbol, 100.0));

        const auto log_path = std::filesystem::path(cfg.log_dir) / "audit.log";
        ASSERT_TRUE(wait_until([&] {
            std::ifstream in(log_path);
            if (!in.good()) {
                return false;
            }
            const std::string content((std::istreambuf_iterator<char>(in)),
                                      std::istreambuf_iterator<char>());
            return content.find("DryRunOrder") != std::string::npos
                   && content.find("Dry-run order suppressed") != std::string::npos;
        }));
        EXPECT_EQ(broker_ptr->submit_count(), 0);

        engine->stop();
    }

    TEST(LiveEngineIntegration, RateLimitRejectsSecondOrder) {
        strategy::StrategyFactory::instance().register_creator(
            "two_order", [](const Config&) { return std::make_unique<LiveTwoOrderStrategy>(); });
        {
            Config cfg_check;
            cfg_check.set("type", "two_order");
            ASSERT_NE(strategy::StrategyFactory::instance().create(cfg_check), nullptr);
        }

        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "two_order";
        cfg.strategy_config.set("type", "two_order");
        cfg.symbols = {"LIMIT"};
        cfg.max_orders_per_minute = 1;
        cfg.max_orders_per_second = 1;
        cfg.max_order_value = 100000;
        cfg.log_dir = fresh_log_dir("regimeflow_rate_limit_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        std::mutex error_mutex;
        std::condition_variable error_cv;
        int errors = 0;
        engine->on_error([&](const std::string&) {
            {
                std::lock_guard<std::mutex> lock(error_mutex);
                ++errors;
            }
            error_cv.notify_all();
        });
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        SymbolRegistry::instance().intern("DUMMY_LIVE2");
        auto symbol = SymbolRegistry::instance().intern("LIMIT");
        ASSERT_NE(symbol, 0u);
        broker_ptr->emit_bar(make_bar(symbol, 50.0));

        EXPECT_TRUE(broker_ptr->wait_for_submit_count(1));
        {
            std::unique_lock<std::mutex> lock(error_mutex);
            EXPECT_TRUE(error_cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return errors >= 1; }));
        }
        EXPECT_EQ(broker_ptr->submit_count(), 1);

        engine->stop();
    }

    TEST(LiveEngineIntegration, ReconciliationRefreshesAccountAndPositions) {
        strategy::StrategyFactory::instance().register_creator(
            "noop", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "noop";
        cfg.strategy_config.set("type", "noop");
        cfg.symbols = {"RECON"};
        cfg.order_reconcile_interval = Duration::milliseconds(20);
        cfg.position_reconcile_interval = Duration::milliseconds(20);
        cfg.account_refresh_interval = Duration::milliseconds(20);
        cfg.max_order_value = 100000;
        cfg.log_dir = fresh_log_dir("regimeflow_reconcile_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());

        EXPECT_TRUE(broker_ptr->wait_for_reconcile_calls(2, 2));
        EXPECT_GE(broker_ptr->account_calls(), 2);
        EXPECT_GE(broker_ptr->positions_calls(), 2);

        engine->stop();
    }

    TEST(LiveEngineIntegration, DailyLossLimitDisablesTrading) {
        strategy::StrategyFactory::instance().register_creator(
            "noop_loss", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "noop_loss";
        cfg.strategy_config.set("type", "noop_loss");
        cfg.symbols = {"LOSS"};
        cfg.account_refresh_interval = Duration::milliseconds(20);
        cfg.daily_loss_limit = 1000.0;
        cfg.log_dir = fresh_log_dir("regimeflow_loss_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());

        live::AccountInfo info;
        info.equity = 98000.0;
        info.cash = 98000.0;
        info.buying_power = 98000.0;
        broker_ptr->set_account_info(info);

        EXPECT_TRUE(wait_until([&] { return !engine->get_status().trading_enabled; }));

        EXPECT_FALSE(engine->get_status().trading_enabled);
        engine->stop();
    }

    TEST(LiveEngineIntegration, PositionRiskLimitDisablesTrading) {
        strategy::StrategyFactory::instance().register_creator(
            "noop_risk", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "noop_risk";
        cfg.strategy_config.set("type", "noop_risk");
        cfg.position_reconcile_interval = Duration::milliseconds(20);
        cfg.risk_config.set_path("limits.max_gross_exposure", 5000.0);
        cfg.log_dir = fresh_log_dir("regimeflow_risk_limit_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());

        live::Position pos;
        pos.symbol = "RISKY";
        pos.quantity = 100;
        pos.market_value = 100000.0;
        broker_ptr->set_positions({pos});

        EXPECT_TRUE(wait_until([&] { return !engine->get_status().trading_enabled; }));

        EXPECT_FALSE(engine->get_status().trading_enabled);
        engine->stop();
    }

    TEST(LiveEngineIntegration, BinanceAssetPositionsResolveToConfiguredTradingSymbol) {
        strategy::StrategyFactory::instance().register_creator(
            "noop_binance_positions", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "binance";
        cfg.strategy_name = "noop_binance_positions";
        cfg.strategy_config.set("type", "noop_binance_positions");
        cfg.symbols = {"BTCUSDT"};
        cfg.position_reconcile_interval = Duration::milliseconds(20);
        cfg.account_refresh_interval = Duration::milliseconds(20);
        cfg.log_dir = fresh_log_dir("regimeflow_binance_position_map_test");

        live::AccountInfo info;
        info.equity = 10000.0;
        info.cash = 10000.0;
        info.buying_power = 10000.0;
        broker_ptr->set_account_info(info);

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());

        live::Position pos;
        pos.symbol = "BTC";
        pos.quantity = 1.5;
        pos.average_price = 42000.0;
        pos.market_value = 63000.0;
        broker_ptr->set_positions({pos});

        const bool resolved = wait_until([&] {
            const auto snapshot = engine->get_dashboard_snapshot();
            if (!snapshot.positions.empty()) {
                if (snapshot.positions.size() != 1u) {
                    return false;
                }
                EXPECT_EQ(SymbolRegistry::instance().lookup(snapshot.positions.front().symbol), "BTCUSDT");
                EXPECT_GE(snapshot.equity, 73000.0 - 1e-6);
                EXPECT_GE(engine->get_status().equity, 73000.0 - 1e-6);
                return true;
            }
            return false;
        });

        EXPECT_TRUE(resolved);
        engine->stop();
    }

    TEST(LiveEngineIntegration, DashboardCallbackReceivesSnapshot) {
        strategy::StrategyFactory::instance().register_creator(
            "noop_dashboard", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "noop_dashboard";
        cfg.strategy_config.set("type", "noop_dashboard");
        cfg.symbols = {"DASH"};
        cfg.max_order_value = 100000;
        cfg.log_dir = fresh_log_dir("regimeflow_dashboard_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        std::mutex update_mutex;
        std::condition_variable update_cv;
        int updates = 0;
        engine->on_dashboard_update([&](const live::LiveTradingEngine::DashboardSnapshot& snapshot) {
            EXPECT_GE(snapshot.equity, 0.0);
            EXPECT_GE(snapshot.cash, 0.0);
            {
                std::lock_guard<std::mutex> lock(update_mutex);
                ++updates;
            }
            update_cv.notify_all();
        });
        auto start_res = engine->start();
        ASSERT_TRUE(start_res.is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        auto symbol = SymbolRegistry::instance().intern("DASH");
        ASSERT_NE(symbol, 0u);
        broker_ptr->emit_bar(make_bar(symbol, 101.0));

        {
            std::unique_lock<std::mutex> lock(update_mutex);
            EXPECT_TRUE(update_cv.wait_for(lock, std::chrono::milliseconds(500), [&] { return updates > 0; }));
        }

        EXPECT_GT(updates, 0);
        const auto dashboard_json = engine->dashboard_snapshot_json();
        EXPECT_NE(dashboard_json.find("\"buying_power\""), std::string::npos);
        EXPECT_NE(dashboard_json.find("\"initial_margin\""), std::string::npos);
        engine->stop();
    }

    TEST(LiveEngineIntegration, HeartbeatTimeoutDisablesTradingWhenMarketDataIsStale) {
        strategy::StrategyFactory::instance().register_creator(
            "noop_stale_heartbeat", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "noop_stale_heartbeat";
        cfg.strategy_config.set("type", "noop_stale_heartbeat");
        cfg.symbols = {"STALE"};
        cfg.log_dir = fresh_log_dir("regimeflow_stale_heartbeat_test");
        cfg.heartbeat_timeout = Duration::milliseconds(10);
        cfg.enable_auto_reconnect = false;

        std::vector<std::string> errors;
        std::mutex errors_mutex;
        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        engine->on_error([&](const std::string& error) {
            std::lock_guard<std::mutex> lock(errors_mutex);
            errors.push_back(error);
        });

        ASSERT_TRUE(engine->start().is_ok());
        ASSERT_TRUE(wait_until([&] { return !engine->get_status().trading_enabled; },
                               std::chrono::milliseconds(500)));
        ASSERT_TRUE(wait_until([&] {
            std::lock_guard<std::mutex> lock(errors_mutex);
            return std::ranges::find(errors, "Heartbeat timeout: no market data") != errors.end()
                   && std::ranges::find(errors, "Trading disabled because market data is stale") != errors.end();
        }, std::chrono::milliseconds(500)));

        engine->stop();
    }

    TEST(LiveEngineIntegration, CloseAllPositionsSubmitsThroughOrderManager) {
        strategy::StrategyFactory::instance().register_creator(
            "noop_close_positions", [](const Config&) { return std::make_unique<LiveNoopStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::AccountInfo info;
        info.equity = 100000.0;
        info.cash = 100000.0;
        info.buying_power = 100000.0;
        broker_ptr->set_account_info(info);

        live::Position position;
        position.symbol = "CLOSE";
        position.quantity = 5.0;
        position.average_price = 100.0;
        position.market_value = 500.0;
        broker_ptr->set_positions({position});

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "noop_close_positions";
        cfg.strategy_config.set("type", "noop_close_positions");
        cfg.symbols = {"CLOSE"};
        cfg.max_order_value = 0.0;
        cfg.log_dir = fresh_log_dir("regimeflow_close_positions_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        ASSERT_TRUE(engine->start().is_ok());

        engine->close_all_positions();
        ASSERT_TRUE(broker_ptr->wait_for_submit_count(1));

        const auto submitted = broker_ptr->last_submitted_order();
        ASSERT_TRUE(submitted.has_value());
        EXPECT_EQ(submitted->side, engine::OrderSide::Sell);
        EXPECT_DOUBLE_EQ(submitted->quantity, 5.0);
        EXPECT_EQ(submitted->metadata.at("risk_exit"), "close_all_positions");

        engine->stop();
    }

    TEST(LiveEngineIntegration, NormalizesBrokerSpecificDefaultsBeforeSubmit) {
        strategy::StrategyFactory::instance().register_creator(
            "buy_once_normalized", [](const Config&) { return std::make_unique<LiveBuyOnceStrategy>(); });
        auto broker = std::make_unique<MockBrokerAdapter>();
        auto* broker_ptr = broker.get();

        live::LiveConfig cfg;
        cfg.broker_type = "binance";
        cfg.strategy_name = "buy_once_normalized";
        cfg.strategy_config.set("type", "buy_once_normalized");
        cfg.symbols = {"BTCUSDT"};
        cfg.max_order_value = 1000000.0;
        cfg.log_dir = fresh_log_dir("regimeflow_normalization_test");

        auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker));
        ASSERT_TRUE(engine->start().is_ok());
        ASSERT_TRUE(broker_ptr->wait_for_callbacks());

        const auto lower_symbol = SymbolRegistry::instance().intern("btcusdt");
        broker_ptr->emit_bar(make_bar(lower_symbol, 42000.0));

        ASSERT_TRUE(broker_ptr->wait_for_submit_count(1));

        const auto submitted = broker_ptr->last_submitted_order();
        ASSERT_TRUE(submitted.has_value());
        EXPECT_EQ(SymbolRegistry::instance().lookup(submitted->symbol), "BTCUSDT");
        EXPECT_EQ(submitted->tif, engine::TimeInForce::IOC);
        EXPECT_EQ(submitted->metadata.at("normalized_symbol"), "BTCUSDT");
        EXPECT_EQ(submitted->metadata.at("normalized_tif"), "IOC");

        engine->stop();
    }

    TEST(LiveEngineIntegration, RestoresReconciliationJournalMappingsAcrossRestart) {
        strategy::StrategyFactory::instance().register_creator(
            "buy_once_restore", [](const Config&) { return std::make_unique<LiveBuyOnceStrategy>(); });
        const auto log_dir = fresh_log_dir("regimeflow_reconciliation_restore_test");
        auto broker1 = std::make_unique<MockBrokerAdapter>();
        auto* broker1_ptr = broker1.get();
        broker1_ptr->set_auto_fill(false);

        live::LiveConfig cfg;
        cfg.broker_type = "mock";
        cfg.strategy_name = "buy_once_restore";
        cfg.strategy_config.set("type", "buy_once_restore");
        cfg.symbols = {"RESTORE"};
        cfg.log_dir = log_dir;
        cfg.order_reconcile_interval = Duration::milliseconds(20);

        {
            auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker1));
            ASSERT_TRUE(engine->start().is_ok());
            ASSERT_TRUE(broker1_ptr->wait_for_callbacks());
            const auto symbol = SymbolRegistry::instance().intern("RESTORE");
            broker1_ptr->emit_bar(make_bar(symbol, 100.0));

            ASSERT_TRUE(broker1_ptr->wait_for_submit_count(1));
            ASSERT_EQ(broker1_ptr->submit_count(), 1);
            engine->stop();
        }

        auto broker2 = std::make_unique<MockBrokerAdapter>();
        auto* broker2_ptr = broker2.get();
        broker2_ptr->set_auto_fill(false);
        live::ExecutionReport open_report;
        open_report.broker_order_id = "B1";
        open_report.symbol = "RESTORE";
        open_report.side = engine::OrderSide::Buy;
        open_report.quantity = 1.0;
        open_report.price = 100.0;
        open_report.status = live::LiveOrderStatus::New;
        open_report.timestamp = fixed_timestamp(10'000);
        broker2_ptr->set_open_orders({open_report});
        broker2_ptr->set_account_info(live::AccountInfo{100000.0, 100000.0, 100000.0});
        broker2_ptr->set_positions({});

        {
            broker2_ptr->set_auto_fill(false);
            auto engine = std::make_unique<live::LiveTradingEngine>(cfg, std::move(broker2));
            ASSERT_TRUE(engine->start().is_ok());

            live::ExecutionReport fill = open_report;
            fill.status = live::LiveOrderStatus::Filled;
            fill.timestamp = fixed_timestamp(20'000);
            broker2_ptr->emit_execution_report(fill);
            engine->stop();
        }

        const auto journal_path = std::filesystem::path(log_dir) / "reconciliation_journal.tsv";
        std::ifstream in(journal_path);
        ASSERT_TRUE(in.good());
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("\t1\tB1\tRESTORE\tPendingNew"), std::string::npos);
        EXPECT_NE(content.find("\t1\tB1\tRESTORE\tFilled"), std::string::npos);
    }
}  // namespace regimeflow::test
