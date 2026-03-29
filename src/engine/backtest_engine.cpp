#include "regimeflow/engine/backtest_engine.h"

#include "regimeflow/common/result.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/engine/order_routing.h"
#include "regimeflow/metrics/report.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <mutex>
#include <thread>

namespace regimeflow::engine
{
    BacktestEngine::BacktestEngine(const double initial_capital, std::string currency)
        : event_loop_(&event_queue_),
          portfolio_(initial_capital, std::move(currency)),
          market_data_(),
          timer_service_(&event_queue_),
          execution_pipeline_(&market_data_, &order_book_cache_, &event_queue_),
          regime_tracker_(nullptr) {
        event_loop_.set_dispatcher(&dispatcher_);
        install_default_handlers();
    }

    namespace {
        std::string bar_type_name(const data::BarType bar_type) {
            switch (bar_type) {
            case data::BarType::Time_1Min:
                return "1m";
            case data::BarType::Time_5Min:
                return "5m";
            case data::BarType::Time_15Min:
                return "15m";
            case data::BarType::Time_30Min:
                return "30m";
            case data::BarType::Time_1Hour:
                return "1h";
            case data::BarType::Time_4Hour:
                return "4h";
            case data::BarType::Time_1Day:
                return "1d";
            case data::BarType::Volume:
                return "volume";
            case data::BarType::Tick:
                return "tick";
            case data::BarType::Dollar:
                return "dollar";
            }
            return "unknown";
        }

        int parse_hhmm_utc(const std::string& value, const int fallback) {
            if (value.size() != 5 || value[2] != ':') {
                return fallback;
            }
            const int hour = std::stoi(value.substr(0, 2));
            const int minute = std::stoi(value.substr(3, 2));
            if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
                return fallback;
            }
            return hour * 60 + minute;
        }

        std::optional<int> parse_weekday_utc(const std::string& value) {
            std::string normalized;
            normalized.reserve(value.size());
            for (const char c : value) {
                normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            if (normalized == "0" || normalized == "sun" || normalized == "sunday") {
                return 0;
            }
            if (normalized == "1" || normalized == "mon" || normalized == "monday") {
                return 1;
            }
            if (normalized == "2" || normalized == "tue" || normalized == "tues"
                || normalized == "tuesday") {
                return 2;
            }
            if (normalized == "3" || normalized == "wed" || normalized == "wednesday") {
                return 3;
            }
            if (normalized == "4" || normalized == "thu" || normalized == "thurs"
                || normalized == "thursday") {
                return 4;
            }
            if (normalized == "5" || normalized == "fri" || normalized == "friday") {
                return 5;
            }
            if (normalized == "6" || normalized == "sat" || normalized == "saturday") {
                return 6;
            }
            return std::nullopt;
        }
    }  // namespace

    void BacktestEngine::cancel_day_orders_if_needed(const Timestamp timestamp) {
        const std::string day_stamp = timestamp.to_string("%Y-%m-%d");
        if (current_day_stamp_.empty()) {
            current_day_stamp_ = day_stamp;
            return;
        }
        if (day_stamp == current_day_stamp_) {
            return;
        }
        apply_daily_financing(timestamp);
        current_day_stamp_ = day_stamp;
        for (const auto& order : order_manager_.get_open_orders()) {
            if (order.tif != TimeInForce::Day) {
                continue;
            }
            events::Event cancel_event = events::make_order_event(
                events::OrderEventKind::Cancel,
                timestamp,
                order.id,
                0,
                0.0,
                0.0,
                order.symbol);
            event_queue_.push(std::move(cancel_event));
        }
    }

    void BacktestEngine::apply_daily_financing(const Timestamp timestamp) {
        if (!financing_policy_.enabled) {
            return;
        }
        double total_charge = 0.0;
        for (const auto& position : portfolio_.get_all_positions()) {
            if (position.quantity == 0.0) {
                continue;
            }
            const Price mark_price = position.current_price > 0.0 ? position.current_price : position.avg_cost;
            if (mark_price <= 0.0) {
                continue;
            }
            const double notional = std::abs(position.quantity) * mark_price;
            const double rate_bps = position.quantity >= 0.0
                                        ? financing_policy_.long_rate_bps_per_day
                                        : financing_policy_.short_borrow_bps_per_day;
            total_charge += notional * (rate_bps / 10000.0);
        }
        if (total_charge == 0.0) {
            return;
        }
        portfolio_.adjust_cash(-total_charge, timestamp);
        portfolio_.record_snapshot(timestamp);
        log_account_event(timestamp,
                          "Daily financing applied",
                          {
                              {"charge", std::to_string(total_charge)},
                              {"cash", std::to_string(portfolio_.cash())}
                          });
        evaluate_account_state(timestamp, "financing");
    }

    void BacktestEngine::cancel_expired_orders(const Timestamp timestamp) {
        const auto expired = order_manager_.expired_order_ids(timestamp);
        if (expired.empty()) {
            return;
        }
        for (const auto id : expired) {
            SymbolId symbol = 0;
            if (const auto order = order_manager_.get_order(id)) {
                symbol = order->symbol;
            }
            events::Event cancel_event = events::make_order_event(
                events::OrderEventKind::Cancel,
                timestamp,
                id,
                0,
                0.0,
                0.0,
                symbol);
            event_queue_.push(std::move(cancel_event));
        }
    }

    std::vector<data::Tick> BacktestEngine::build_execution_ticks(const data::Bar& bar) const {
        std::vector<Price> prices;
        prices.reserve(4);
        const auto append_price = [&prices](const Price price) {
            if (price <= 0.0) {
                return;
            }
            if (!prices.empty() && prices.back() == price) {
                return;
            }
            prices.emplace_back(price);
        };

        switch (synthetic_tick_profile_) {
        case SyntheticTickProfile::BarOpen:
            append_price(bar.open);
            break;
        case SyntheticTickProfile::Ohlc4Tick:
            append_price(bar.open);
            if (bar.close >= bar.open) {
                append_price(bar.low);
                append_price(bar.high);
            } else {
                append_price(bar.high);
                append_price(bar.low);
            }
            append_price(bar.close);
            break;
        case SyntheticTickProfile::BarClose:
            append_price(bar.close);
            break;
        }

        if (prices.empty()) {
            append_price(bar.close);
        }

        std::vector<data::Tick> ticks;
        ticks.reserve(prices.size());
        for (size_t i = 0; i < prices.size(); ++i) {
            data::Tick tick;
            tick.symbol = bar.symbol;
            tick.price = prices[i];
            tick.quantity = 0.0;
            tick.timestamp = bar.timestamp + Duration::microseconds(static_cast<int64_t>(i));
            ticks.emplace_back(std::move(tick));
        }
        return ticks;
    }

    void BacktestEngine::replay_execution_ticks(const data::Bar& bar) {
        for (const auto& tick : build_execution_ticks(bar)) {
            market_data_.update(tick);
            execution_pipeline_.on_market_update(tick.symbol, tick.timestamp);
        }
    }

