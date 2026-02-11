#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/common/result.h"
#include "regimeflow/common/mpsc_queue.h"
#include "regimeflow/common/spsc_queue.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/live/audit_log.h"
#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/live/event_bus.h"
#include "regimeflow/live/live_order_manager.h"
#include "regimeflow/live/mq_adapter.h"
#include "regimeflow/live/types.h"
#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/regime/features.h"
#include "regimeflow/risk/risk_limits.h"
#include "regimeflow/strategy/strategy.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace regimeflow::live {

struct LiveConfig {
    std::string broker_type;
    std::map<std::string, std::string> broker_config;

    std::vector<std::string> symbols;

    std::string strategy_name;
    Config strategy_config;

    Config risk_config;

    std::string regime_model_path;
    bool enable_regime_updates = true;

    bool paper_trading = true;
    int max_orders_per_minute = 60;
    double max_order_value = 100000;
    int max_orders_per_second = 0;  // 0 = use broker limit
    Duration order_reconcile_interval = Duration::seconds(30);
    Duration position_reconcile_interval = Duration::seconds(60);
    Duration account_refresh_interval = Duration::seconds(30);
    double daily_loss_limit = 0.0;
    double daily_loss_limit_pct = 0.0;
    Duration heartbeat_timeout = Duration::seconds(30);
    bool enable_auto_reconnect = true;
    Duration reconnect_initial = Duration::seconds(1);
    Duration reconnect_max = Duration::seconds(30);
    int reconnect_max_attempts = 0;

    bool enable_message_queue = false;
    MessageQueueConfig message_queue;

    Duration regime_retrain_interval = Duration::hours(24);
    size_t regime_retrain_min_samples = 200;
    int regime_feature_window = 50;

    std::string log_dir = "./logs";
};

class LiveTradingEngine {
public:
    explicit LiveTradingEngine(const LiveConfig& config);
    LiveTradingEngine(const LiveConfig& config, std::unique_ptr<BrokerAdapter> broker);
    ~LiveTradingEngine();

    Result<void> start();
    void stop();
    bool is_running() const;

    struct EngineStatus {
        bool connected = false;
        bool trading_enabled = false;
        regime::RegimeState current_regime;
        int open_orders = 0;
        double equity = 0.0;
        double daily_pnl = 0.0;
        Timestamp last_update;
    };

    struct LiveOrderSummary {
        engine::OrderId id = 0;
        std::string symbol;
        engine::OrderSide side = engine::OrderSide::Buy;
        engine::OrderType type = engine::OrderType::Market;
        double quantity = 0.0;
        double filled_quantity = 0.0;
        double limit_price = 0.0;
        double stop_price = 0.0;
        double avg_fill_price = 0.0;
        std::string status;
        Timestamp updated_at;
    };

    struct DashboardSnapshot {
        Timestamp timestamp;
        double equity = 0.0;
        double cash = 0.0;
        double daily_pnl = 0.0;
        regime::RegimeState current_regime;
        std::vector<engine::PortfolioSnapshot> equity_curve;
        std::vector<engine::Position> positions;
        std::vector<LiveOrderSummary> open_orders;
        std::vector<std::string> alerts;
        double cpu_usage_pct = 0.0;
        double memory_mb = 0.0;
        double event_loop_latency_ms = 0.0;
    };

    struct SystemHealth {
        double cpu_usage_pct = 0.0;
        double memory_mb = 0.0;
        double event_loop_latency_ms = 0.0;
        Timestamp last_sample;
    };

    EngineStatus get_status() const;
    DashboardSnapshot get_dashboard_snapshot() const;
    SystemHealth get_system_health() const;

    void enable_trading();
    void disable_trading();
    void close_all_positions();

