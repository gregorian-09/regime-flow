#include "regimeflow/live/live_engine.h"

#include "regimeflow/live/alpaca_adapter.h"
#include "regimeflow/live/binance_adapter.h"
#include "regimeflow/live/ib_adapter.h"
#include "regimeflow/regime/regime_factory.h"
#include "regimeflow/risk/risk_factory.h"
#include "regimeflow/strategy/strategy_factory.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <type_traits>

#if defined(_WIN32)
#define NOMINMAX
#include <psapi.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/host_info.h>
#include <sys/sysctl.h>
#endif

namespace regimeflow::live {

namespace {

const char* status_name(LiveOrderStatus status) {
    switch (status) {
        case LiveOrderStatus::PendingNew: return "PendingNew";
        case LiveOrderStatus::New: return "New";
        case LiveOrderStatus::PartiallyFilled: return "PartiallyFilled";
        case LiveOrderStatus::Filled: return "Filled";
        case LiveOrderStatus::PendingCancel: return "PendingCancel";
        case LiveOrderStatus::Cancelled: return "Cancelled";
        case LiveOrderStatus::Rejected: return "Rejected";
        case LiveOrderStatus::Expired: return "Expired";
        case LiveOrderStatus::Error: return "Error";
        default: return "Unknown";
    }
}

Timestamp best_order_timestamp(const LiveOrder& order) {
    if (order.filled_at.microseconds() != 0) {
        return order.filled_at;
    }
    if (order.acked_at.microseconds() != 0) {
        return order.acked_at;
    }
    if (order.submitted_at.microseconds() != 0) {
        return order.submitted_at;
    }
    if (order.created_at.microseconds() != 0) {
        return order.created_at;
    }
    return Timestamp::now();
}

std::unique_ptr<BrokerAdapter> build_broker(const LiveConfig& config) {
    if (config.broker_type == "alpaca") {
        AlpacaAdapter::Config cfg;
        auto it = config.broker_config.find("api_key");
        if (it != config.broker_config.end()) cfg.api_key = it->second;
        it = config.broker_config.find("secret_key");
        if (it != config.broker_config.end()) cfg.secret_key = it->second;
        it = config.broker_config.find("base_url");
        if (it != config.broker_config.end()) cfg.base_url = it->second;
        it = config.broker_config.find("data_url");
        if (it != config.broker_config.end()) cfg.data_url = it->second;
        it = config.broker_config.find("stream_url");
        if (it != config.broker_config.end()) cfg.stream_url = it->second;
        it = config.broker_config.find("stream_auth_template");
        if (it != config.broker_config.end()) cfg.stream_auth_template = it->second;
        it = config.broker_config.find("stream_subscribe_template");
        if (it != config.broker_config.end()) cfg.stream_subscribe_template = it->second;
        it = config.broker_config.find("stream_unsubscribe_template");
        if (it != config.broker_config.end()) cfg.stream_unsubscribe_template = it->second;
        it = config.broker_config.find("stream_ca_bundle_path");
        if (it != config.broker_config.end()) cfg.stream_ca_bundle_path = it->second;
        it = config.broker_config.find("stream_expected_hostname");
        if (it != config.broker_config.end()) cfg.stream_expected_hostname = it->second;
        it = config.broker_config.find("enable_streaming");
        if (it != config.broker_config.end()) cfg.enable_streaming = (it->second == "true");
        it = config.broker_config.find("paper");
        if (it != config.broker_config.end()) cfg.paper = (it->second == "true");
        it = config.broker_config.find("timeout_seconds");
        if (it != config.broker_config.end()) cfg.timeout_seconds = std::stoi(it->second);
        return std::make_unique<AlpacaAdapter>(std::move(cfg));
    }
    if (config.broker_type == "binance") {
        BinanceAdapter::Config cfg;
        auto it = config.broker_config.find("api_key");
        if (it != config.broker_config.end()) cfg.api_key = it->second;
        it = config.broker_config.find("secret_key");
        if (it != config.broker_config.end()) cfg.secret_key = it->second;
        it = config.broker_config.find("base_url");
        if (it != config.broker_config.end()) cfg.base_url = it->second;
        it = config.broker_config.find("stream_url");
        if (it != config.broker_config.end()) cfg.stream_url = it->second;
        it = config.broker_config.find("stream_subscribe_template");
        if (it != config.broker_config.end()) cfg.stream_subscribe_template = it->second;
        it = config.broker_config.find("stream_unsubscribe_template");
        if (it != config.broker_config.end()) cfg.stream_unsubscribe_template = it->second;
        it = config.broker_config.find("stream_ca_bundle_path");
        if (it != config.broker_config.end()) cfg.stream_ca_bundle_path = it->second;
        it = config.broker_config.find("stream_expected_hostname");
        if (it != config.broker_config.end()) cfg.stream_expected_hostname = it->second;
        it = config.broker_config.find("timeout_seconds");
        if (it != config.broker_config.end()) cfg.timeout_seconds = std::stoi(it->second);
        it = config.broker_config.find("enable_streaming");
        if (it != config.broker_config.end()) cfg.enable_streaming = (it->second == "true");
        it = config.broker_config.find("recv_window_ms");
        if (it != config.broker_config.end()) cfg.recv_window_ms = std::stoll(it->second);
        return std::make_unique<BinanceAdapter>(std::move(cfg));
    }
    if (config.broker_type == "ib") {
        IBAdapter::Config cfg;
        auto it = config.broker_config.find("host");
        if (it != config.broker_config.end()) cfg.host = it->second;
        it = config.broker_config.find("port");
        if (it != config.broker_config.end()) cfg.port = std::stoi(it->second);
        it = config.broker_config.find("client_id");
        if (it != config.broker_config.end()) cfg.client_id = std::stoi(it->second);
        return std::make_unique<IBAdapter>(std::move(cfg));
    }
    return nullptr;
}

}  // namespace

LiveTradingEngine::LiveTradingEngine(const LiveConfig& config) : LiveTradingEngine(config, nullptr) {}

LiveTradingEngine::LiveTradingEngine(const LiveConfig& config, std::unique_ptr<BrokerAdapter> broker)
    : config_(config) {
    if (broker) {
        broker_ = std::move(broker);
    } else {
        broker_ = build_broker(config_);
    }
    order_manager_ = std::make_unique<LiveOrderManager>(broker_.get());
    risk_manager_ = std::make_unique<risk::RiskManager>(risk::RiskFactory::create_risk_manager(
        config_.risk_config));
    if (config_.enable_message_queue) {
        mq_adapter_ = create_message_queue_adapter(config_.message_queue);
    }

    strategy_order_manager_.on_order_update([this](const engine::Order& order) {
        if (!broker_ || !trading_enabled_) {
            return;
        }
        if (order.status != engine::OrderStatus::Created) {
            return;
        }
        auto now = Timestamp::now();
        if (config_.max_orders_per_minute > 0) {
            std::lock_guard<std::mutex> lock(rate_mutex_);
            const auto window = Duration::seconds(60).total_microseconds();
            while (!order_timestamps_.empty() &&
                   (now.microseconds() - order_timestamps_.front().microseconds()) > window) {
                order_timestamps_.pop_front();
            }
            if (static_cast<int>(order_timestamps_.size()) >= config_.max_orders_per_minute) {
                if (error_cb_) {
                    error_cb_("Rate limit exceeded: max_orders_per_minute");
                }
                if (audit_logger_) {
                    audit_logger_->log_error("Rate limit exceeded: max_orders_per_minute");
                }
                strategy_order_manager_.update_order_status(order.id,
                                                            engine::OrderStatus::Rejected);
                return;
            }
        }
        int max_per_second = config_.max_orders_per_second;
        if (max_per_second <= 0 && broker_) {
            max_per_second = broker_->max_orders_per_second();
        }
        if (max_per_second > 0) {
            std::lock_guard<std::mutex> lock(rate_mutex_);
            const auto window = Duration::seconds(1).total_microseconds();
            while (!second_order_timestamps_.empty() &&
                   (now.microseconds() - second_order_timestamps_.front().microseconds()) > window) {
                second_order_timestamps_.pop_front();
            }
            if (static_cast<int>(second_order_timestamps_.size()) >= max_per_second) {
                if (error_cb_) {
                    error_cb_("Rate limit exceeded: max_orders_per_second");
                }
                if (audit_logger_) {
                    audit_logger_->log_error("Rate limit exceeded: max_orders_per_second");
                }
                strategy_order_manager_.update_order_status(order.id,
                                                            engine::OrderStatus::Rejected);
                return;
            }
        }
        if (config_.max_order_value > 0.0) {
            Price price = 0.0;
            if (order.type == engine::OrderType::Limit && order.limit_price > 0) {
                price = order.limit_price;
            } else {
                auto it = last_prices_.find(order.symbol);
                if (it != last_prices_.end()) {
                    price = it->second;
                }
            }
            if (price <= 0.0) {
                if (error_cb_) {
                    error_cb_("Cannot evaluate order notional without price");
                }
                if (audit_logger_) {
                    audit_logger_->log_error("Cannot evaluate order notional without price");
                }
                strategy_order_manager_.update_order_status(order.id,
                                                            engine::OrderStatus::Rejected);
                return;
            }
            double notional = std::abs(order.quantity) * price;
            if (notional > config_.max_order_value) {
                if (error_cb_) {
                    error_cb_("Order exceeds max_order_value");
                }
                if (audit_logger_) {
                    audit_logger_->log_error("Order exceeds max_order_value");
                }
                strategy_order_manager_.update_order_status(order.id,
                                                            engine::OrderStatus::Rejected);
                return;
            }
        }
        if (risk_manager_ && portfolio_) {
            auto risk_check = risk_manager_->validate(order, *portfolio_);
            if (risk_check.is_err()) {
                if (error_cb_) {
                    error_cb_(risk_check.error().to_string());
                }
                if (audit_logger_) {
                    AuditEvent event;
                    event.timestamp = now;
                    event.type = AuditEvent::Type::RiskLimitBreached;
                    event.details = risk_check.error().to_string();
                    audit_logger_->log(event);
                }
                strategy_order_manager_.update_order_status(order.id,
                                                            engine::OrderStatus::Rejected);
                return;
            }
        }
        {
            std::lock_guard<std::mutex> lock(broker_mutex_);
            if (broker_order_ids_.count(order.id)) {
                return;
            }
        }
        auto submit_res = order_manager_->submit_order(order);
        if (submit_res.is_ok()) {
            auto live_order = order_manager_->get_order(submit_res.value());
            if (!live_order.has_value()) {
                if (error_cb_) {
                    error_cb_("Live order not available after submit");
                }
                strategy_order_manager_.update_order_status(order.id,
                                                            engine::OrderStatus::Rejected);
                return;
            }
            {
                std::lock_guard<std::mutex> lock(broker_mutex_);
                broker_order_ids_[order.id] = live_order->broker_order_id;
                broker_to_order_ids_[live_order->broker_order_id] = order.id;
            }
            if (config_.max_orders_per_minute > 0) {
                std::lock_guard<std::mutex> lock(rate_mutex_);
                order_timestamps_.push_back(now);
            }
            if (max_per_second > 0) {
                std::lock_guard<std::mutex> lock(rate_mutex_);
                second_order_timestamps_.push_back(now);
            }
            if (audit_logger_) {
                AuditEvent event;
                event.timestamp = now;
                event.type = AuditEvent::Type::OrderSubmitted;
                event.details = "Order submitted";
                event.metadata["broker_order_id"] = live_order->broker_order_id;
                event.metadata["symbol"] = SymbolRegistry::instance().lookup(order.symbol);
                audit_logger_->log(event);
            }
        } else {
            if (error_cb_) {
                error_cb_(submit_res.error().to_string());
            }
            if (audit_logger_) {
                audit_logger_->log_error(submit_res.error().to_string());
            }
            strategy_order_manager_.update_order_status(order.id, engine::OrderStatus::Rejected);
        }
    });

    Config strategy_cfg = config_.strategy_config;
    strategy_cfg.set("type", config_.strategy_name);
    strategy_ = strategy::StrategyFactory::instance().create(strategy_cfg);

    Config regime_cfg;
    regime_cfg.set("type", "constant");
    regime_detector_ = regime::RegimeFactory::create_detector(regime_cfg);
    feature_extractor_ = std::make_unique<regime::FeatureExtractor>(config_.regime_feature_window);

    if (!config_.log_dir.empty()) {
        std::filesystem::create_directories(config_.log_dir);
        audit_logger_ = std::make_unique<AuditLogger>(config_.log_dir + "/audit.log");
    }
    last_market_data_ = Timestamp::now();
}

LiveTradingEngine::~LiveTradingEngine() {
    stop();
}

Result<void> LiveTradingEngine::start() {
    if (!broker_) {
        return Error(Error::Code::InvalidState, "Broker adapter not configured");
    }
    if (config_.enable_message_queue && !mq_adapter_) {
        return Error(Error::Code::InvalidState, "Message queue adapter not configured");
    }
    if (running_) {
        return Ok();
    }

    auto res = broker_->connect();
    if (res.is_err()) {
        return res;
    }

    event_bus_.start();
    market_sub_id_ = event_bus_.subscribe(LiveTopic::MarketData, [this](const LiveMessage& msg) {
        auto payload = std::get_if<MarketDataUpdate>(&msg.payload);
        if (!payload) {
            return;
        }
        if (!market_queue_.push(*payload)) {
            add_alert("Market queue full: dropping update");
        } else {
            queue_cv_.notify_one();
        }
    });

    broker_->on_market_data([this](const MarketDataUpdate& update) {
        LiveMessage msg;
        msg.topic = LiveTopic::MarketData;
        msg.payload = update;
        event_bus_.publish(std::move(msg));
    });

    broker_->on_execution_report([this](const ExecutionReport& report) {
        LiveMessage msg;
        msg.topic = LiveTopic::ExecutionReport;
        msg.payload = report;
        event_bus_.publish(std::move(msg));
        handle_execution_report(report);
    });

    broker_->on_position_update([this](const Position& position) {
        LiveMessage msg;
        msg.topic = LiveTopic::PositionUpdate;
        msg.payload = position;
        event_bus_.publish(std::move(msg));
        apply_position_update(position, Timestamp::now());
    });

    if (config_.enable_message_queue && mq_adapter_) {
        auto mq_res = mq_adapter_->connect();
        if (mq_res.is_err()) {
            return mq_res;
        }
        mq_adapter_->on_message([this](const LiveMessage& msg) {
            LiveMessage incoming = msg;
            incoming.origin = "mq";
            event_bus_.publish(std::move(incoming));
        });
        mq_forward_sub_id_ = event_bus_.subscribe(LiveTopic::MarketData, [this](const LiveMessage& msg) {
            if (msg.origin == "mq") {
                return;
            }
            if (mq_adapter_) {
                mq_adapter_->publish(msg);
            }
        });
        mq_forward_extra_ids_.push_back(event_bus_.subscribe(
            LiveTopic::ExecutionReport, [this](const LiveMessage& msg) {
            if (msg.origin == "mq") {
                return;
            }
            if (mq_adapter_) {
                mq_adapter_->publish(msg);
            }
        }));
        mq_forward_extra_ids_.push_back(event_bus_.subscribe(
            LiveTopic::PositionUpdate, [this](const LiveMessage& msg) {
            if (msg.origin == "mq") {
                return;
            }
            if (mq_adapter_) {
                mq_adapter_->publish(msg);
            }
        }));
    }

    running_ = true;
    trading_enabled_ = true;
    event_loop_thread_ = std::thread(&LiveTradingEngine::event_loop, this);
    regime_thread_ = std::thread(&LiveTradingEngine::regime_update_loop, this);
    last_order_reconcile_ = Timestamp::now();
    last_position_refresh_ = last_order_reconcile_;
    last_account_refresh_ = last_order_reconcile_;

    if (!config_.symbols.empty()) {
        broker_->subscribe_market_data(config_.symbols);
    }

    auto account = broker_->get_account_info();
    portfolio_ = std::make_unique<engine::Portfolio>(account.equity);
    last_account_info_ = account;
    daily_start_equity_ = account.equity;
    daily_pnl_ = 0.0;
    portfolio_->set_cash(account.cash, Timestamp::now());

    auto positions = broker_->get_positions();
    apply_positions(positions, Timestamp::now());

    if (strategy_) {
        strategy_ctx_ = std::make_unique<strategy::StrategyContext>(&strategy_order_manager_,
                                                                    portfolio_.get(),
                                                                    nullptr,
                                                                    nullptr,
                                                                    nullptr,
                                                                    nullptr,
                                                                    nullptr,
                                                                    config_.strategy_config);
        strategy_->set_context(strategy_ctx_.get());
        strategy_->initialize(*strategy_ctx_);
        strategy_->on_start();
    }

    update_dashboard_snapshot("start");

    if (audit_logger_) {
        AuditEvent event;
        event.timestamp = Timestamp::now();
        event.type = AuditEvent::Type::SystemStart;
        event.details = "Live engine started";
        audit_logger_->log(event);
    }

    return Ok();
}

void LiveTradingEngine::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    queue_cv_.notify_all();
    if (regime_thread_.joinable()) {
        regime_thread_.join();
    }
    if (event_loop_thread_.joinable()) {
        event_loop_thread_.join();
    }
    if (broker_) {
        broker_->disconnect();
    }
    if (market_sub_id_ != 0) {
        event_bus_.unsubscribe(market_sub_id_);
        market_sub_id_ = 0;
    }
    if (mq_forward_sub_id_ != 0) {
        event_bus_.unsubscribe(mq_forward_sub_id_);
        mq_forward_sub_id_ = 0;
    }
    for (auto id : mq_forward_extra_ids_) {
        event_bus_.unsubscribe(id);
    }
    mq_forward_extra_ids_.clear();
    event_bus_.stop();
    if (mq_adapter_) {
        mq_adapter_->disconnect();
    }
    if (strategy_) {
        strategy_->on_stop();
    }
    if (audit_logger_) {
        AuditEvent event;
        event.timestamp = Timestamp::now();
        event.type = AuditEvent::Type::SystemStop;
        event.details = "Live engine stopped";
        audit_logger_->log(event);
    }
}

