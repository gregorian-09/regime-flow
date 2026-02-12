/**
 * @file live_engine.h
 * @brief RegimeFlow regimeflow live engine declarations.
 */

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

/**
 * @brief Configuration for live trading engine.
 */
struct LiveConfig {
    /**
     * @brief Broker adapter type name.
     */
    std::string broker_type;
    /**
     * @brief Broker-specific configuration key/value pairs.
     */
    std::map<std::string, std::string> broker_config;

    /**
     * @brief Symbols to trade.
     */
    std::vector<std::string> symbols;

    /**
     * @brief Strategy name or registry key.
     */
    std::string strategy_name;
    /**
     * @brief Strategy configuration.
     */
    Config strategy_config;

    /**
     * @brief Risk manager configuration.
     */
    Config risk_config;

    /**
     * @brief Path to persisted regime model.
     */
    std::string regime_model_path;
    /**
     * @brief Enable live regime updates.
     */
    bool enable_regime_updates = true;

    /**
     * @brief Paper trading mode toggle.
     */
    bool paper_trading = true;
    /**
     * @brief Max orders per minute.
     */
    int max_orders_per_minute = 60;
    /**
     * @brief Maximum notional value per order.
     */
    double max_order_value = 100000;
    /**
     * @brief Max orders per second (0 = broker limit).
     */
    int max_orders_per_second = 0;  // 0 = use broker limit
    /**
     * @brief Interval for order reconciliation.
     */
    Duration order_reconcile_interval = Duration::seconds(30);
    /**
     * @brief Interval for position reconciliation.
     */
    Duration position_reconcile_interval = Duration::seconds(60);
    /**
     * @brief Interval for account refresh.
     */
    Duration account_refresh_interval = Duration::seconds(30);
    /**
     * @brief Absolute daily loss limit.
     */
    double daily_loss_limit = 0.0;
    /**
     * @brief Daily loss limit as a fraction of equity.
     */
    double daily_loss_limit_pct = 0.0;
    /**
     * @brief Heartbeat timeout for live feed.
     */
    Duration heartbeat_timeout = Duration::seconds(30);
    /**
     * @brief Enable automatic broker reconnects.
     */
    bool enable_auto_reconnect = true;
    /**
     * @brief Initial reconnect backoff.
     */
    Duration reconnect_initial = Duration::seconds(1);
    /**
     * @brief Maximum reconnect backoff.
     */
    Duration reconnect_max = Duration::seconds(30);
    /**
     * @brief Maximum reconnect attempts (0 = unlimited).
     */
    int reconnect_max_attempts = 0;

    /**
     * @brief Enable message queue integration.
     */
    bool enable_message_queue = false;
    /**
     * @brief Message queue configuration.
     */
    MessageQueueConfig message_queue;

    /**
     * @brief Interval for regime model retraining.
     */
    Duration regime_retrain_interval = Duration::hours(24);
    /**
     * @brief Minimum samples before retraining.
     */
    size_t regime_retrain_min_samples = 200;
    /**
     * @brief Feature window size for regime features.
     */
    int regime_feature_window = 50;

    /**
     * @brief Log output directory.
     */
    std::string log_dir = "./logs";
};

/**
 * @brief Live trading engine orchestrating broker, strategy, and risk.
 */
class LiveTradingEngine {
public:
    /**
     * @brief Construct with live configuration.
     * @param config Live configuration.
     */
    explicit LiveTradingEngine(const LiveConfig& config);
    /**
     * @brief Construct with live configuration and injected broker.
     * @param config Live configuration.
     * @param broker Broker adapter instance.
     */
    LiveTradingEngine(const LiveConfig& config, std::unique_ptr<BrokerAdapter> broker);
    /**
     * @brief Stop engine and join threads.
     */
    ~LiveTradingEngine();

    /**
     * @brief Start the engine.
     * @return Ok on success, error otherwise.
     */
    Result<void> start();
    /**
     * @brief Stop the engine.
     */
    void stop();
    /**
     * @brief Check if engine is running.
     */
    bool is_running() const;

    /**
     * @brief Live engine status snapshot.
     */
    struct EngineStatus {
        bool connected = false;
        bool trading_enabled = false;
        regime::RegimeState current_regime;
        int open_orders = 0;
        double equity = 0.0;
        double daily_pnl = 0.0;
        Timestamp last_update;
    };

    /**
     * @brief Summary of an open live order for dashboards.
     */
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

    /**
     * @brief Dashboard snapshot for UI/monitoring.
     */
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

    /**
     * @brief System health telemetry snapshot.
     */
    struct SystemHealth {
        double cpu_usage_pct = 0.0;
        double memory_mb = 0.0;
        double event_loop_latency_ms = 0.0;
        Timestamp last_sample;
        Timestamp last_market_data;
        Timestamp last_reconnect_attempt;
        Timestamp last_reconnect_success;
    };

    /**
     * @brief Get the current engine status.
     */
    EngineStatus get_status() const;
    /**
     * @brief Get the latest dashboard snapshot.
     */
    DashboardSnapshot get_dashboard_snapshot() const;
    /**
     * @brief Get the latest system health snapshot.
     */
    SystemHealth get_system_health() const;

    /**
     * @brief Enable live trading.
     */
    void enable_trading();
    /**
     * @brief Disable live trading.
     */
    void disable_trading();
    /**
     * @brief Close all open positions.
     */
    void close_all_positions();

    /**
     * @brief Register trade callback.
     */
    void on_trade(std::function<void(const Trade&)> cb);
    /**
     * @brief Register regime change callback.
     */
    void on_regime_change(std::function<void(const regime::RegimeTransition&)> cb);
    /**
     * @brief Register error callback.
     */
    void on_error(std::function<void(const std::string&)> cb);
    /**
     * @brief Register dashboard update callback.
     */
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
    Timestamp last_reconnect_attempt_;
    Timestamp last_reconnect_success_;

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
