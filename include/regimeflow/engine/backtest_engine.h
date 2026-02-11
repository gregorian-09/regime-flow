#pragma once

#include "regimeflow/engine/event_loop.h"
#include "regimeflow/engine/event_generator.h"
#include "regimeflow/engine/audit_log.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/portfolio.h"
#include "regimeflow/engine/market_data_cache.h"
#include "regimeflow/engine/order_book_cache.h"
#include "regimeflow/engine/execution_pipeline.h"
#include "regimeflow/engine/regime_tracker.h"
#include "regimeflow/engine/backtest_results.h"
#include "regimeflow/engine/timer_service.h"
#include "regimeflow/execution/execution_factory.h"
#include "regimeflow/events/dispatcher.h"
#include "regimeflow/events/event_queue.h"
#include "regimeflow/metrics/metrics_tracker.h"
#include "regimeflow/plugins/hooks.h"
#include "regimeflow/risk/risk_limits.h"
#include "regimeflow/risk/risk_factory.h"
#include "regimeflow/risk/stop_loss.h"
#include "regimeflow/strategy/strategy.h"
#include "regimeflow/strategy/strategy_manager.h"
#include "regimeflow/regime/regime_factory.h"

#include <string>
#include <optional>
#include <map>

namespace regimeflow::engine {

struct ParallelContext {
    Config data_config;
    TimeRange range;
    std::vector<std::string> symbols;
    data::BarType bar_type = data::BarType::Time_1Day;
};

class BacktestEngine {
public:
    explicit BacktestEngine(double initial_capital = 0.0, std::string currency = "USD");

    events::EventQueue& event_queue() { return event_queue_; }
    events::EventDispatcher& dispatcher() { return dispatcher_; }
    EventLoop& event_loop() { return event_loop_; }
    OrderManager& order_manager() { return order_manager_; }
    Portfolio& portfolio() { return portfolio_; }
    MarketDataCache& market_data() { return market_data_; }

    void enqueue(events::Event event);
    void load_data(std::unique_ptr<data::DataIterator> iterator);
    void load_data(std::unique_ptr<data::DataIterator> bar_iterator,
                   std::unique_ptr<data::TickIterator> tick_iterator,
                   std::unique_ptr<data::OrderBookIterator> book_iterator);
    void set_strategy(std::unique_ptr<strategy::Strategy> strategy, Config config = {});
    void add_strategy(std::unique_ptr<strategy::Strategy> strategy);
    void set_execution_model(std::unique_ptr<execution::ExecutionModel> model);
    void set_commission_model(std::unique_ptr<execution::CommissionModel> model);
    void set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model);
    void set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model);
    void set_latency_model(std::unique_ptr<execution::LatencyModel> model);
    void set_regime_detector(std::unique_ptr<regime::RegimeDetector> detector);
    risk::RiskManager& risk_manager() { return risk_manager_; }
    metrics::MetricsTracker& metrics() { return metrics_; }
    const regime::RegimeState& current_regime() const { return regime_tracker_.current_state(); }
    Timestamp current_time() const { return event_loop_.current_time(); }
    plugins::HookSystem& hooks() { return hooks_; }
    plugins::HookManager& hook_manager() { return hook_manager_; }
    void configure_execution(const Config& config);
    void configure_risk(const Config& config);
    void configure_regime(const Config& config);
    void set_parallel_context(ParallelContext context);
    std::vector<BacktestResults> run_parallel(
        const std::vector<std::map<std::string, double>>& param_sets,
        std::function<std::unique_ptr<strategy::Strategy>(
            const std::map<std::string, double>&)> strategy_factory,
        int num_threads = -1);
    void register_hook(plugins::HookType type,
                       std::function<plugins::HookResult(plugins::HookContext&)> hook,
                       int priority = 100);
    void on_progress(std::function<void(double pct, const std::string& msg)> callback);
    void set_audit_log_path(std::string path);
    BacktestResults results() const;
    bool step();
    void run_until(Timestamp end_time);
    void run();
    void stop();

private:
    void install_default_handlers();

    events::EventQueue event_queue_;
    events::EventDispatcher dispatcher_;
    EventLoop event_loop_;
    OrderManager order_manager_;
    Portfolio portfolio_;
    MarketDataCache market_data_;
    OrderBookCache order_book_cache_;
    TimerService timer_service_;
    ExecutionPipeline execution_pipeline_;
    RegimeTracker regime_tracker_{nullptr};
    std::unique_ptr<EventGenerator> event_generator_;
    std::unique_ptr<strategy::Strategy> strategy_;
    strategy::StrategyManager strategy_manager_;
    std::unique_ptr<strategy::StrategyContext> strategy_context_;
    risk::RiskManager risk_manager_;
    risk::StopLossManager stop_loss_manager_;
    metrics::MetricsTracker metrics_;
    plugins::HookSystem hooks_;
    plugins::HookManager hook_manager_;
    std::function<void(double, const std::string&)> progress_callback_;
    size_t progress_total_estimate_ = 0;
    bool started_ = false;
    std::optional<Config> execution_config_;
    std::optional<Config> risk_config_;
    std::optional<Config> regime_config_;
    std::optional<ParallelContext> parallel_context_;
    std::unique_ptr<AuditLogger> audit_logger_;
};

}  // namespace regimeflow::engine