bool LiveTradingEngine::is_running() const {
    return running_.load();
}

LiveTradingEngine::EngineStatus LiveTradingEngine::get_status() const {
    EngineStatus status;
    status.connected = broker_ ? broker_->is_connected() : false;
    status.trading_enabled = trading_enabled_;
    status.current_regime = current_regime_;
    status.open_orders = static_cast<int>(order_manager_->get_open_orders().size());
    if (last_account_info_.equity > 0.0) {
        status.equity = last_account_info_.equity;
    } else {
        status.equity = portfolio_ ? portfolio_->equity() : 0.0;
    }
    status.daily_pnl = daily_pnl_;
    status.last_update = Timestamp::now();
    return status;
}

LiveTradingEngine::DashboardSnapshot LiveTradingEngine::get_dashboard_snapshot() const {
    std::lock_guard<std::mutex> lock(dashboard_mutex_);
    return last_dashboard_;
}

LiveTradingEngine::SystemHealth LiveTradingEngine::get_system_health() const {
    std::lock_guard<std::mutex> lock(health_mutex_);
    return last_health_;
}

void LiveTradingEngine::enable_trading() {
    trading_enabled_ = true;
}

void LiveTradingEngine::disable_trading() {
    trading_enabled_ = false;
}

void LiveTradingEngine::close_all_positions() {
    if (!portfolio_) {
        return;
    }
    for (const auto& position : portfolio_->get_all_positions()) {
        if (position.quantity == 0) {
            continue;
        }
        engine::Order order;
        order.symbol = position.symbol;
        order.quantity = std::abs(position.quantity);
        order.side = position.quantity > 0 ? engine::OrderSide::Sell : engine::OrderSide::Buy;
        order.type = engine::OrderType::Market;
        if (broker_) {
            broker_->submit_order(order);
        }
    }
}

