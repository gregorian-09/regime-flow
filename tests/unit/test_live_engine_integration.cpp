#include <gtest/gtest.h>

#include "regimeflow/live/live_engine.h"
#include "regimeflow/strategy/strategy_factory.h"
#include "regimeflow/strategy/strategy.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

namespace regimeflow::test {

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
        subscribed_ = symbols;
    }

    void unsubscribe_market_data(const std::vector<std::string>&) override {}

    Result<std::string> submit_order(const engine::Order& order) override {
        std::lock_guard<std::mutex> lock(mutex_);
        ++submit_count_;
        std::string broker_id = "B" + std::to_string(submit_count_);
        live::ExecutionReport report;
        report.broker_order_id = broker_id;
        report.symbol = SymbolRegistry::instance().lookup(order.symbol);
        report.side = order.side;
        report.quantity = std::abs(order.quantity);
        report.price = last_price_ > 0 ? last_price_ : 100.0;
        report.status = live::LiveOrderStatus::Filled;
        report.timestamp = Timestamp::now();
        pending_execs_.push_back(report);
        return broker_id;
    }

    Result<void> cancel_order(const std::string&) override { return Ok(); }
    Result<void> modify_order(const std::string&, const engine::OrderModification&) override {
        return Ok();
    }

    live::AccountInfo get_account_info() override {
        account_calls_.fetch_add(1);
        std::lock_guard<std::mutex> lock(mutex_);
        return account_info_;
    }

    std::vector<live::Position> get_positions() override {
        positions_calls_.fetch_add(1);
        std::lock_guard<std::mutex> lock(mutex_);
        return positions_;
    }
    std::vector<live::ExecutionReport> get_open_orders() override { return {}; }

    void on_market_data(std::function<void(const live::MarketDataUpdate&)> cb) override {
        market_cb_ = std::move(cb);
    }

    void on_execution_report(std::function<void(const live::ExecutionReport&)> cb) override {
        exec_cb_ = std::move(cb);
    }

    void on_position_update(std::function<void(const live::Position&)>) override {}

    int max_orders_per_second() const override { return 5; }
    int max_messages_per_second() const override { return 1000; }

    void poll() override {
        std::vector<live::ExecutionReport> pending;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending.swap(pending_execs_);
        }
        if (exec_cb_) {
            for (const auto& report : pending) {
                exec_cb_(report);
            }
        }
    }

    void emit_bar(const data::Bar& bar) {
        last_price_ = bar.close;
        if (market_cb_) {
            live::MarketDataUpdate update;
            update.data = bar;
            market_cb_(update);
        }
    }

    int submit_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return submit_count_;
    }

    void set_account_info(const live::AccountInfo& info) {
        std::lock_guard<std::mutex> lock(mutex_);
        account_info_ = info;
    }

    void set_positions(std::vector<live::Position> positions) {
        std::lock_guard<std::mutex> lock(mutex_);
        positions_ = std::move(positions);
    }

    int account_calls() const { return account_calls_.load(); }
    int positions_calls() const { return positions_calls_.load(); }

private:
    std::atomic<bool> connected_{false};
    std::vector<std::string> subscribed_;
    std::function<void(const live::MarketDataUpdate&)> market_cb_;
    std::function<void(const live::ExecutionReport&)> exec_cb_;
    mutable std::mutex mutex_;
    int submit_count_ = 0;
    double last_price_ = 0.0;
    std::vector<live::ExecutionReport> pending_execs_;
    live::AccountInfo account_info_{100000.0, 100000.0, 100000.0};
    std::vector<live::Position> positions_;
    std::atomic<int> account_calls_{0};
    std::atomic<int> positions_calls_{0};
};

class BuyOnceStrategy final : public strategy::Strategy {
public:
    void initialize(strategy::StrategyContext&) override {}

