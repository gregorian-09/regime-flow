#include "regimeflow/engine/backtest_engine.h"

#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/common/result.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace regimeflow::engine {

BacktestEngine::BacktestEngine(double initial_capital, std::string currency)
    : event_loop_(&event_queue_),
      portfolio_(initial_capital, std::move(currency)),
      market_data_(),
      timer_service_(&event_queue_),
      execution_pipeline_(&order_manager_, &market_data_, &order_book_cache_, &event_queue_),
      regime_tracker_(nullptr) {
    event_loop_.set_dispatcher(&dispatcher_);
    install_default_handlers();
}

void BacktestEngine::register_hook(plugins::HookType type,
                                   std::function<plugins::HookResult(plugins::HookContext&)> hook,
                                   int priority) {
    hook_manager_.register_hook(type, std::move(hook), priority);
}

void BacktestEngine::on_progress(std::function<void(double pct, const std::string& msg)> callback) {
    progress_callback_ = std::move(callback);
    if (!progress_callback_) {
        event_loop_.set_progress_callback({});
        return;
    }
    progress_total_estimate_ = 0;
    event_loop_.set_progress_callback([this](size_t processed, size_t remaining) {
        size_t current_total = processed + remaining;
        if (current_total > progress_total_estimate_) {
            progress_total_estimate_ = current_total;
        }
        double pct = progress_total_estimate_ == 0 ? 100.0
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
    event_generator_ = std::make_unique<EventGenerator>(std::move(iterator), &event_queue_);
    event_generator_->enqueue_all();
}

void BacktestEngine::load_data(std::unique_ptr<data::DataIterator> bar_iterator,
                               std::unique_ptr<data::TickIterator> tick_iterator,
                               std::unique_ptr<data::OrderBookIterator> book_iterator) {
    event_generator_ = std::make_unique<EventGenerator>(std::move(bar_iterator),
                                                        std::move(tick_iterator),
                                                        std::move(book_iterator),
                                                        &event_queue_);
    event_generator_->enqueue_all();
}

void BacktestEngine::set_strategy(std::unique_ptr<strategy::Strategy> strategy, Config config) {
    strategy_ = std::move(strategy);
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
}

void BacktestEngine::configure_risk(const Config& config) {
    risk_config_ = config;
    risk_manager_ = risk::RiskFactory::create_risk_manager(config);
    risk::StopLossConfig stop_cfg;
    if (auto v = config.get_as<bool>("stop_loss.enable")) stop_cfg.enable_fixed = *v;
    if (auto v = config.get_as<double>("stop_loss.pct")) stop_cfg.stop_loss_pct = *v;
    if (auto v = config.get_as<bool>("trailing_stop.enable")) stop_cfg.enable_trailing = *v;
    if (auto v = config.get_as<double>("trailing_stop.pct")) stop_cfg.trailing_stop_pct = *v;
    if (auto v = config.get_as<bool>("atr_stop.enable")) stop_cfg.enable_atr = *v;
    if (auto v = config.get_as<int64_t>("atr_stop.window")) stop_cfg.atr_window = static_cast<int>(*v);
    if (auto v = config.get_as<double>("atr_stop.multiplier")) stop_cfg.atr_multiplier = *v;
    if (auto v = config.get_as<bool>("time_stop.enable")) stop_cfg.enable_time = *v;
    if (auto v = config.get_as<int64_t>("time_stop.max_holding_seconds")) {
        stop_cfg.max_holding_seconds = *v;
    }
    stop_loss_manager_.configure(stop_cfg);
}

void BacktestEngine::configure_regime(const Config& config) {
    regime_config_ = config;
    auto detector = regime::RegimeFactory::create_detector(config);
    if (detector) {
        set_regime_detector(std::move(detector));
    }
}

void BacktestEngine::set_parallel_context(ParallelContext context) {
    parallel_context_ = std::move(context);
}

std::vector<BacktestResults> BacktestEngine::run_parallel(
    const std::vector<std::map<std::string, double>>& param_sets,
    std::function<std::unique_ptr<strategy::Strategy>(
        const std::map<std::string, double>&)> strategy_factory,
    int num_threads) {
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
            size_t i = index.fetch_add(1, std::memory_order_relaxed);
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

                auto source = data::DataSourceFactory::create(parallel_context_->data_config);
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
        if (progress_callback_) {
            progress_callback_(0.0, "starting");
        }
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStart;
            event.details = "Backtest start";
            audit_logger_->log(event);
        }
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
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStop;
            event.details = "Backtest end";
            audit_logger_->log(event);
        }
        {
            auto summary = results();
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
        if (progress_callback_) {
            progress_callback_(0.0, "starting");
        }
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStart;
            event.details = "Backtest start";
            audit_logger_->log(event);
        }
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
    bool processed = event_loop_.step();
    if (!processed || event_queue_.empty()) {
        strategy_manager_.stop();
        hooks_.run_stop();
        if (strategy_) {
            strategy_->on_stop();
        }
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStop;
            event.details = "Backtest end";
            audit_logger_->log(event);
        }
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

void BacktestEngine::run_until(Timestamp end_time) {
    if (!started_) {
        if (progress_callback_) {
            progress_callback_(0.0, "starting");
        }
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStart;
            event.details = "Backtest start";
            audit_logger_->log(event);
        }
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
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::SystemStop;
            event.details = "Backtest end";
            audit_logger_->log(event);
        }
        {
            auto summary = results();
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
    const auto& equity = metrics_.equity_curve().equities();
    if (!equity.empty()) {
        result.total_return = metrics_.equity_curve().total_return();
        result.max_drawdown = metrics_.drawdown().max_drawdown();
        result.metrics = metrics_;
    }
    result.fills = portfolio_.get_fills();
    return result;
}

void BacktestEngine::install_default_handlers() {
    order_manager_.on_pre_submit([this](Order& order) {
        plugins::HookContext ctx(&portfolio_, &market_data_, &regime_tracker_.current_state(),
                                 &event_queue_, event_loop_.current_time());
        ctx.set_order(&order);
        if (hook_manager_.invoke(plugins::HookType::OrderSubmit, ctx)
            == plugins::HookResult::Cancel) {
            return Result<void>(Error(Error::Code::InvalidState, "Order rejected by hook"));
        }
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = event_loop_.current_time();
            event.type = AuditEvent::Type::OrderSubmitted;
            event.details = "Order submitted";
            event.metadata["order_id"] = std::to_string(order.id);
            event.metadata["symbol"] = SymbolRegistry::instance().lookup(order.symbol);
            event.metadata["qty"] = std::to_string(order.quantity);
            event.metadata["side"] = order.side == OrderSide::Buy ? "buy" : "sell";
            event.metadata["type"] = std::to_string(static_cast<int>(order.type));
            audit_logger_->log(event);
        }
        return Result<void>();
    });
    portfolio_.on_position_change([this](const Position& position) {
        stop_loss_manager_.on_position_update(position);
    });
    order_manager_.on_fill([this](const Fill& fill) { portfolio_.update_position(fill); });
    order_manager_.on_fill([this](const Fill& fill) {
        if (!audit_logger_) {
            return;
        }
        AuditEvent event;
        event.timestamp = fill.timestamp;
        event.type = AuditEvent::Type::OrderFilled;
        event.details = "Order filled";
        event.metadata["order_id"] = std::to_string(fill.order_id);
        event.metadata["symbol"] = SymbolRegistry::instance().lookup(fill.symbol);
        event.metadata["qty"] = std::to_string(fill.quantity);
        event.metadata["price"] = std::to_string(fill.price);
        audit_logger_->log(event);
    });
    order_manager_.on_fill([this](const Fill& fill) {
        if (strategy_) {
            strategy_->on_fill(fill);
        }
        strategy_manager_.on_fill(fill);
    });
    order_manager_.on_order_update([this](const Order& order) {
        if (strategy_) {
            strategy_->on_order_update(order);
        }
        strategy_manager_.on_order_update(order);
    });
    order_manager_.on_order_update([this](const Order& order) {
        if (order.status == OrderStatus::Pending) {
            if (auto result = risk_manager_.validate(order, portfolio_); result.is_ok()) {
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
                portfolio_.mark_to_market(bar.symbol, bar.close, bar.timestamp);
                stop_loss_manager_.on_bar(bar, order_manager_);
                metrics_.update(bar.timestamp, portfolio_, regime_tracker_.current_state().regime);
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
                    if (audit_logger_) {
                        audit_logger_->log_regime_change(*transition);
                    }
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
                market_data_.update(tick);
                portfolio_.mark_to_market(tick.symbol, tick.price, tick.timestamp);
                stop_loss_manager_.on_tick(tick, order_manager_);
                metrics_.update(tick.timestamp, portfolio_, regime_tracker_.current_state().regime);
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
                    if (audit_logger_) {
                        audit_logger_->log_regime_change(*transition);
                    }
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
                market_data_.update(quote);
                portfolio_.mark_to_market(quote.symbol, quote.mid(), quote.timestamp);
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
                order_book_cache_.update(book);
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
                fill.id = payload->fill_id;
                fill.symbol = event.symbol;
                fill.quantity = payload->quantity;
                fill.price = payload->price;
                fill.commission = payload->commission;
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
                order_manager_.update_order_status(payload->order_id, OrderStatus::Cancelled);
                if (audit_logger_) {
                    AuditEvent audit_event;
                    audit_event.timestamp = event.timestamp;
                    audit_event.type = AuditEvent::Type::OrderCancelled;
                    audit_event.details = "Order cancelled";
                    audit_event.metadata["order_id"] = std::to_string(payload->order_id);
                    audit_logger_->log(audit_event);
                }
                break;
            case events::OrderEventKind::Reject:
                order_manager_.update_order_status(payload->order_id, OrderStatus::Rejected);
                if (audit_logger_) {
                    AuditEvent audit_event;
                    audit_event.timestamp = event.timestamp;
                    audit_event.type = AuditEvent::Type::OrderRejected;
                    audit_event.details = "Order rejected";
                    audit_event.metadata["order_id"] = std::to_string(payload->order_id);
                    audit_logger_->log(audit_event);
                }
                break;
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
        hooks_.run_post_event(event);
    });
}

}  // namespace regimeflow::engine
