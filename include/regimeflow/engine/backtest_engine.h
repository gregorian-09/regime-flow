/**
 * @file backtest_engine.h
 * @brief RegimeFlow regimeflow backtest engine declarations.
 */

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

/**
 * @brief Context parameters used for parallel backtest execution.
 */
struct ParallelContext {
    /**
     * @brief Data source configuration for iterators.
     */
    Config data_config;
    /**
     * @brief Backtest time range.
     */
    TimeRange range;
    /**
     * @brief Symbols to load and trade.
     */
    std::vector<std::string> symbols;
    /**
     * @brief Bar type for aggregated data.
     */
    data::BarType bar_type = data::BarType::Time_1Day;
};

/**
 * @brief Backtest engine orchestrating data, strategy, risk, and execution.
 *
 * @details Wires event queue, data feeds, strategy execution, risk controls,
 * metrics, and audit logging into a deterministic event loop.
 */
class BacktestEngine {
public:
    /**
     * @brief Construct a backtest engine.
     * @param initial_capital Starting cash.
     * @param currency Base currency code.
     */
    explicit BacktestEngine(double initial_capital = 0.0, std::string currency = "USD");

    /**
     * @brief Access the internal event queue.
     */
    events::EventQueue& event_queue() { return event_queue_; }
    /**
     * @brief Access the event dispatcher.
     */
    events::EventDispatcher& dispatcher() { return dispatcher_; }
    /**
     * @brief Access the event loop.
     */
    EventLoop& event_loop() { return event_loop_; }
    /**
     * @brief Access the order manager.
     */
    OrderManager& order_manager() { return order_manager_; }
    /**
     * @brief Access the portfolio.
     */
    Portfolio& portfolio() { return portfolio_; }
    /**
     * @brief Access the market data cache.
     */
    MarketDataCache& market_data() { return market_data_; }

    /**
     * @brief Enqueue a raw event into the engine.
     * @param event Event to enqueue.
     */
    void enqueue(events::Event event);
    /**
     * @brief Load a single data iterator (bars or ticks).
     * @param iterator Data iterator.
     */
    void load_data(std::unique_ptr<data::DataIterator> iterator);
    /**
     * @brief Load bar, tick, and order book iterators.
     * @param bar_iterator Bar iterator.
     * @param tick_iterator Tick iterator.
     * @param book_iterator Order book iterator.
     */
    void load_data(std::unique_ptr<data::DataIterator> bar_iterator,
                   std::unique_ptr<data::TickIterator> tick_iterator,
                   std::unique_ptr<data::OrderBookIterator> book_iterator);
    /**
     * @brief Set the primary strategy and optional config.
     * @param strategy Strategy instance.
     * @param config Strategy config.
     */
    void set_strategy(std::unique_ptr<strategy::Strategy> strategy, Config config = {});
    /**
     * @brief Add an additional strategy to the manager.
     * @param strategy Strategy instance.
     */
    void add_strategy(std::unique_ptr<strategy::Strategy> strategy);
    /**
     * @brief Set the execution model.
     * @param model Execution model instance.
     */
    void set_execution_model(std::unique_ptr<execution::ExecutionModel> model);
    /**
     * @brief Set the commission model.
     * @param model Commission model instance.
     */
    void set_commission_model(std::unique_ptr<execution::CommissionModel> model);
    /**
     * @brief Set the transaction cost model.
     * @param model Transaction cost model instance.
     */
    void set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model);
    /**
     * @brief Set the market impact model.
     * @param model Market impact model instance.
     */
    void set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model);
    /**
     * @brief Set the latency model.
     * @param model Latency model instance.
     */
    void set_latency_model(std::unique_ptr<execution::LatencyModel> model);
    /**
     * @brief Set the regime detector implementation.
     * @param detector Regime detector instance.
     */
    void set_regime_detector(std::unique_ptr<regime::RegimeDetector> detector);
    /**
     * @brief Access the risk manager.
     */
    risk::RiskManager& risk_manager() { return risk_manager_; }
    /**
     * @brief Access the metrics tracker.
     */
    metrics::MetricsTracker& metrics() { return metrics_; }
    /**
     * @brief Get the current regime state.
     * @return Regime state snapshot.
     */
    const regime::RegimeState& current_regime() const { return regime_tracker_.current_state(); }
    /**
     * @brief Get the current simulated time.
     * @return Timestamp for the event loop.
     */
    Timestamp current_time() const { return event_loop_.current_time(); }
    /**
     * @brief Access the hook system.
     */
    plugins::HookSystem& hooks() { return hooks_; }
    /**
     * @brief Access the hook manager.
     */
    plugins::HookManager& hook_manager() { return hook_manager_; }
    /**
     * @brief Configure execution models from config.
     * @param config Execution configuration.
     */
    void configure_execution(const Config& config);
    /**
     * @brief Configure risk controls from config.
     * @param config Risk configuration.
     */
    void configure_risk(const Config& config);
    /**
     * @brief Configure regime detection from config.
     * @param config Regime configuration.
     */
    void configure_regime(const Config& config);
    /**
     * @brief Configure parallel run context.
     * @param context Parallel configuration.
     */
    void set_parallel_context(ParallelContext context);
    /**
     * @brief Run parallel parameter sweeps.
     * @param param_sets Parameter sets to evaluate.
     * @param strategy_factory Factory to build strategies for each set.
     * @param num_threads Worker threads (-1 uses default).
     * @return Backtest results for each parameter set.
     */
    std::vector<BacktestResults> run_parallel(
        const std::vector<std::map<std::string, double>>& param_sets,
        std::function<std::unique_ptr<strategy::Strategy>(
            const std::map<std::string, double>&)> strategy_factory,
        int num_threads = -1);
    /**
     * @brief Register a hook callback with priority.
     * @param type Hook type.
     * @param hook Callback function.
     * @param priority Lower values run first.
     */
    void register_hook(plugins::HookType type,
                       std::function<plugins::HookResult(plugins::HookContext&)> hook,
                       int priority = 100);
    /**
     * @brief Register a progress callback.
     * @param callback Progress callback (percent, message).
     */
    void on_progress(std::function<void(double pct, const std::string& msg)> callback);
    /**
     * @brief Enable audit logging to the specified path.
     * @param path Output file path.
     */
    void set_audit_log_path(std::string path);
    /**
     * @brief Snapshot of results after run completion.
     * @return BacktestResults.
     */
    BacktestResults results() const;
    /**
     * @brief Advance the event loop by one step.
     * @return True if more events remain.
     */
    bool step();
    /**
     * @brief Run the event loop until a time limit.
     * @param end_time Time to stop at.
     */
    void run_until(Timestamp end_time);
    /**
     * @brief Run the event loop until exhaustion.
     */
    void run();
    /**
     * @brief Stop the engine and event loop.
     */
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