void LiveTradingEngine::on_trade(std::function<void(const Trade&)> cb) {
    trade_cb_ = std::move(cb);
}

void LiveTradingEngine::on_regime_change(
    std::function<void(const regime::RegimeTransition&)> cb) {
    regime_cb_ = std::move(cb);
}

void LiveTradingEngine::on_error(std::function<void(const std::string&)> cb) {
    error_cb_ = std::move(cb);
}

void LiveTradingEngine::on_dashboard_update(
    std::function<void(const DashboardSnapshot&)> cb) {
    dashboard_cb_ = std::move(cb);
}

void LiveTradingEngine::event_loop() {
    last_event_loop_tick_ = Timestamp::now();
    while (running_) {
        auto loop_start = Timestamp::now();
        broker_->poll();
        if (mq_adapter_) {
            mq_adapter_->poll();
            if (!mq_adapter_->is_connected()) {
                add_alert("Message queue disconnected");
            }
        }
        check_heartbeat();
        attempt_reconnect();
        if (config_.order_reconcile_interval.total_microseconds() > 0) {
            Timestamp now = Timestamp::now();
            if ((now - last_order_reconcile_).total_microseconds()
                >= config_.order_reconcile_interval.total_microseconds()) {
                reconcile_orders();
                last_order_reconcile_ = now;
            }
        }
        if (config_.position_reconcile_interval.total_microseconds() > 0) {
            Timestamp now = Timestamp::now();
            if ((now - last_position_refresh_).total_microseconds()
                >= config_.position_reconcile_interval.total_microseconds()) {
                refresh_positions();
                last_position_refresh_ = now;
            }
        }
        if (config_.account_refresh_interval.total_microseconds() > 0) {
            Timestamp now = Timestamp::now();
            if ((now - last_account_refresh_).total_microseconds()
                >= config_.account_refresh_interval.total_microseconds()) {
                refresh_account_info();
                last_account_refresh_ = now;
            }
        }

        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait_for(lock, std::chrono::milliseconds(50), [this] {
            return !market_queue_.empty() || !running_;
        });
        lock.unlock();
        MarketDataUpdate update;
        while (market_queue_.pop(update)) {
            handle_market_data(update);
        }
        last_event_loop_tick_ = loop_start;
        sample_system_health();
    }
}