    void on_bar(const data::Bar& bar) override {
        if (sent_) {
            return;
        }
        engine::Order order = engine::Order::market(bar.symbol, engine::OrderSide::Buy, 1.0);
        ctx_->submit_order(std::move(order));
        sent_ = true;
    }

private:
    bool sent_ = false;
};

class TwoOrderStrategy final : public strategy::Strategy {
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

class NoopStrategy final : public strategy::Strategy {
public:
    void initialize(strategy::StrategyContext&) override {}
};

static data::Bar make_bar(SymbolId symbol, double price) {
    data::Bar bar{};
    bar.symbol = symbol;
    bar.timestamp = Timestamp::now();
    bar.open = bar.high = bar.low = bar.close = price;
    bar.volume = 100;
    return bar;
}

TEST(LiveEngineIntegration, FeedToStrategyToOrderToAudit) {
    strategy::StrategyFactory::instance().register_creator(
        "buy_once", [](const Config&) { return std::make_unique<BuyOnceStrategy>(); });
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
    cfg.log_dir = (std::filesystem::temp_directory_path() / "regimeflow_audit_test").string();

    live::LiveTradingEngine engine(cfg, std::move(broker));
    auto start_res = engine.start();
    ASSERT_TRUE(start_res.is_ok());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    SymbolRegistry::instance().intern("DUMMY_LIVE");
    auto symbol = SymbolRegistry::instance().intern("LIVETEST");
    ASSERT_NE(symbol, 0u);
    broker_ptr->emit_bar(make_bar(symbol, 100.0));

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
    while (std::chrono::steady_clock::now() < deadline && broker_ptr->submit_count() < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_GE(broker_ptr->submit_count(), 1);

    engine.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::filesystem::path log_path = std::filesystem::path(cfg.log_dir) / "audit.log";
    std::ifstream in(log_path);
    ASSERT_TRUE(in.good());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("OrderSubmitted"), std::string::npos);
    EXPECT_NE(content.find("OrderFilled"), std::string::npos);
}

TEST(LiveEngineIntegration, RateLimitRejectsSecondOrder) {
    strategy::StrategyFactory::instance().register_creator(
        "two_order", [](const Config&) { return std::make_unique<TwoOrderStrategy>(); });
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
    cfg.log_dir = (std::filesystem::temp_directory_path() / "regimeflow_rate_limit_test").string();

    live::LiveTradingEngine engine(cfg, std::move(broker));
    std::atomic<int> errors{0};
    engine.on_error([&](const std::string&) { errors.fetch_add(1); });
    auto start_res = engine.start();
    ASSERT_TRUE(start_res.is_ok());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    SymbolRegistry::instance().intern("DUMMY_LIVE2");
    auto symbol = SymbolRegistry::instance().intern("LIMIT");
    ASSERT_NE(symbol, 0u);
    broker_ptr->emit_bar(make_bar(symbol, 50.0));

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
    while (std::chrono::steady_clock::now() < deadline && broker_ptr->submit_count() < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_EQ(broker_ptr->submit_count(), 1);
    EXPECT_GE(errors.load(), 1);

    engine.stop();
}

TEST(LiveEngineIntegration, ReconciliationRefreshesAccountAndPositions) {
    strategy::StrategyFactory::instance().register_creator(
        "noop", [](const Config&) { return std::make_unique<NoopStrategy>(); });
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
    cfg.log_dir = (std::filesystem::temp_directory_path() / "regimeflow_reconcile_test").string();

    live::LiveTradingEngine engine(cfg, std::move(broker));
    auto start_res = engine.start();
    ASSERT_TRUE(start_res.is_ok());
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (std::chrono::steady_clock::now() < deadline) {
        if (broker_ptr->account_calls() >= 2 && broker_ptr->positions_calls() >= 2) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_GE(broker_ptr->account_calls(), 2);
    EXPECT_GE(broker_ptr->positions_calls(), 2);

    engine.stop();
}

TEST(LiveEngineIntegration, DailyLossLimitDisablesTrading) {
    strategy::StrategyFactory::instance().register_creator(
        "noop_loss", [](const Config&) { return std::make_unique<NoopStrategy>(); });
    auto broker = std::make_unique<MockBrokerAdapter>();
    auto* broker_ptr = broker.get();

    live::LiveConfig cfg;
    cfg.broker_type = "mock";
    cfg.strategy_name = "noop_loss";
    cfg.strategy_config.set("type", "noop_loss");
    cfg.symbols = {"LOSS"};
    cfg.account_refresh_interval = Duration::milliseconds(20);
    cfg.daily_loss_limit = 1000.0;
    cfg.log_dir = (std::filesystem::temp_directory_path() / "regimeflow_loss_test").string();

    live::LiveTradingEngine engine(cfg, std::move(broker));
    auto start_res = engine.start();
    ASSERT_TRUE(start_res.is_ok());

    live::AccountInfo info;
    info.equity = 98000.0;
    info.cash = 98000.0;
    info.buying_power = 98000.0;
    broker_ptr->set_account_info(info);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!engine.get_status().trading_enabled) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FALSE(engine.get_status().trading_enabled);
    engine.stop();
}

TEST(LiveEngineIntegration, PositionRiskLimitDisablesTrading) {
    strategy::StrategyFactory::instance().register_creator(
        "noop_risk", [](const Config&) { return std::make_unique<NoopStrategy>(); });
    auto broker = std::make_unique<MockBrokerAdapter>();
    auto* broker_ptr = broker.get();

    live::LiveConfig cfg;
    cfg.broker_type = "mock";
    cfg.strategy_name = "noop_risk";
    cfg.strategy_config.set("type", "noop_risk");
    cfg.position_reconcile_interval = Duration::milliseconds(20);
    cfg.risk_config.set_path("limits.max_gross_exposure", 5000.0);
    cfg.log_dir = (std::filesystem::temp_directory_path() / "regimeflow_risk_limit_test").string();

    live::LiveTradingEngine engine(cfg, std::move(broker));
    auto start_res = engine.start();
    ASSERT_TRUE(start_res.is_ok());

    live::Position pos;
    pos.symbol = "RISKY";
    pos.quantity = 100;
    pos.market_value = 100000.0;
    broker_ptr->set_positions({pos});

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!engine.get_status().trading_enabled) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_FALSE(engine.get_status().trading_enabled);
    engine.stop();
}

TEST(LiveEngineIntegration, DashboardCallbackReceivesSnapshot) {
    strategy::StrategyFactory::instance().register_creator(
        "noop_dashboard", [](const Config&) { return std::make_unique<NoopStrategy>(); });
    auto broker = std::make_unique<MockBrokerAdapter>();
    auto* broker_ptr = broker.get();

    live::LiveConfig cfg;
    cfg.broker_type = "mock";
    cfg.strategy_name = "noop_dashboard";
    cfg.strategy_config.set("type", "noop_dashboard");
    cfg.symbols = {"DASH"};
    cfg.max_order_value = 100000;
    cfg.log_dir = (std::filesystem::temp_directory_path() / "regimeflow_dashboard_test").string();

    live::LiveTradingEngine engine(cfg, std::move(broker));
    std::atomic<int> updates{0};
    engine.on_dashboard_update([&](const live::LiveTradingEngine::DashboardSnapshot& snapshot) {
        updates.fetch_add(1);
        EXPECT_GE(snapshot.equity, 0.0);
        EXPECT_GE(snapshot.cash, 0.0);
    });
    auto start_res = engine.start();
    ASSERT_TRUE(start_res.is_ok());
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    auto symbol = SymbolRegistry::instance().intern("DASH");
    ASSERT_NE(symbol, 0u);
    broker_ptr->emit_bar(make_bar(symbol, 101.0));

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    while (std::chrono::steady_clock::now() < deadline && updates.load() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_GT(updates.load(), 0);
    engine.stop();
}

}  // namespace regimeflow::test