    void BacktestEngine::evaluate_account_state(const Timestamp timestamp, const char* source) {
        const auto margin = portfolio_.margin_snapshot();

        if (margin.margin_call) {
            if (!margin_call_state_) {
                margin_call_state_ = true;
                log_account_event(timestamp,
                                  "Margin call threshold breached",
                                  {
                                      {"source", source},
                                      {"margin_excess", std::to_string(margin.margin_excess)},
                                      {"available_funds", std::to_string(margin.available_funds)}
                                  });
                if (account_enforcement_.enabled) {
                    if (account_enforcement_.margin_call_action == MarginCallAction::CancelOpenOrders
                        || account_enforcement_.margin_call_action == MarginCallAction::HaltTrading) {
                        std::vector<OrderId> open_ids;
                        for (const auto& order : order_manager_.get_open_orders()) {
                            open_ids.emplace_back(order.id);
                        }
                        for (const auto id : open_ids) {
                            (void)order_manager_.cancel_order(id);
                        }
                    }
                    if (account_enforcement_.margin_call_action == MarginCallAction::HaltTrading) {
                        account_trading_halted_ = true;
                    }
                }
            }
        } else {
            margin_call_state_ = false;
        }

        if (!margin.stop_out) {
            stop_out_state_ = false;
            return;
        }
        if (stop_out_state_) {
            return;
        }
        stop_out_state_ = true;
        log_account_event(timestamp,
                          "Stop-out threshold breached",
                          {
                              {"source", source},
                              {"equity", std::to_string(portfolio_.equity())},
                              {"maintenance_margin", std::to_string(margin.maintenance_margin)}
                          });
        if (!account_enforcement_.enabled
            || account_enforcement_.stop_out_action == StopOutAction::None) {
            return;
        }
        liquidate_for_stop_out(timestamp);
        if (account_enforcement_.halt_after_stop_out) {
            account_trading_halted_ = true;
        }
    }

    void BacktestEngine::liquidate_for_stop_out(const Timestamp timestamp) {
        std::vector<OrderId> open_ids;
        for (const auto& order : order_manager_.get_open_orders()) {
            open_ids.emplace_back(order.id);
        }
        for (const auto id : open_ids) {
            (void)order_manager_.cancel_order(id);
        }

        auto positions = portfolio_.get_all_positions();
        if (account_enforcement_.stop_out_action == StopOutAction::LiquidateWorstFirst) {
            std::ranges::sort(positions, [](const Position& lhs, const Position& rhs) {
                return lhs.unrealized_pnl() < rhs.unrealized_pnl();
            });
        }

        for (const auto& position : positions) {
            if (position.quantity == 0.0) {
                continue;
            }
            const Price liquidation_price = position.current_price > 0.0
                                                ? position.current_price
                                                : position.avg_cost;
            if (liquidation_price <= 0.0) {
                continue;
            }
            Order forced_order = Order::market(
                position.symbol,
                position.quantity > 0.0 ? OrderSide::Sell : OrderSide::Buy,
                std::abs(position.quantity));
            forced_order.tif = TimeInForce::IOC;
            forced_order.created_at = timestamp;
            forced_order.metadata["forced_liquidation"] = "true";
            forced_order.metadata["liquidation_reason"] = "stop_out";

            const auto fills = execution_pipeline_.simulate_immediate_fills(forced_order,
                                                                            liquidation_price,
                                                                            timestamp,
                                                                            false);
            for (const auto& fill : fills) {
                portfolio_.update_position(fill);
                portfolio_.record_snapshot(fill.timestamp);
                if (strategy_) {
                    strategy_->on_fill(fill);
                }
                strategy_manager_.on_fill(fill);
                log_account_event(fill.timestamp,
                                  "Stop-out liquidation fill",
                                  {
                                      {"symbol", SymbolRegistry::instance().lookup(position.symbol)},
                                      {"quantity", std::to_string(fill.quantity)},
                                      {"price", std::to_string(fill.price)},
                                      {"commission", std::to_string(fill.commission)},
                                      {"transaction_cost", std::to_string(fill.transaction_cost)}
                                  });
            }

            if (!fills.empty()
                && account_enforcement_.stop_out_action == StopOutAction::LiquidateWorstFirst
                && !portfolio_.margin_snapshot().stop_out) {
                break;
            }
        }
    }

    void BacktestEngine::record_audit_event(AuditEvent event) {
        journal_events_.emplace_back(event);
        if (!audit_logger_) {
            return;
        }
        audit_logger_->log(event);
    }

    void BacktestEngine::log_account_event(const Timestamp timestamp,
                                           const std::string& details,
                                           const std::map<std::string, std::string>& metadata) {
        AuditEvent event;
        event.timestamp = timestamp;
        event.type = AuditEvent::Type::AccountUpdate;
        event.details = details;
        event.metadata = metadata;
        record_audit_event(std::move(event));
    }

    void BacktestEngine::register_hook(const plugins::HookType type,
                                       std::function<plugins::HookResult(plugins::HookContext&)> hook,
                                       const int priority) {
        hook_manager_.register_hook(type, std::move(hook), priority);
    }

    void BacktestEngine::on_progress(std::function<void(double pct, const std::string& msg)> callback) {
        progress_callback_ = std::move(callback);
        if (!progress_callback_) {
            event_loop_.set_progress_callback({});
            return;
        }
        progress_total_estimate_ = 0;
        event_loop_.set_progress_callback([this](const size_t processed, const size_t remaining) {
            if (const size_t current_total = processed + remaining; current_total > progress_total_estimate_) {
                progress_total_estimate_ = current_total;
            }
            const double pct = progress_total_estimate_ == 0 ? 100.0
                                             : (100.0 * static_cast<double>(processed)
                                                / static_cast<double>(progress_total_estimate_));
            progress_callback_(pct, "processing events");
            if (remaining == 0 && processed > 0 && pct >= 100.0) {
                progress_callback_(100.0, "complete");
            }
        });
    }

    void BacktestEngine::set_audit_log_path(std::string path) {
        if (path.empty()) {
            audit_logger_.reset();
            return;
        }
        audit_logger_ = std::make_unique<AuditLogger>(std::move(path));
    }

    void BacktestEngine::enqueue(events::Event event) {
        event_queue_.push(std::move(event));
    }

    void BacktestEngine::load_data(std::unique_ptr<data::DataIterator> iterator) {
        symbols_with_real_ticks_.clear();
        event_generator_ = std::make_unique<EventGenerator>(std::move(iterator), &event_queue_);
        event_generator_->enqueue_all();
    }

    void BacktestEngine::load_data(std::unique_ptr<data::DataIterator> bar_iterator,
                                   std::unique_ptr<data::TickIterator> tick_iterator,
                                   std::unique_ptr<data::OrderBookIterator> book_iterator) {
        symbols_with_real_ticks_.clear();
        event_generator_ = std::make_unique<EventGenerator>(std::move(bar_iterator),
                                                            std::move(tick_iterator),
                                                            std::move(book_iterator),
                                                            &event_queue_);
        event_generator_->enqueue_all();
    }

    void BacktestEngine::set_strategy(std::unique_ptr<strategy::Strategy> strategy, Config config) {
        strategy_ = std::move(strategy);
        if (const auto configured_name = config.get_as<std::string>("name")) {
            dashboard_setup_.strategy_name = *configured_name;
        } else if (strategy_ && dashboard_setup_.strategy_name.empty()) {
            dashboard_setup_.strategy_name = "custom";
        }
        if (strategy_) {
            strategy_context_ = std::make_unique<strategy::StrategyContext>(
                &order_manager_, &portfolio_, &event_loop_, &market_data_, &order_book_cache_,
                &timer_service_, &regime_tracker_, std::move(config));
            strategy_->set_context(strategy_context_.get());
            strategy_->initialize(*strategy_context_);
        }
    }

    void BacktestEngine::add_strategy(std::unique_ptr<strategy::Strategy> strategy) {
        strategy_manager_.add_strategy(std::move(strategy));
    }

    void BacktestEngine::set_execution_model(std::unique_ptr<execution::ExecutionModel> model) {
        execution_pipeline_.set_execution_model(std::move(model));
    }