void LiveTradingEngine::regime_update_loop() {
    last_retrain_ = Timestamp::now();
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (!config_.enable_regime_updates || !regime_detector_) {
            continue;
        }
        Timestamp now = Timestamp::now();
        if (config_.regime_retrain_interval.total_microseconds() <= 0) {
            continue;
        }
        if ((now - last_retrain_).total_microseconds()
            < config_.regime_retrain_interval.total_microseconds()) {
            continue;
        }
        std::deque<regime::FeatureVector> snapshot;
        {
            std::lock_guard<std::mutex> lock(feature_mutex_);
            snapshot = feature_buffer_;
        }
        if (snapshot.size() < config_.regime_retrain_min_samples) {
            continue;
        }
        std::vector<regime::FeatureVector> features(snapshot.begin(), snapshot.end());
        regime_detector_->train(features);
        last_retrain_ = now;
        if (audit_logger_) {
            AuditEvent event;
            event.timestamp = now;
            event.type = AuditEvent::Type::RegimeChange;
            event.details = "Regime model retrained";
            audit_logger_->log(event);
        }
    }
}

void LiveTradingEngine::handle_market_data(const MarketDataUpdate& update) {
    if (!portfolio_) {
        return;
    }
    last_market_data_ = Timestamp::now();
    heartbeat_alerted_ = false;
    bool snapshot_updated = false;
    Timestamp snapshot_time = Timestamp::now();
    std::visit([&](const auto& data) {
        using T = std::decay_t<decltype(data)>;
        Price price = 0.0;
        bool updated_regime = false;
        if constexpr (std::is_same_v<T, data::Bar>) {
            price = data.close;
            snapshot_time = data.timestamp;
            if (feature_extractor_) {
                auto features = feature_extractor_->on_bar(data);
                std::lock_guard<std::mutex> lock(feature_mutex_);
                feature_buffer_.push_back(std::move(features));
                if (feature_buffer_.size() > config_.regime_retrain_min_samples * 2) {
                    feature_buffer_.pop_front();
                }
            }
            if (config_.enable_regime_updates && regime_detector_) {
                auto state = regime_detector_->on_bar(data);
                updated_regime = true;
                if (state.regime != current_regime_.regime) {
                    regime::RegimeTransition transition;
                    transition.from = current_regime_.regime;
                    transition.to = state.regime;
                    transition.timestamp = state.timestamp;
                    transition.confidence = state.confidence;
                    current_regime_ = state;
                    if (regime_cb_) {
                        regime_cb_(transition);
                    }
                    if (audit_logger_) {
                        audit_logger_->log_regime_change(transition);
                    }
                    if (strategy_) {
                        strategy_->on_regime_change(transition);
                    }
                } else {
                    current_regime_ = state;
                }
            }
            if (price > 0) {
                last_prices_[data.symbol] = price;
                portfolio_->mark_to_market(data.symbol, price, data.timestamp);
                portfolio_->record_snapshot(data.timestamp);
                snapshot_updated = true;
            }
            if (strategy_) {
                strategy_->on_bar(data);
            }
        } else if constexpr (std::is_same_v<T, data::Tick>) {
            price = data.price;
            snapshot_time = data.timestamp;
            if (config_.enable_regime_updates && regime_detector_) {
                auto state = regime_detector_->on_tick(data);
                updated_regime = true;
                if (state.regime != current_regime_.regime) {
                    regime::RegimeTransition transition;
                    transition.from = current_regime_.regime;
                    transition.to = state.regime;
                    transition.timestamp = state.timestamp;
                    transition.confidence = state.confidence;
                    current_regime_ = state;
                    if (regime_cb_) {
                        regime_cb_(transition);
                    }
                    if (audit_logger_) {
                        audit_logger_->log_regime_change(transition);
                    }
                    if (strategy_) {
                        strategy_->on_regime_change(transition);
                    }
                } else {
                    current_regime_ = state;
                }
            }
            if (price > 0) {
                last_prices_[data.symbol] = price;
                portfolio_->mark_to_market(data.symbol, price, data.timestamp);
                portfolio_->record_snapshot(data.timestamp);
                snapshot_updated = true;
            }
            if (strategy_) {
                strategy_->on_tick(data);
            }
        } else if constexpr (std::is_same_v<T, data::Quote>) {
            price = data.mid();
            snapshot_time = data.timestamp;
            if (price > 0) {
                last_prices_[data.symbol] = price;
                portfolio_->mark_to_market(data.symbol, price, data.timestamp);
                portfolio_->record_snapshot(data.timestamp);
                snapshot_updated = true;
            }
        } else if constexpr (std::is_same_v<T, data::OrderBook>) {
            price = (data.bids[0].price + data.asks[0].price) / 2.0;
            snapshot_time = data.timestamp;
            if (price > 0) {
                last_prices_[data.symbol] = price;
                portfolio_->mark_to_market(data.symbol, price, data.timestamp);
                portfolio_->record_snapshot(data.timestamp);
                snapshot_updated = true;
            }
        }
        (void)updated_regime;
    }, update.data);
    if (snapshot_updated) {
        update_dashboard_snapshot("market_data");
    }
}