    void on_trade(std::function<void(const Trade&)> cb);
    void on_regime_change(std::function<void(const regime::RegimeTransition&)> cb);
    void on_error(std::function<void(const std::string&)> cb);
    void on_dashboard_update(std::function<void(const DashboardSnapshot&)> cb);

private:
    void event_loop();
    void regime_update_loop();
    void handle_market_data(const MarketDataUpdate& update);
    void handle_execution_report(const ExecutionReport& report);
    void refresh_account_info();
    void refresh_positions();
    void reconcile_orders();
    void apply_positions(const std::vector<Position>& positions, Timestamp timestamp);
    void apply_position_update(const Position& position, Timestamp timestamp);
    void check_daily_loss_limit();
    void enforce_portfolio_limits(const std::string& context);
    void update_dashboard_snapshot(const std::string& context);
    DashboardSnapshot build_dashboard_snapshot(const std::string& context);
    void add_alert(const std::string& message);
    void drain_pending_alerts();
    void sample_system_health();
    void check_heartbeat();
    void attempt_reconnect();

    LiveConfig config_;

    std::unique_ptr<BrokerAdapter> broker_;
    std::unique_ptr<strategy::Strategy> strategy_;
    std::unique_ptr<strategy::StrategyContext> strategy_ctx_;
    std::unique_ptr<risk::RiskManager> risk_manager_;
    std::unique_ptr<regime::RegimeDetector> regime_detector_;
    std::unique_ptr<regime::FeatureExtractor> feature_extractor_;
    std::unique_ptr<LiveOrderManager> order_manager_;
    std::unique_ptr<MessageQueueAdapter> mq_adapter_;
    std::unique_ptr<AuditLogger> audit_logger_;
    engine::OrderManager strategy_order_manager_;

    std::atomic<bool> running_{false};
    std::atomic<bool> trading_enabled_{false};
    regime::RegimeState current_regime_;
    std::unique_ptr<engine::Portfolio> portfolio_;

    std::thread event_loop_thread_;
    std::thread regime_thread_;

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    common::SpscQueue<MarketDataUpdate, 8192> market_queue_;

    EventBus event_bus_;
    EventBus::SubscriptionId market_sub_id_ = 0;
    EventBus::SubscriptionId mq_forward_sub_id_ = 0;
    std::vector<EventBus::SubscriptionId> mq_forward_extra_ids_;

    std::mutex broker_mutex_;
    std::unordered_map<engine::OrderId, std::string> broker_order_ids_;
    std::unordered_map<std::string, engine::OrderId> broker_to_order_ids_;
    std::unordered_map<SymbolId, Price> last_prices_;

    std::mutex rate_mutex_;
    std::deque<Timestamp> order_timestamps_;
    std::deque<Timestamp> second_order_timestamps_;

    std::function<void(const Trade&)> trade_cb_;
    std::function<void(const regime::RegimeTransition&)> regime_cb_;
    std::function<void(const std::string&)> error_cb_;
    std::function<void(const DashboardSnapshot&)> dashboard_cb_;

    mutable std::mutex dashboard_mutex_;
    DashboardSnapshot last_dashboard_;

    std::mutex feature_mutex_;
    std::deque<regime::FeatureVector> feature_buffer_;
    Timestamp last_retrain_;
    Timestamp last_order_reconcile_;
    Timestamp last_position_refresh_;
    Timestamp last_account_refresh_;
    AccountInfo last_account_info_;
    double daily_start_equity_ = 0.0;
    double daily_pnl_ = 0.0;
    Timestamp last_market_data_;
    bool heartbeat_alerted_ = false;
    int reconnect_attempts_ = 0;
    Timestamp next_reconnect_attempt_;
    int64_t reconnect_backoff_ms_ = 0;

    mutable std::mutex alert_mutex_;
    std::vector<std::string> alerts_;
    common::MpscQueue<std::string> pending_alerts_;

    mutable std::mutex health_mutex_;
    SystemHealth last_health_{};
    uint64_t prev_cpu_total_ = 0;
    uint64_t prev_cpu_idle_ = 0;
    Timestamp last_health_sample_;
    Timestamp last_event_loop_tick_;
};

}  // namespace regimeflow::live