    void BacktestEngine::set_commission_model(std::unique_ptr<execution::CommissionModel> model) {
        execution_pipeline_.set_commission_model(std::move(model));
    }

    void BacktestEngine::set_transaction_cost_model(
        std::unique_ptr<execution::TransactionCostModel> model) {
        execution_pipeline_.set_transaction_cost_model(std::move(model));
    }

    void BacktestEngine::set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model) {
        execution_pipeline_.set_market_impact_model(std::move(model));
    }

    void BacktestEngine::set_latency_model(std::unique_ptr<execution::LatencyModel> model) {
        execution_pipeline_.set_latency_model(std::move(model));
    }

    void BacktestEngine::set_regime_detector(std::unique_ptr<regime::RegimeDetector> detector) {
        regime_tracker_ = RegimeTracker(std::move(detector));
    }

    void BacktestEngine::configure_execution(const Config& config) {
        execution_config_ = config;
        if (const auto configured = config.get_as<std::string>("model")) {
            dashboard_setup_.execution_model = *configured;
        }
        if (const auto configured = config.get_as<std::string>("simulation.tick_mode")) {
            dashboard_setup_.tick_mode = *configured;
        } else {
            dashboard_setup_.tick_mode = "synthetic_ticks";
        }
        if (const auto configured = config.get_as<std::string>("simulation.synthetic_tick_profile")) {
            dashboard_setup_.synthetic_tick_profile = *configured;
        } else {
            dashboard_setup_.synthetic_tick_profile = "bar_close";
        }
        if (const auto configured = config.get_as<std::string>("simulation.bar_mode")) {
            dashboard_setup_.bar_mode = *configured;
        } else {
            dashboard_setup_.bar_mode = "close_only";
        }
        if (const auto configured = config.get_as<std::string>("policy.fill")) {
            dashboard_setup_.fill_policy = *configured;
        } else {
            dashboard_setup_.fill_policy = "preserve";
        }
        if (const auto configured = config.get_as<std::string>("policy.price_drift_action")) {
            dashboard_setup_.price_drift_action = *configured;
        } else {
            dashboard_setup_.price_drift_action = "ignore";
        }
        dashboard_setup_.routing_enabled = config.get_as<bool>("routing.enabled").value_or(false);
        dashboard_setup_.queue_enabled = config.get_as<bool>("queue.enabled").value_or(false);
        dashboard_setup_.session_enabled = config.get_as<bool>("session.enabled").value_or(false);
        dashboard_setup_.margin_enabled =
            config.get_as<double>("margin.initial_margin_ratio").has_value()
            || config.get_as<double>("account.margin.initial_margin_ratio").has_value();
        dashboard_setup_.financing_enabled =
            config.get_as<bool>("financing.enabled").value_or(false)
            || config.get_as<bool>("account.financing.enabled").value_or(false);
        auto margin_profile = MarginProfile::from_config(config, "margin");
        margin_profile = MarginProfile::from_config(config, "account.margin", margin_profile);
        portfolio_.configure_margin(margin_profile);

        account_enforcement_ = {};
        if (const auto configured = config.get_as<bool>("enforcement.enabled")) {
            account_enforcement_.enabled = *configured;
        }
        if (const auto configured = config.get_as<bool>("account.enforcement.enabled")) {
            account_enforcement_.enabled = *configured;
        }
        if (const auto action = config.get_as<std::string>("enforcement.margin_call_action")) {
            if (*action == "cancel_open_orders" || *action == "cancel") {
                account_enforcement_.margin_call_action = MarginCallAction::CancelOpenOrders;
            } else if (*action == "halt_trading" || *action == "halt") {
                account_enforcement_.margin_call_action = MarginCallAction::HaltTrading;
            }
        }
        if (const auto action = config.get_as<std::string>("account.enforcement.margin_call_action")) {
            if (*action == "cancel_open_orders" || *action == "cancel") {
                account_enforcement_.margin_call_action = MarginCallAction::CancelOpenOrders;
            } else if (*action == "halt_trading" || *action == "halt") {
                account_enforcement_.margin_call_action = MarginCallAction::HaltTrading;
            } else {
                account_enforcement_.margin_call_action = MarginCallAction::Ignore;
            }
        }
        if (const auto action = config.get_as<std::string>("enforcement.stop_out_action")) {
            if (*action == "liquidate_all") {
                account_enforcement_.stop_out_action = StopOutAction::LiquidateAll;
            } else if (*action == "liquidate_worst_first" || *action == "worst_first") {
                account_enforcement_.stop_out_action = StopOutAction::LiquidateWorstFirst;
            }
        }
        if (const auto action = config.get_as<std::string>("account.enforcement.stop_out_action")) {
            if (*action == "liquidate_all") {
                account_enforcement_.stop_out_action = StopOutAction::LiquidateAll;
            } else if (*action == "liquidate_worst_first" || *action == "worst_first") {
                account_enforcement_.stop_out_action = StopOutAction::LiquidateWorstFirst;
            } else {
                account_enforcement_.stop_out_action = StopOutAction::None;
            }
        }
        if (const auto configured = config.get_as<bool>("enforcement.halt_after_stop_out")) {
            account_enforcement_.halt_after_stop_out = *configured;
        }
        if (const auto configured = config.get_as<bool>("account.enforcement.halt_after_stop_out")) {
            account_enforcement_.halt_after_stop_out = *configured;
        }

        financing_policy_ = {};
        if (const auto configured = config.get_as<bool>("financing.enabled")) {
            financing_policy_.enabled = *configured;
        }
        if (const auto configured = config.get_as<bool>("account.financing.enabled")) {
            financing_policy_.enabled = *configured;
        }
        if (const auto configured = config.get_as<double>("financing.long_rate_bps_per_day")) {
            financing_policy_.long_rate_bps_per_day = *configured;
        }
        if (const auto configured = config.get_as<double>("account.financing.long_rate_bps_per_day")) {
            financing_policy_.long_rate_bps_per_day = *configured;
        }
        if (const auto configured = config.get_as<double>("financing.short_borrow_bps_per_day")) {
            financing_policy_.short_borrow_bps_per_day = *configured;
        }
        if (const auto configured = config.get_as<double>("account.financing.short_borrow_bps_per_day")) {
            financing_policy_.short_borrow_bps_per_day = *configured;
        }

        account_trading_halted_ = false;
        margin_call_state_ = false;
        stop_out_state_ = false;

        auto execution_model = execution::ExecutionFactory::create_execution_model(config);
        auto slippage = execution::ExecutionFactory::create_slippage_model(config);
        auto commission = execution::ExecutionFactory::create_commission_model(config);
        auto transaction_cost = execution::ExecutionFactory::create_transaction_cost_model(config);
        auto market_impact = execution::ExecutionFactory::create_market_impact_model(config);
        auto latency = execution::ExecutionFactory::create_latency_model(config);

        if (execution_model) {
            set_execution_model(std::move(execution_model));
        } else if (slippage) {
            set_execution_model(std::make_unique<execution::BasicExecutionModel>(std::move(slippage)));
        }
        if (commission) {
            set_commission_model(std::move(commission));
        }
        if (transaction_cost) {
            set_transaction_cost_model(std::move(transaction_cost));
        }
        if (market_impact) {
            set_market_impact_model(std::move(market_impact));
        }
        if (latency) {
            execution_pipeline_.set_latency_model(std::move(latency));
        }
        if (const auto fill_policy = config.get_as<std::string>("policy.fill")) {
            if (*fill_policy == "ioc") {
                execution_pipeline_.set_default_fill_policy(
                    ExecutionPipeline::FillPolicy::IOC);
            } else if (*fill_policy == "fok") {
                execution_pipeline_.set_default_fill_policy(
                    ExecutionPipeline::FillPolicy::FOK);
            } else if (*fill_policy == "return") {
                execution_pipeline_.set_default_fill_policy(
                    ExecutionPipeline::FillPolicy::Return);
            } else {
                execution_pipeline_.set_default_fill_policy(
                    ExecutionPipeline::FillPolicy::PreserveOrder);
            }
        } else {
            execution_pipeline_.set_default_fill_policy(
                ExecutionPipeline::FillPolicy::PreserveOrder);
        }
        double max_deviation_bps = 0.0;
        if (const auto configured = config.get_as<double>("policy.max_deviation_bps")) {
            max_deviation_bps = *configured;
        }
        auto drift_action = ExecutionPipeline::PriceDriftAction::Ignore;
        if (const auto action = config.get_as<std::string>("policy.price_drift_action")) {
            if (*action == "reject") {
                drift_action = ExecutionPipeline::PriceDriftAction::Reject;
            } else if (*action == "requote") {
                drift_action = ExecutionPipeline::PriceDriftAction::Requote;
            }
        }
        execution_pipeline_.set_price_drift_rule(max_deviation_bps, drift_action);
        bool queue_enabled = false;
        if (const auto configured = config.get_as<bool>("queue.enabled")) {
            queue_enabled = *configured;
        }
        double queue_progress_fraction = 1.0;
        if (const auto configured = config.get_as<double>("queue.progress_fraction")) {
            queue_progress_fraction = *configured;
        }
        double queue_default_visible_qty = 1.0;
        if (const auto configured = config.get_as<double>("queue.default_visible_qty")) {
            queue_default_visible_qty = *configured;
        }
        auto queue_depth_mode = ExecutionPipeline::QueueDepthMode::TopOnly;
        if (const auto configured = config.get_as<std::string>("queue.depth_mode")) {
            if (*configured == "price_level") {
                queue_depth_mode = ExecutionPipeline::QueueDepthMode::PriceLevel;
            } else if (*configured == "full_depth") {
                queue_depth_mode = ExecutionPipeline::QueueDepthMode::FullDepth;
            }
        }
        execution_pipeline_.set_queue_model(
            queue_enabled,
            queue_progress_fraction,
            queue_default_visible_qty,
            queue_depth_mode,
            config.get_as<double>("queue.aging_fraction").value_or(0.0),
            config.get_as<double>("queue.replenishment_fraction").value_or(0.0));
        if (const auto bar_mode = config.get_as<std::string>("simulation.bar_mode")) {
            if (*bar_mode == "open" || *bar_mode == "open_only") {
                execution_pipeline_.set_bar_simulation_mode(
                    ExecutionPipeline::BarSimulationMode::OpenOnly);
            } else if (*bar_mode == "ohlc" || *bar_mode == "intrabar_ohlc") {
                execution_pipeline_.set_bar_simulation_mode(
                    ExecutionPipeline::BarSimulationMode::IntrabarOhlc);
            } else {
                execution_pipeline_.set_bar_simulation_mode(
                    ExecutionPipeline::BarSimulationMode::CloseOnly);
            }
        } else {
            execution_pipeline_.set_bar_simulation_mode(
                ExecutionPipeline::BarSimulationMode::CloseOnly);
        }
        if (const auto tick_mode = config.get_as<std::string>("simulation.tick_mode")) {
            if (*tick_mode == "real_ticks" || *tick_mode == "real") {
                tick_simulation_mode_ = TickSimulationMode::RealTicks;
            } else {
                tick_simulation_mode_ = TickSimulationMode::SyntheticTicks;
            }
        } else {
            tick_simulation_mode_ = TickSimulationMode::SyntheticTicks;
        }
        synthetic_tick_profile_ = SyntheticTickProfile::BarClose;
        if (const auto profile = config.get_as<std::string>("simulation.synthetic_tick_profile")) {
            if (*profile == "bar_open" || *profile == "open_only") {
                synthetic_tick_profile_ = SyntheticTickProfile::BarOpen;
            } else if (*profile == "ohlc_4tick" || *profile == "intrabar_ohlc") {
                synthetic_tick_profile_ = SyntheticTickProfile::Ohlc4Tick;
            }
        } else if (const auto bar_mode = config.get_as<std::string>("simulation.bar_mode")) {
            if (*bar_mode == "open" || *bar_mode == "open_only") {
                synthetic_tick_profile_ = SyntheticTickProfile::BarOpen;
            } else if (*bar_mode == "ohlc" || *bar_mode == "intrabar_ohlc") {
                synthetic_tick_profile_ = SyntheticTickProfile::Ohlc4Tick;
            }
        }
        symbols_with_real_ticks_.clear();

        ExecutionPipeline::SessionPolicy session_policy;
        if (const auto configured = config.get_as<bool>("session.enabled")) {
            session_policy.enabled = *configured;
        }
        if (const auto configured = config.get_as<std::string>("session.start_hhmm")) {
            session_policy.start_minute_utc =
                parse_hhmm_utc(*configured, session_policy.start_minute_utc);
        }
        if (const auto configured = config.get_as<std::string>("session.end_hhmm")) {
            session_policy.end_minute_utc =
                parse_hhmm_utc(*configured, session_policy.end_minute_utc);
        }
        if (const auto configured = config.get_as<int64_t>("session.open_auction_minutes")) {
            session_policy.open_auction_minutes = static_cast<int>(*configured);
        }
        if (const auto configured = config.get_as<int64_t>("session.close_auction_minutes")) {
            session_policy.close_auction_minutes = static_cast<int>(*configured);
        }
        if (const auto configured = config.get_as<bool>("session.halt")) {
            session_policy.halt_all = *configured;
        }
        if (const auto configured = config.get_as<ConfigValue::Array>("session.halted_symbols")) {
            for (const auto& entry : *configured) {
                if (const auto* symbol = entry.get_if<std::string>()) {
                    session_policy.halted_symbols.emplace(SymbolRegistry::instance().intern(*symbol));
                }
            }
        }
        if (const auto configured = config.get_as<ConfigValue::Array>("session.weekdays")) {
            for (const auto& entry : *configured) {
                if (const auto* day = entry.get_if<std::string>()) {
                    if (const auto parsed = parse_weekday_utc(*day)) {
                        session_policy.allowed_weekdays.emplace(*parsed);
                    }
                } else if (const auto* day = entry.get_if<int64_t>()) {
                    if (*day >= 0 && *day <= 6) {
                        session_policy.allowed_weekdays.emplace(static_cast<int>(*day));
                    }
                }
            }
        }
        if (const auto configured = config.get_as<ConfigValue::Array>("session.closed_dates")) {
            for (const auto& entry : *configured) {
                if (const auto* date = entry.get_if<std::string>()) {
                    session_policy.closed_dates.emplace(*date);
                }
            }
        }
        execution_pipeline_.set_session_policy(std::move(session_policy));

        const auto routing = RoutingConfig::from_config(config, "routing");
        if (routing.enabled) {
            order_manager_.set_router(
                std::make_unique<SmartOrderRouter>(routing),
                [this](const Order& order) {
                    RoutingContext ctx;
                    if (const auto book = order_book_cache_.latest(order.symbol)) {
                        if (!book->bids.empty()) {
                            ctx.bid = book->bids.front().price;
                        }
                        if (!book->asks.empty()) {
                            ctx.ask = book->asks.front().price;
                        }
                    }
                    if (const auto quote = market_data_.latest_quote(order.symbol)) {
                        if (quote->bid > 0.0) {
                            ctx.bid = quote->bid;
                        }
                        if (quote->ask > 0.0) {
                            ctx.ask = quote->ask;
                        }
                    }
                    if (const auto tick = market_data_.latest_tick(order.symbol)) {
                        if (tick->price > 0.0) {
                            ctx.last = tick->price;
                        }
                    } else if (const auto bar = market_data_.latest_bar(order.symbol)) {
                        if (bar->close > 0.0) {
                            ctx.last = bar->close;
                        }
                    }
                    return ctx;
                });
        } else {
            order_manager_.clear_router();
        }
    }

    void BacktestEngine::configure_risk(const Config& config) {
        risk_config_ = config;
        risk_manager_ = risk::RiskFactory::create_risk_manager(config);
        risk::StopLossConfig stop_cfg;
        if (const auto v = config.get_as<bool>("stop_loss.enable")) stop_cfg.enable_fixed = *v;
        if (const auto v = config.get_as<double>("stop_loss.pct")) stop_cfg.stop_loss_pct = *v;
        if (const auto v = config.get_as<bool>("trailing_stop.enable")) stop_cfg.enable_trailing = *v;
        if (const auto v = config.get_as<double>("trailing_stop.pct")) stop_cfg.trailing_stop_pct = *v;
        if (const auto v = config.get_as<bool>("atr_stop.enable")) stop_cfg.enable_atr = *v;
        if (const auto v = config.get_as<int64_t>("atr_stop.window")) stop_cfg.atr_window = static_cast<int>(*v);
        if (const auto v = config.get_as<double>("atr_stop.multiplier")) stop_cfg.atr_multiplier = *v;
        if (const auto v = config.get_as<bool>("time_stop.enable")) stop_cfg.enable_time = *v;
        if (const auto v = config.get_as<int64_t>("time_stop.max_holding_seconds")) {
            stop_cfg.max_holding_seconds = *v;
        }
        stop_loss_manager_.configure(stop_cfg);
    }

    void BacktestEngine::configure_regime(const Config& config) {
        regime_config_ = config;
        if (auto detector = regime::RegimeFactory::create_detector(config)) {
            set_regime_detector(std::move(detector));
        }
    }

    void BacktestEngine::set_parallel_context(ParallelContext context) {
        dashboard_setup_.symbols = context.symbols;
        dashboard_setup_.start_date = context.range.start.microseconds() == 0
            ? ""
            : context.range.start.to_string("%Y-%m-%d");
        dashboard_setup_.end_date = context.range.end.microseconds() == 0
            ? ""
            : context.range.end.to_string("%Y-%m-%d");
        dashboard_setup_.bar_type = bar_type_name(context.bar_type);
        parallel_context_ = std::move(context);
    }

    void BacktestEngine::set_dashboard_setup(DashboardSetup setup) {
        dashboard_setup_ = std::move(setup);
    }

    std::vector<BacktestResults> BacktestEngine::run_parallel(
        const std::vector<std::map<std::string, double>>& param_sets,
        const std::function<std::unique_ptr<strategy::Strategy>(
            const std::map<std::string, double>&)>& strategy_factory,
        int num_threads) const
    {
        if (!parallel_context_) {
            throw std::runtime_error("Parallel context not configured");
        }
        if (!strategy_factory) {
            throw std::runtime_error("Strategy factory not provided");
        }
        if (param_sets.empty()) {
            return {};
        }
        if (num_threads <= 0) {
            num_threads = static_cast<int>(std::thread::hardware_concurrency());
            if (num_threads <= 0) {
                num_threads = 1;
            }
        }

        std::vector<BacktestResults> results(param_sets.size());
        std::atomic<size_t> index{0};
        std::exception_ptr failure;
        std::mutex failure_mutex;

        auto worker = [&]() {
            while (true) {
                const size_t i = index.fetch_add(1, std::memory_order_relaxed);
                if (i >= param_sets.size()) {
                    break;
                }
                try {
                    const auto& params = param_sets[i];
                    BacktestEngine engine(portfolio_.initial_capital(), portfolio_.currency());
                    if (execution_config_) {
                        engine.configure_execution(*execution_config_);
                    }
                    if (risk_config_) {
                        engine.configure_risk(*risk_config_);
                    }
                    if (regime_config_) {
                        engine.configure_regime(*regime_config_);
                    }
                    auto run_setup = dashboard_setup_;
                    run_setup.optimization_enabled = true;
                    engine.set_dashboard_setup(std::move(run_setup));

                    const auto source = data::DataSourceFactory::create(parallel_context_->data_config);
                    if (!source) {
                        throw std::runtime_error("Failed to create data source");
                    }
                    std::vector<SymbolId> symbol_ids;
                    symbol_ids.reserve(parallel_context_->symbols.size());
                    for (const auto& sym : parallel_context_->symbols) {
                        symbol_ids.push_back(SymbolRegistry::instance().intern(sym));
                    }
                    auto bar_it = source->create_iterator(symbol_ids, parallel_context_->range,
                                                          parallel_context_->bar_type);
                    auto tick_it = source->create_tick_iterator(symbol_ids, parallel_context_->range);
                    auto book_it = source->create_book_iterator(symbol_ids, parallel_context_->range);
                    engine.load_data(std::move(bar_it), std::move(tick_it), std::move(book_it));

                    auto strat = strategy_factory(params);
                    if (!strat) {
                        throw std::runtime_error("Strategy factory returned null");
                    }
                    engine.set_strategy(std::move(strat));
                    engine.run();
                    results[i] = engine.results();
                } catch (...) {
                    std::lock_guard<std::mutex> lock(failure_mutex);
                    if (!failure) {
                        failure = std::current_exception();
                    }
                }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(num_threads));
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker);
        }
        for (auto& t : threads) {
            t.join();
        }
        if (failure) {
            std::rethrow_exception(failure);
        }
        return results;
    }

    void BacktestEngine::run() {
        if (!started_) {
            journal_events_.clear();
            journal_submitted_order_ids_.clear();
            if (progress_callback_) {
                progress_callback_(0.0, "starting");
            }
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStart;
            event.details = "Backtest start";
            record_audit_event(std::move(event));
            {
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event_loop_.current_time());
                hook_manager_.invoke(plugins::HookType::BacktestStart, ctx);
            }
            if (strategy_) {
                strategy_->on_start();
            }
            hooks_.run_start();
            if (strategy_context_) {
                strategy_manager_.initialize(*strategy_context_);
                strategy_manager_.start();
            }
            started_ = true;
        }
        event_loop_.run();
        if (event_queue_.empty()) {
            strategy_manager_.stop();
            hooks_.run_stop();
            if (strategy_) {
                strategy_->on_stop();
            }
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStop;
            event.details = "Backtest end";
            record_audit_event(std::move(event));
            {
                const auto summary = results();
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event_loop_.current_time());
                ctx.set_results(&summary);
                hook_manager_.invoke(plugins::HookType::BacktestEnd, ctx);
            }
            started_ = false;
        }
    }

    void BacktestEngine::stop() {
        event_loop_.stop();
    }

    bool BacktestEngine::step() {
        if (!started_) {
            journal_events_.clear();
            journal_submitted_order_ids_.clear();
            if (progress_callback_) {
                progress_callback_(0.0, "starting");
            }
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStart;
            event.details = "Backtest start";
            record_audit_event(std::move(event));
            {
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event_loop_.current_time());
                hook_manager_.invoke(plugins::HookType::BacktestStart, ctx);
            }
            if (strategy_) {
                strategy_->on_start();
            }
            hooks_.run_start();
            if (strategy_context_) {
                strategy_manager_.initialize(*strategy_context_);
                strategy_manager_.start();
            }
            started_ = true;
        }
        const bool processed = event_loop_.step();
        if (!processed || event_queue_.empty()) {
            strategy_manager_.stop();
            hooks_.run_stop();
            if (strategy_) {
                strategy_->on_stop();
            }
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStop;
            event.details = "Backtest end";
            record_audit_event(std::move(event));
            {
                auto summary = results();
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event_loop_.current_time());
                ctx.set_results(&summary);
                hook_manager_.invoke(plugins::HookType::BacktestEnd, ctx);
            }
            started_ = false;
        }
        return processed;
    }

    void BacktestEngine::run_until(const Timestamp end_time) {
        if (!started_) {
            journal_events_.clear();
            journal_submitted_order_ids_.clear();
            if (progress_callback_) {
                progress_callback_(0.0, "starting");
            }
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStart;
            event.details = "Backtest start";
            record_audit_event(std::move(event));
            {
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event_loop_.current_time());
                hook_manager_.invoke(plugins::HookType::BacktestStart, ctx);
            }
            if (strategy_) {
                strategy_->on_start();
            }
            hooks_.run_start();
            if (strategy_context_) {
                strategy_manager_.initialize(*strategy_context_);
                strategy_manager_.start();
            }
            started_ = true;
        }
        event_loop_.run_until(end_time);
        if (event_queue_.empty()) {
            strategy_manager_.stop();
            hooks_.run_stop();
            if (strategy_) {
                strategy_->on_stop();
            }
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStop;
            event.details = "Backtest end";
            record_audit_event(std::move(event));
            {
                const auto summary = results();
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event_loop_.current_time());
                ctx.set_results(&summary);
                hook_manager_.invoke(plugins::HookType::BacktestEnd, ctx);
            }
            started_ = false;
        }
    }

    BacktestResults BacktestEngine::results() const {
        BacktestResults result;
        result.setup = dashboard_setup_;
        if (const auto& equity = metrics_.equity_curve().equities(); !equity.empty()) {
            result.total_return = metrics_.equity_curve().total_return();
            result.max_drawdown = metrics_.drawdown().max_drawdown();
            result.metrics = metrics_;
        }
        result.fills = portfolio_.get_fills();
        result.regime_history = metrics_.regime_history();
        result.journal_events = journal_events_;
        return result;
    }

    DashboardSnapshot BacktestEngine::dashboard_snapshot() const {
        DashboardSnapshot snapshot;
        snapshot.setup = dashboard_setup_;
        snapshot.timestamp = current_time();
        snapshot.current_regime = current_regime();

        const auto current_portfolio = portfolio_.snapshot(snapshot.timestamp);
        snapshot.equity = current_portfolio.equity;
        snapshot.cash = current_portfolio.cash;
        snapshot.buying_power = current_portfolio.buying_power;
        snapshot.initial_margin = current_portfolio.initial_margin;
        snapshot.maintenance_margin = current_portfolio.maintenance_margin;
        snapshot.available_funds = current_portfolio.available_funds;
        snapshot.margin_excess = current_portfolio.margin_excess;
        snapshot.margin_call = current_portfolio.margin_call;
        snapshot.stop_out = current_portfolio.stop_out;

        snapshot.equity_curve = metrics_.portfolio_snapshots();
        if (snapshot.equity_curve.empty()
            || snapshot.equity_curve.back().timestamp != current_portfolio.timestamp) {
            snapshot.equity_curve.emplace_back(current_portfolio);
        }
        snapshot.positions.reserve(current_portfolio.positions.size());
        for (const auto& [symbol, position] : current_portfolio.positions) {
            (void)symbol;
            snapshot.positions.emplace_back(position);
        }
        snapshot.position_count = snapshot.positions.size();

        const auto fills = portfolio_.get_fills();
        snapshot.fill_count = fills.size();
        snapshot.venue_summary = summarize_dashboard_venues(fills);
        snapshot.recent_fills = fills;
        if (snapshot.recent_fills.size() > 10) {
            snapshot.recent_fills.erase(snapshot.recent_fills.begin(),
                                        snapshot.recent_fills.end() - 10);
        }
        for (const auto& fill : fills) {
            snapshot.total_commission += fill.commission;
            snapshot.total_transaction_cost += fill.transaction_cost;
            if (fill.is_maker) {
                snapshot.maker_fill_ratio += 1.0;
            }
        }
        snapshot.total_cost = snapshot.total_commission + snapshot.total_transaction_cost;
        if (!fills.empty()) {
            snapshot.maker_fill_ratio /= static_cast<double>(fills.size());
        }

        const auto report = metrics::build_report(metrics_, fills);
        snapshot.total_return = report.performance_summary.total_return;
        snapshot.max_drawdown = report.performance_summary.max_drawdown;
        snapshot.sharpe_ratio = report.performance_summary.sharpe_ratio;
        snapshot.sortino_ratio = report.performance_summary.sortino_ratio;
        snapshot.win_rate = report.performance_summary.win_rate;
        snapshot.profit_factor = report.performance_summary.profit_factor;

        const auto status_name = [](const OrderStatus status) {
            switch (status) {
            case OrderStatus::Created:
                return "created";
            case OrderStatus::Pending:
                return "pending";
            case OrderStatus::PartiallyFilled:
                return "partially_filled";
            case OrderStatus::Filled:
                return "filled";
            case OrderStatus::Cancelled:
                return "cancelled";
            case OrderStatus::Rejected:
                return "rejected";
            case OrderStatus::Invalid:
                return "invalid";
            }
            return "unknown";
        };

        const auto orders = order_manager_.get_open_orders();
        snapshot.open_order_count = orders.size();
        snapshot.open_orders.reserve(orders.size());
        for (const auto& order : orders) {
            DashboardOrderSummary summary;
            summary.id = order.id;
            try {
                summary.symbol = SymbolRegistry::instance().lookup(order.symbol);
            } catch (...) {
                summary.symbol = "unknown";
            }
            summary.side = order.side;
            summary.type = order.type;
            summary.quantity = order.quantity;
            summary.filled_quantity = order.filled_quantity;
            summary.limit_price = order.limit_price;
            summary.stop_price = order.stop_price;
            summary.avg_fill_price = order.avg_fill_price;
            summary.status = status_name(order.status);
            summary.updated_at = order.updated_at;
            snapshot.open_orders.emplace_back(std::move(summary));
        }

        snapshot.quote_summary.reserve(dashboard_setup_.symbols.size());
        for (const auto& symbol_name : dashboard_setup_.symbols) {
            const auto symbol_id = SymbolRegistry::instance().intern(symbol_name);
            if (const auto quote = market_data_.latest_quote(symbol_id)) {
                DashboardQuoteSummary summary;
                summary.symbol = symbol_name;
                summary.timestamp = quote->timestamp;
                summary.bid = quote->bid;
                summary.ask = quote->ask;
                summary.bid_size = quote->bid_size;
                summary.ask_size = quote->ask_size;
                snapshot.quote_summary.emplace_back(std::move(summary));
            }
        }

        if (account_trading_halted_) {
            snapshot.alerts.emplace_back("Trading halted by account enforcement");
        }
        if (margin_call_state_) {
            snapshot.alerts.emplace_back("Margin call active");
        }
        if (stop_out_state_) {
            snapshot.alerts.emplace_back("Stop-out active");
        }

        return snapshot;
    }

    std::string BacktestEngine::dashboard_snapshot_json() const {
        return dashboard_snapshot_to_json(dashboard_snapshot());
    }

    std::string BacktestEngine::dashboard_terminal(const DashboardRenderOptions& options) const {
        return render_dashboard_terminal(dashboard_snapshot(), options);
    }

    void BacktestEngine::install_default_handlers() {
        order_manager_.on_pre_submit([this](Order& order) {
            if (account_trading_halted_ && !order.metadata.contains("forced_liquidation")) {
                return Result<void>(Error(Error::Code::InvalidState, "Trading halted by account enforcement"));
            }
            plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                     &event_queue_, event_loop_.current_time());
            ctx.set_order(&order);
            if (hook_manager_.invoke(plugins::HookType::OrderSubmit, ctx)
                == plugins::HookResult::Cancel) {
                return Result<void>(Error(Error::Code::InvalidState, "Order rejected by hook"));
            }
            return Result<void>();
        });
        portfolio_.on_position_change([this](const Position& position) {
            stop_loss_manager_.on_position_update(position);
        });
        order_manager_.on_fill([this](const Fill& fill) {
            portfolio_.update_position(fill);
            portfolio_.record_snapshot(fill.timestamp);
            evaluate_account_state(fill.timestamp, "fill");
        });
        order_manager_.on_fill([this](const Fill& fill) {
            AuditEvent event;
            event.timestamp = fill.timestamp;
            event.type = AuditEvent::Type::OrderFilled;
            event.details = "Order filled";
            event.metadata["order_id"] = std::to_string(fill.order_id);
            event.metadata["symbol"] = SymbolRegistry::instance().lookup(fill.symbol);
            event.metadata["qty"] = std::to_string(fill.quantity);
            event.metadata["price"] = std::to_string(fill.price);
            record_audit_event(std::move(event));
        });
        order_manager_.on_fill([this](const Fill& fill) {
            if (strategy_) {
                strategy_->on_fill(fill);
            }
            strategy_manager_.on_fill(fill);
        });
        order_manager_.on_order_update([this](const Order& order) {
            execution_pipeline_.on_order_update(order);
            if (strategy_) {
                strategy_->on_order_update(order);
            }
            strategy_manager_.on_order_update(order);
        });
        order_manager_.on_order_update([this](const Order& order) {
            if (order.status == OrderStatus::Pending) {
                if (!journal_submitted_order_ids_.contains(order.id)) {
                    AuditEvent event;
                    event.timestamp = event_loop_.current_time();
                    event.type = AuditEvent::Type::OrderSubmitted;
                    event.details = "Order submitted";
                    event.metadata["order_id"] = std::to_string(order.id);
                    event.metadata["symbol"] = SymbolRegistry::instance().lookup(order.symbol);
                    event.metadata["qty"] = std::to_string(order.quantity);
                    event.metadata["side"] = order.side == OrderSide::Buy ? "buy" : "sell";
                    event.metadata["type"] = std::to_string(static_cast<int>(order.type));
                    if (order.limit_price > 0.0) {
                        event.metadata["limit_price"] = std::to_string(order.limit_price);
                    }
                    if (order.stop_price > 0.0) {
                        event.metadata["stop_price"] = std::to_string(order.stop_price);
                    }
                    record_audit_event(std::move(event));
                    journal_submitted_order_ids_.insert(order.id);
                }
                if (order.is_parent) {
                    if (const auto result = risk_manager_.validate(order, portfolio_); result.is_ok()) {
                        order_manager_.activate_routed_order(order.id);
                    } else {
                        order_manager_.update_order_status(order.id, OrderStatus::Rejected);
                    }
                    return;
                }
                if (const auto result = risk_manager_.validate(order, portfolio_); result.is_ok()) {
                    execution_pipeline_.on_order_submitted(order);
                } else {
                    order_manager_.update_order_status(order.id, OrderStatus::Rejected);
                }
            }
        });

        dispatcher_.set_market_handler([this](const events::Event& event) {
            hooks_.run_pre_event(event);
            const auto* payload = std::get_if<events::MarketEventPayload>(&event.payload);
            if (!payload) {
                return;
            }
            cancel_day_orders_if_needed(event.timestamp);
            cancel_expired_orders(event.timestamp);
            timer_service_.on_time_advance(event.timestamp);
            switch (payload->kind) {
                case events::MarketEventKind::Bar: {
                    const auto& bar = std::get<data::Bar>(payload->data);
                    {
                        plugins::HookContext ctx(&portfolio_, &market_data_,
                                                 &regime_tracker_.current_state(),
                                                 &event_queue_, event.timestamp);
                        ctx.set_bar(&bar);
                        if (hook_manager_.invoke(plugins::HookType::Bar, ctx)
                            == plugins::HookResult::Cancel) {
                            return;
                        }
                    }
                    market_data_.update(bar);
                    const bool use_bar_for_execution =
                        tick_simulation_mode_ == TickSimulationMode::SyntheticTicks
                        || !symbols_with_real_ticks_.contains(bar.symbol);
                    if (use_bar_for_execution) {
                        replay_execution_ticks(bar);
                    }
                    portfolio_.mark_to_market(bar.symbol, bar.close, bar.timestamp);
                    portfolio_.record_snapshot(bar.timestamp);
                    evaluate_account_state(bar.timestamp, "bar");
                    stop_loss_manager_.on_bar(bar, order_manager_);
                    metrics_.update(bar.timestamp, portfolio_, regime_tracker_.current_state());
                    if (auto transition = regime_tracker_.on_bar(bar)) {
                        events::Event evt = events::make_system_event(
                            events::SystemEventKind::RegimeChange, transition->timestamp);
                        event_queue_.push(std::move(evt));
                        plugins::HookContext ctx(&portfolio_, &market_data_,
                                                 &regime_tracker_.current_state(),
                                                 &event_queue_, transition->timestamp);
                        ctx.set_regime_change(&(*transition));
                        if (hook_manager_.invoke(plugins::HookType::RegimeChange, ctx)
                            == plugins::HookResult::Cancel) {
                            return;
                        }
                        AuditEvent audit_event;
                        audit_event.timestamp = transition->timestamp;
                        audit_event.type = AuditEvent::Type::RegimeChange;
                        audit_event.details = "Regime change";
                        audit_event.metadata["from"] = std::to_string(static_cast<int>(transition->from));
                        audit_event.metadata["to"] = std::to_string(static_cast<int>(transition->to));
                        audit_event.metadata["confidence"] = std::to_string(transition->confidence);
                        record_audit_event(std::move(audit_event));
                        if (strategy_) {
                            strategy_->on_regime_change(*transition);
                        }
                        strategy_manager_.on_regime_change(*transition);
                    }
                    if (strategy_) {
                        strategy_->on_bar(bar);
                    }
                    strategy_manager_.on_bar(bar);
                    break;
                }
                case events::MarketEventKind::Tick: {
                    const auto& tick = std::get<data::Tick>(payload->data);
                    {
                        plugins::HookContext ctx(&portfolio_, &market_data_,
                                                 &regime_tracker_.current_state(),
                                                 &event_queue_, event.timestamp);
                        ctx.set_tick(&tick);
                        if (hook_manager_.invoke(plugins::HookType::Tick, ctx)
                            == plugins::HookResult::Cancel) {
                            return;
                        }
                    }
                    symbols_with_real_ticks_.insert(tick.symbol);
                    market_data_.update(tick);
                    execution_pipeline_.on_market_update(tick.symbol, tick.timestamp);
                    portfolio_.mark_to_market(tick.symbol, tick.price, tick.timestamp);
                    portfolio_.record_snapshot(tick.timestamp);
                    evaluate_account_state(tick.timestamp, "tick");
                    stop_loss_manager_.on_tick(tick, order_manager_);
                    metrics_.update(tick.timestamp, portfolio_, regime_tracker_.current_state());
                    if (auto transition = regime_tracker_.on_tick(tick)) {
                        events::Event evt = events::make_system_event(
                            events::SystemEventKind::RegimeChange, transition->timestamp);
                        event_queue_.push(std::move(evt));
                        plugins::HookContext ctx(&portfolio_, &market_data_,
                                                 &regime_tracker_.current_state(),
                                                 &event_queue_, transition->timestamp);
                        ctx.set_regime_change(&(*transition));
                        if (hook_manager_.invoke(plugins::HookType::RegimeChange, ctx)
                            == plugins::HookResult::Cancel) {
                            return;
                        }
                        AuditEvent audit_event;
                        audit_event.timestamp = transition->timestamp;
                        audit_event.type = AuditEvent::Type::RegimeChange;
                        audit_event.details = "Regime change";
                        audit_event.metadata["from"] = std::to_string(static_cast<int>(transition->from));
                        audit_event.metadata["to"] = std::to_string(static_cast<int>(transition->to));
                        audit_event.metadata["confidence"] = std::to_string(transition->confidence);
                        record_audit_event(std::move(audit_event));
                        if (strategy_) {
                            strategy_->on_regime_change(*transition);
                        }
                        strategy_manager_.on_regime_change(*transition);
                    }
                    if (strategy_) {
                        strategy_->on_tick(tick);
                    }
                    strategy_manager_.on_tick(tick);
                    break;
                }
                case events::MarketEventKind::Quote: {
                    const auto& quote = std::get<data::Quote>(payload->data);
                    {
                        plugins::HookContext ctx(&portfolio_, &market_data_,
                                                 &regime_tracker_.current_state(),
                                                 &event_queue_, event.timestamp);
                        ctx.set_quote(&quote);
                        if (hook_manager_.invoke(plugins::HookType::Quote, ctx)
                            == plugins::HookResult::Cancel) {
                            return;
                        }
                    }
                    symbols_with_real_ticks_.insert(quote.symbol);
                    market_data_.update(quote);
                    execution_pipeline_.on_market_update(quote.symbol, quote.timestamp);
                    portfolio_.mark_to_market(quote.symbol, quote.mid(), quote.timestamp);
                    portfolio_.record_snapshot(quote.timestamp);
                    evaluate_account_state(quote.timestamp, "quote");
                    if (strategy_) {
                        strategy_->on_quote(quote);
                    }
                    strategy_manager_.on_quote(quote);
                    break;
                }
                case events::MarketEventKind::Book:
                    {
                        const auto& book = std::get<data::OrderBook>(payload->data);
                        {
                            plugins::HookContext ctx(&portfolio_, &market_data_,
                                                     &regime_tracker_.current_state(),
                                                     &event_queue_, event.timestamp);
                            ctx.set_book(&book);
                            if (hook_manager_.invoke(plugins::HookType::Book, ctx)
                                == plugins::HookResult::Cancel) {
                                return;
                            }
                        }
                        symbols_with_real_ticks_.insert(book.symbol);
                        order_book_cache_.update(book);
                        execution_pipeline_.on_market_update(book.symbol, book.timestamp);
                        if (strategy_) {
                            strategy_->on_order_book(book);
                        }
                        strategy_manager_.on_order_book(book);
                        break;
                    }
                }
                hooks_.run_post_event(event);
            });

        dispatcher_.set_order_handler([this](const events::Event& event) {
            hooks_.run_pre_event(event);
            const auto* payload = std::get_if<events::OrderEventPayload>(&event.payload);
            if (!payload) {
                return;
            }
            switch (payload->kind) {
                case events::OrderEventKind::Fill: {
                    Fill fill;
                    fill.order_id = payload->order_id;
                    fill.parent_order_id = payload->parent_order_id;
                    fill.id = payload->fill_id;
                    fill.symbol = event.symbol;
                    fill.quantity = payload->quantity;
                    fill.price = payload->price;
                    fill.commission = payload->commission;
                    fill.transaction_cost = payload->transaction_cost;
                    fill.is_maker = payload->is_maker;
                    fill.venue = payload->venue;
                    fill.timestamp = event.timestamp;
                    {
                        plugins::HookContext ctx(&portfolio_, &market_data_,
                                                 &regime_tracker_.current_state(),
                                                 &event_queue_, event.timestamp);
                        ctx.set_fill(&fill);
                        if (hook_manager_.invoke(plugins::HookType::Fill, ctx)
                            == plugins::HookResult::Cancel) {
                            return;
                        }
                    }
                    order_manager_.process_fill(fill);
                    break;
                }
                case events::OrderEventKind::NewOrder:
                order_manager_.update_order_status(payload->order_id, OrderStatus::Pending);
                break;
            case events::OrderEventKind::Cancel:
            {
                order_manager_.update_order_status(payload->order_id, OrderStatus::Cancelled);
                AuditEvent audit_event;
                audit_event.timestamp = event.timestamp;
                audit_event.type = AuditEvent::Type::OrderCancelled;
                audit_event.details = "Order cancelled";
                audit_event.metadata["order_id"] = std::to_string(payload->order_id);
                record_audit_event(std::move(audit_event));
                break;
            }
            case events::OrderEventKind::Reject:
            {
                order_manager_.update_order_status(payload->order_id, OrderStatus::Rejected);
                AuditEvent audit_event;
                audit_event.timestamp = event.timestamp;
                audit_event.type = AuditEvent::Type::OrderRejected;
                audit_event.details = "Order rejected";
                audit_event.metadata["order_id"] = std::to_string(payload->order_id);
                record_audit_event(std::move(audit_event));
                break;
            }
            case events::OrderEventKind::Update:
                order_manager_.update_order_status(payload->order_id, OrderStatus::Pending);
                break;
        }
        hooks_.run_post_event(event);
    });

        dispatcher_.set_system_handler([this](const events::Event& event) {
            hooks_.run_pre_event(event);
            const auto* payload = std::get_if<events::SystemEventPayload>(&event.payload);
            if (!payload) {
                return;
            }
            if (payload->kind == events::SystemEventKind::DayStart) {
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event.timestamp);
                hook_manager_.invoke(plugins::HookType::DayStart, ctx);
            }
            if (payload->kind == events::SystemEventKind::EndOfDay) {
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event.timestamp);
                hook_manager_.invoke(plugins::HookType::DayEnd, ctx);
            }
            if (payload->kind == events::SystemEventKind::Timer) {
                plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                         &event_queue_, event.timestamp);
                ctx.set_timer_id(payload->id);
                if (hook_manager_.invoke(plugins::HookType::Timer, ctx)
                    == plugins::HookResult::Cancel) {
                    hooks_.run_post_event(event);
                    return;
                }
                if (strategy_) {
                    strategy_->on_timer(payload->id);
                }
                strategy_manager_.on_timer(payload->id);
            }
            if (payload->kind == events::SystemEventKind::TradingHalt
                || payload->kind == events::SystemEventKind::TradingResume) {
                const bool halted = payload->kind == events::SystemEventKind::TradingHalt;
                if (payload->id.empty() || payload->id == "*") {
                    execution_pipeline_.set_global_halt(halted);
                } else {
                    execution_pipeline_.set_symbol_halt(
                        SymbolRegistry::instance().intern(payload->id),
                        halted);
                }
            }
            hooks_.run_post_event(event);
        });
    }
}  // namespace regimeflow::engine