void LiveTradingEngine::handle_execution_report(const ExecutionReport& report) {
    order_manager_->handle_execution_report(report);
    if (audit_logger_) {
        if (report.status == LiveOrderStatus::New) {
            AuditEvent event;
            event.timestamp = report.timestamp;
            event.type = AuditEvent::Type::OrderAcknowledged;
            event.details = "Order acknowledged";
            event.metadata["broker_order_id"] = report.broker_order_id;
            audit_logger_->log(event);
        }
        if (report.status == LiveOrderStatus::Filled) {
            AuditEvent event;
            event.timestamp = report.timestamp;
            event.type = AuditEvent::Type::OrderFilled;
            event.details = "Order filled";
            event.metadata["broker_order_id"] = report.broker_order_id;
            audit_logger_->log(event);
        } else if (report.status == LiveOrderStatus::Cancelled) {
            AuditEvent event;
            event.timestamp = report.timestamp;
            event.type = AuditEvent::Type::OrderCancelled;
            event.details = "Order cancelled";
            event.metadata["broker_order_id"] = report.broker_order_id;
            audit_logger_->log(event);
        } else if (report.status == LiveOrderStatus::Rejected) {
            AuditEvent event;
            event.timestamp = report.timestamp;
            event.type = AuditEvent::Type::OrderRejected;
            event.details = report.text;
            event.metadata["broker_order_id"] = report.broker_order_id;
            audit_logger_->log(event);
        }
    }
    if (report.status == LiveOrderStatus::PartiallyFilled ||
        report.status == LiveOrderStatus::Filled) {
        SymbolId symbol_id = SymbolRegistry::instance().intern(report.symbol);
        engine::Fill fill;
        {
            std::lock_guard<std::mutex> lock(broker_mutex_);
            auto it = broker_to_order_ids_.find(report.broker_order_id);
            if (it != broker_to_order_ids_.end()) {
                fill.order_id = it->second;
            }
        }
        if (fill.order_id == 0 && order_manager_) {
            auto internal_id = order_manager_->find_order_id_by_broker_id(report.broker_order_id);
            if (internal_id.has_value()) {
                fill.order_id = internal_id.value();
                std::lock_guard<std::mutex> lock(broker_mutex_);
                broker_to_order_ids_[report.broker_order_id] = fill.order_id;
            }
        }
        fill.symbol = symbol_id;
        fill.quantity = report.side == engine::OrderSide::Buy ? report.quantity : -report.quantity;
        fill.price = report.price;
        fill.commission = report.commission;
        fill.timestamp = report.timestamp;
        if (portfolio_) {
            portfolio_->update_position(fill);
            portfolio_->record_snapshot(report.timestamp);
        }
        strategy_order_manager_.process_fill(fill);
        if (report.status == LiveOrderStatus::Filled) {
            Trade trade;
            trade.symbol = report.symbol;
            trade.quantity = report.quantity;
            trade.price = report.price;
            trade.timestamp = report.timestamp;
            if (trade_cb_) {
                trade_cb_(trade);
            }
        }
    }
    update_dashboard_snapshot("execution_report");
}

void LiveTradingEngine::refresh_account_info() {
    if (!broker_) {
        return;
    }
    auto info = broker_->get_account_info();
    last_account_info_ = info;
    if (portfolio_) {
        portfolio_->set_cash(info.cash, Timestamp::now());
    }
    if (daily_start_equity_ <= 0.0 && info.equity > 0.0) {
        daily_start_equity_ = info.equity;
    }
    if (daily_start_equity_ > 0.0) {
        daily_pnl_ = info.equity - daily_start_equity_;
    }
    check_daily_loss_limit();
    if (portfolio_) {
        portfolio_->record_snapshot(Timestamp::now());
    }
    update_dashboard_snapshot("account_refresh");
}

void LiveTradingEngine::refresh_positions() {
    if (!broker_ || !portfolio_) {
        return;
    }
    auto positions = broker_->get_positions();
    apply_positions(positions, Timestamp::now());
}

void LiveTradingEngine::reconcile_orders() {
    if (!order_manager_) {
        return;
    }
    auto res = order_manager_->reconcile_with_broker();
    if (res.is_err()) {
        if (error_cb_) {
            error_cb_(res.error().to_string());
        }
        if (audit_logger_) {
            audit_logger_->log_error(res.error().to_string());
        }
    }
}

void LiveTradingEngine::apply_positions(const std::vector<Position>& positions,
                                        Timestamp timestamp) {
    if (!portfolio_) {
        return;
    }
    std::unordered_map<SymbolId, engine::Position> mapped;
    mapped.reserve(positions.size());
    for (const auto& position : positions) {
        if (position.symbol.empty() || position.quantity == 0.0) {
            continue;
        }
        SymbolId symbol_id = SymbolRegistry::instance().intern(position.symbol);
        engine::Position pos;
        pos.symbol = symbol_id;
        pos.quantity = position.quantity;
        pos.avg_cost = position.average_price;
        auto price_it = last_prices_.find(symbol_id);
        if (price_it != last_prices_.end()) {
            pos.current_price = price_it->second;
        } else if (position.market_value != 0.0) {
            pos.current_price = position.market_value / position.quantity;
        } else {
            pos.current_price = position.average_price;
        }
        pos.last_update = timestamp;
        mapped[symbol_id] = pos;
    }
    portfolio_->replace_positions(mapped, timestamp);
    enforce_portfolio_limits("position_reconcile");
    portfolio_->record_snapshot(timestamp);
    update_dashboard_snapshot("position_reconcile");
}

void LiveTradingEngine::apply_position_update(const Position& position, Timestamp timestamp) {
    if (!portfolio_) {
        return;
    }
    if (position.symbol.empty()) {
        return;
    }
    SymbolId symbol_id = SymbolRegistry::instance().intern(position.symbol);
    Price current_price = position.average_price;
    auto price_it = last_prices_.find(symbol_id);
    if (price_it != last_prices_.end()) {
        current_price = price_it->second;
    } else if (position.market_value != 0.0 && position.quantity != 0.0) {
        current_price = position.market_value / position.quantity;
    }
    portfolio_->set_position(symbol_id, position.quantity, position.average_price,
                             current_price, timestamp);
    enforce_portfolio_limits("position_update");
    portfolio_->record_snapshot(timestamp);
    update_dashboard_snapshot("position_update");
}

void LiveTradingEngine::check_daily_loss_limit() {
    if (!trading_enabled_) {
        return;
    }
    bool breached = false;
    if (config_.daily_loss_limit > 0.0 && daily_pnl_ <= -config_.daily_loss_limit) {
        breached = true;
    }
    if (!breached && config_.daily_loss_limit_pct > 0.0 && daily_start_equity_ > 0.0) {
        double pct = daily_pnl_ / daily_start_equity_;
        if (pct <= -config_.daily_loss_limit_pct) {
            breached = true;
        }
    }
    if (!breached) {
        return;
    }
    trading_enabled_ = false;
    if (order_manager_) {
        order_manager_->cancel_all_orders();
    }
    close_all_positions();
    if (audit_logger_) {
        AuditEvent event;
        event.timestamp = Timestamp::now();
        event.type = AuditEvent::Type::RiskLimitBreached;
        event.details = "Daily loss limit breached";
        audit_logger_->log(event);
    }
    if (error_cb_) {
        error_cb_("Daily loss limit breached");
    }
}

void LiveTradingEngine::enforce_portfolio_limits(const std::string& context) {
    if (!risk_manager_ || !portfolio_) {
        return;
    }
    auto res = risk_manager_->validate_portfolio(*portfolio_);
    if (res.is_ok()) {
        return;
    }
    trading_enabled_ = false;
    if (order_manager_) {
        order_manager_->cancel_all_orders();
    }
    close_all_positions();
    if (audit_logger_) {
        AuditEvent event;
        event.timestamp = Timestamp::now();
        event.type = AuditEvent::Type::RiskLimitBreached;
        event.details = res.error().to_string();
        event.metadata["context"] = context;
        audit_logger_->log(event);
    }
    if (error_cb_) {
        error_cb_(res.error().to_string());
    }
}

void LiveTradingEngine::update_dashboard_snapshot(const std::string& context) {
    auto snapshot = build_dashboard_snapshot(context);
    {
        std::lock_guard<std::mutex> lock(dashboard_mutex_);
        last_dashboard_ = snapshot;
    }
    if (dashboard_cb_) {
        dashboard_cb_(snapshot);
    }
}

LiveTradingEngine::DashboardSnapshot LiveTradingEngine::build_dashboard_snapshot(
    const std::string& context) {
    DashboardSnapshot snapshot;
    snapshot.timestamp = Timestamp::now();
    snapshot.current_regime = current_regime_;
    snapshot.daily_pnl = daily_pnl_;
    if (portfolio_) {
        snapshot.equity = portfolio_->equity();
        snapshot.cash = portfolio_->cash();
        snapshot.equity_curve = portfolio_->equity_curve();
        snapshot.positions = portfolio_->get_all_positions();
    }
    if (order_manager_) {
        auto orders = order_manager_->get_open_orders();
        snapshot.open_orders.reserve(orders.size());
        for (const auto& order : orders) {
            LiveOrderSummary summary;
            summary.id = order.internal_id;
            summary.symbol = order.symbol;
            summary.side = order.side;
            summary.type = order.type;
            summary.quantity = order.quantity;
            summary.filled_quantity = order.filled_quantity;
            summary.limit_price = order.limit_price;
            summary.stop_price = order.stop_price;
            summary.avg_fill_price = order.avg_fill_price;
            summary.status = status_name(order.status);
            summary.updated_at = best_order_timestamp(order);
            snapshot.open_orders.push_back(std::move(summary));
        }
    }
    drain_pending_alerts();
    {
        std::lock_guard<std::mutex> lock(alert_mutex_);
        snapshot.alerts = alerts_;
    }
    {
        std::lock_guard<std::mutex> lock(health_mutex_);
        snapshot.cpu_usage_pct = last_health_.cpu_usage_pct;
        snapshot.memory_mb = last_health_.memory_mb;
        snapshot.event_loop_latency_ms = last_health_.event_loop_latency_ms;
    }
    (void)context;
    return snapshot;
}

void LiveTradingEngine::add_alert(const std::string& message) {
    pending_alerts_.push(message);
}

void LiveTradingEngine::drain_pending_alerts() {
    std::vector<std::string> pending;
    std::string message;
    while (pending_alerts_.pop(message)) {
        pending.push_back(std::move(message));
        message.clear();
    }
    if (pending.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(alert_mutex_);
    for (const auto& entry : pending) {
        if (!alerts_.empty() && alerts_.back() == entry) {
            continue;
        }
        alerts_.push_back(entry);
        if (alerts_.size() > 50) {
            alerts_.erase(alerts_.begin(), alerts_.begin() + (alerts_.size() - 50));
        }
    }
}

void LiveTradingEngine::sample_system_health() {
    Timestamp now = Timestamp::now();
    if (last_health_sample_.microseconds() != 0) {
        if ((now - last_health_sample_).total_microseconds()
            < Duration::seconds(1).total_microseconds()) {
            return;
        }
    }
    last_health_sample_ = now;
    double cpu_usage = 0.0;
    double memory_mb = 0.0;

#if defined(__linux__)
    {
        std::ifstream stat("/proc/stat");
        std::string cpu;
        uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0;
        if (stat >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq) {
            uint64_t total = user + nice + system + idle + iowait + irq + softirq;
            uint64_t idle_all = idle + iowait;
            if (prev_cpu_total_ != 0) {
                uint64_t total_delta = total - prev_cpu_total_;
                uint64_t idle_delta = idle_all - prev_cpu_idle_;
                if (total_delta > 0) {
                    cpu_usage = 100.0 * (static_cast<double>(total_delta - idle_delta) /
                                         static_cast<double>(total_delta));
                }
            }
            prev_cpu_total_ = total;
            prev_cpu_idle_ = idle_all;
        }
    }
    {
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.rfind("VmRSS:", 0) == 0) {
                std::istringstream iss(line);
                std::string label;
                double kb = 0.0;
                iss >> label >> kb;
                memory_mb = kb / 1024.0;
                break;
            }
        }
    }
#elif defined(_WIN32)
    {
        FILETIME idle_time {}, kernel_time {}, user_time {};
        if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
            ULARGE_INTEGER idle {};
            ULARGE_INTEGER kernel {};
            ULARGE_INTEGER user {};
            idle.LowPart = idle_time.dwLowDateTime;
            idle.HighPart = idle_time.dwHighDateTime;
            kernel.LowPart = kernel_time.dwLowDateTime;
            kernel.HighPart = kernel_time.dwHighDateTime;
            user.LowPart = user_time.dwLowDateTime;
            user.HighPart = user_time.dwHighDateTime;
            uint64_t total = kernel.QuadPart + user.QuadPart;
            uint64_t idle_all = idle.QuadPart;
            if (prev_cpu_total_ != 0) {
                uint64_t total_delta = total - prev_cpu_total_;
                uint64_t idle_delta = idle_all - prev_cpu_idle_;
                if (total_delta > 0) {
                    cpu_usage = 100.0 *
                                (static_cast<double>(total_delta - idle_delta) /
                                 static_cast<double>(total_delta));
                }
            }
            prev_cpu_total_ = total;
            prev_cpu_idle_ = idle_all;
        }
    }
    {
        PROCESS_MEMORY_COUNTERS_EX pmc {};
        if (GetProcessMemoryInfo(GetCurrentProcess(),
                                 reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                                 sizeof(pmc))) {
            memory_mb = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
        }
    }
#elif defined(__APPLE__)
    {
        host_cpu_load_info_data_t cpuinfo {};
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                            reinterpret_cast<host_info_t>(&cpuinfo), &count) == KERN_SUCCESS) {
            uint64_t user = cpuinfo.cpu_ticks[CPU_STATE_USER];
            uint64_t system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
            uint64_t idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
            uint64_t nice = cpuinfo.cpu_ticks[CPU_STATE_NICE];
            uint64_t total = user + system + idle + nice;
            uint64_t idle_all = idle;
            if (prev_cpu_total_ != 0) {
                uint64_t total_delta = total - prev_cpu_total_;
                uint64_t idle_delta = idle_all - prev_cpu_idle_;
                if (total_delta > 0) {
                    cpu_usage = 100.0 *
                                (static_cast<double>(total_delta - idle_delta) /
                                 static_cast<double>(total_delta));
                }
            }
            prev_cpu_total_ = total;
            prev_cpu_idle_ = idle_all;
        }
    }
    {
        mach_task_basic_info_data_t info {};
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                      reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
            memory_mb = static_cast<double>(info.resident_size) / (1024.0 * 1024.0);
        }
    }
#endif

    double loop_latency_ms = 0.0;
    if (last_event_loop_tick_.microseconds() != 0) {
        loop_latency_ms = (now - last_event_loop_tick_).total_microseconds() / 1000.0;
    }

    std::lock_guard<std::mutex> lock(health_mutex_);
    last_health_.cpu_usage_pct = cpu_usage;
    last_health_.memory_mb = memory_mb;
    last_health_.event_loop_latency_ms = loop_latency_ms;
    last_health_.last_sample = now;
    last_health_.last_market_data = last_market_data_;
    last_health_.last_reconnect_attempt = last_reconnect_attempt_;
    last_health_.last_reconnect_success = last_reconnect_success_;
}

void LiveTradingEngine::check_heartbeat() {
    if (config_.heartbeat_timeout.total_microseconds() <= 0) {
        return;
    }
    Timestamp now = Timestamp::now();
    if ((now - last_market_data_).total_microseconds()
        < config_.heartbeat_timeout.total_microseconds()) {
        return;
    }
    if (heartbeat_alerted_) {
        return;
    }
    heartbeat_alerted_ = true;
    add_alert("Heartbeat timeout: no market data");
    if (audit_logger_) {
        audit_logger_->log_error("Heartbeat timeout: no market data");
    }
    if (error_cb_) {
        error_cb_("Heartbeat timeout: no market data");
    }
    if (config_.enable_auto_reconnect && broker_) {
        broker_->disconnect();
    }
}

void LiveTradingEngine::attempt_reconnect() {
    if (!config_.enable_auto_reconnect || !broker_) {
        return;
    }
    if (broker_->is_connected()) {
        return;
    }
    Timestamp now = Timestamp::now();
    last_reconnect_attempt_ = now;
    if (next_reconnect_attempt_.microseconds() != 0 &&
        now < next_reconnect_attempt_) {
        return;
    }
    if (config_.reconnect_max_attempts > 0 &&
        reconnect_attempts_ >= config_.reconnect_max_attempts) {
        return;
    }
    auto res = broker_->connect();
    if (res.is_ok()) {
        last_reconnect_success_ = Timestamp::now();
        reconnect_attempts_ = 0;
        reconnect_backoff_ms_ = 0;
        next_reconnect_attempt_ = Timestamp();
        heartbeat_alerted_ = false;
        add_alert("Reconnected to broker");
        if (!config_.symbols.empty()) {
            broker_->subscribe_market_data(config_.symbols);
        }
        refresh_account_info();
        refresh_positions();
        update_dashboard_snapshot("reconnect");
        return;
    }
    reconnect_attempts_ += 1;
    if (reconnect_backoff_ms_ <= 0) {
        reconnect_backoff_ms_ = config_.reconnect_initial.total_milliseconds();
    } else {
        reconnect_backoff_ms_ = std::min(reconnect_backoff_ms_ * 2,
                                         config_.reconnect_max.total_milliseconds());
    }
    if (reconnect_backoff_ms_ <= 0) {
        reconnect_backoff_ms_ = 1000;
    }
    next_reconnect_attempt_ = Timestamp::now()
        + Duration::milliseconds(reconnect_backoff_ms_);
    add_alert("Reconnect failed: " + res.error().to_string());
    if (audit_logger_) {
        audit_logger_->log_error(res.error().to_string());
    }
    if (error_cb_) {
        error_cb_(res.error().to_string());
    }
}

}  // namespace regimeflow::live
