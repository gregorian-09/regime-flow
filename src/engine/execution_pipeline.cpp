#include "regimeflow/engine/execution_pipeline.h"

#include <cmath>
#include <cstdlib>
#include <vector>

namespace regimeflow::engine
{
    namespace {
        constexpr double kQuantityEpsilon = 1e-9;

        int minute_of_day_utc(const Timestamp timestamp) {
            const int hour = std::stoi(timestamp.to_string("%H"));
            const int minute = std::stoi(timestamp.to_string("%M"));
            return hour * 60 + minute;
        }

        int weekday_utc(const Timestamp timestamp) {
            return std::stoi(timestamp.to_string("%w"));
        }

        std::string day_stamp_utc(const Timestamp timestamp) {
            return timestamp.to_string("%Y-%m-%d");
        }

        bool minute_in_window(const int minute, const int start_minute, const int end_minute) {
            if (start_minute == end_minute) {
                return true;
            }
            if (start_minute < end_minute) {
                return minute >= start_minute && minute <= end_minute;
            }
            return minute >= start_minute || minute <= end_minute;
        }

        std::optional<double> metadata_double(const Order& order, const std::string& key) {
            if (const auto it = order.metadata.find(key); it != order.metadata.end()) {
                char* end = nullptr;
                const double value = std::strtod(it->second.c_str(), &end);
                if (end != it->second.c_str()) {
                    return value;
                }
            }
            return std::nullopt;
        }

        std::optional<int64_t> metadata_int64(const Order& order, const std::string& key) {
            if (const auto it = order.metadata.find(key); it != order.metadata.end()) {
                char* end = nullptr;
                const auto value = std::strtoll(it->second.c_str(), &end, 10);
                if (end != it->second.c_str()) {
                    return static_cast<int64_t>(value);
                }
            }
            return std::nullopt;
        }

        std::optional<bool> metadata_bool(const Order& order, const std::string& key) {
            if (const auto it = order.metadata.find(key); it != order.metadata.end()) {
                if (it->second == "true" || it->second == "1") {
                    return true;
                }
                if (it->second == "false" || it->second == "0") {
                    return false;
                }
            }
            return std::nullopt;
        }

        std::optional<std::string> metadata_string(const Order& order, const std::string& key) {
            if (const auto it = order.metadata.find(key); it != order.metadata.end()) {
                return it->second;
            }
            return std::nullopt;
        }
    }

    ExecutionPipeline::ExecutionPipeline(MarketDataCache* market_data,
                                         OrderBookCache* order_books,
                                         events::EventQueue* event_queue)
        : market_data_(market_data),
          order_books_(order_books),
          event_queue_(event_queue) {
        auto slippage = std::make_shared<execution::ZeroSlippageModel>();
        execution_model_ = std::make_unique<execution::BasicExecutionModel>(slippage);
        commission_model_ = std::make_unique<execution::ZeroCommissionModel>();
        transaction_cost_model_ = std::make_unique<execution::ZeroTransactionCostModel>();
        market_impact_model_ = std::make_unique<execution::ZeroMarketImpactModel>();
        latency_model_ = std::make_unique<execution::FixedLatencyModel>(Duration::milliseconds(0));
    }

    void ExecutionPipeline::set_execution_model(std::unique_ptr<execution::ExecutionModel> model) {
        execution_model_ = std::move(model);
    }

    void ExecutionPipeline::set_commission_model(std::unique_ptr<execution::CommissionModel> model) {
        commission_model_ = std::move(model);
    }

    void ExecutionPipeline::set_transaction_cost_model(std::unique_ptr<execution::TransactionCostModel> model) {
        transaction_cost_model_ = std::move(model);
    }

    void ExecutionPipeline::set_market_impact_model(std::unique_ptr<execution::MarketImpactModel> model) {
        market_impact_model_ = std::move(model);
    }

    void ExecutionPipeline::set_latency_model(std::unique_ptr<execution::LatencyModel> model) {
        latency_model_ = std::move(model);
    }

    void ExecutionPipeline::set_default_fill_policy(const FillPolicy policy) {
        default_fill_policy_ = policy;
    }

    void ExecutionPipeline::set_price_drift_rule(const double max_deviation_bps,
                                                 const PriceDriftAction action) {
        max_deviation_bps_ = std::max(0.0, max_deviation_bps);
        price_drift_action_ = action;
    }

    void ExecutionPipeline::set_queue_model(const bool enabled,
                                            const double progress_fraction,
                                            const double default_visible_qty,
                                            const QueueDepthMode mode,
                                            const double aging_fraction,
                                            const double replenishment_fraction) {
        queue_model_enabled_ = enabled;
        queue_progress_fraction_ = std::max(0.0, progress_fraction);
        queue_default_visible_qty_ = std::max(0.0, default_visible_qty);
        queue_aging_fraction_ = std::max(0.0, aging_fraction);
        queue_replenishment_fraction_ = std::max(0.0, replenishment_fraction);
        queue_depth_mode_ = mode;
    }

    void ExecutionPipeline::set_bar_simulation_mode(const BarSimulationMode mode) {
        bar_simulation_mode_ = mode;
    }

    void ExecutionPipeline::set_session_policy(SessionPolicy policy) {
        policy.open_auction_minutes = std::max(0, policy.open_auction_minutes);
        policy.close_auction_minutes = std::max(0, policy.close_auction_minutes);
        session_policy_ = std::move(policy);
    }

    void ExecutionPipeline::set_symbol_halt(const SymbolId symbol, const bool halted) {
        if (symbol == 0) {
            return;
        }
        if (halted) {
            session_policy_.dynamic_halted_symbols.insert(symbol);
            return;
        }
        session_policy_.dynamic_halted_symbols.erase(symbol);
    }

    void ExecutionPipeline::set_global_halt(const bool halted) {
        session_policy_.dynamic_halt_all = halted;
    }

    TimeInForce ExecutionPipeline::effective_tif_for(const Order& order) const {
        if (order.tif == TimeInForce::IOC || order.tif == TimeInForce::FOK || order.tif == TimeInForce::GTD) {
            return order.tif;
        }

        switch (default_fill_policy_) {
        case FillPolicy::IOC:
            return TimeInForce::IOC;
        case FillPolicy::FOK:
            return TimeInForce::FOK;
        case FillPolicy::Return:
        case FillPolicy::PreserveOrder:
            return order.tif;
        }
        return order.tif;
    }

    bool ExecutionPipeline::queue_enabled_for(const Order& order) const {
        if (const auto override = metadata_bool(order, "venue_queue_enabled")) {
            return *override;
        }
        return queue_model_enabled_;
    }

    double ExecutionPipeline::queue_progress_fraction_for(const Order& order) const {
        if (const auto override = metadata_double(order, "venue_queue_progress_fraction")) {
            return std::max(0.0, *override);
        }
        return queue_progress_fraction_;
    }

    double ExecutionPipeline::queue_default_visible_qty_for(const Order& order) const {
        if (const auto override = metadata_double(order, "venue_queue_default_visible_qty")) {
            return std::max(0.0, *override);
        }
        return queue_default_visible_qty_;
    }

    ExecutionPipeline::QueueDepthMode ExecutionPipeline::queue_depth_mode_for(const Order& order) const {
        if (const auto override = metadata_string(order, "venue_queue_depth_mode")) {
            if (*override == "price_level") {
                return QueueDepthMode::PriceLevel;
            }
            if (*override == "full_depth") {
                return QueueDepthMode::FullDepth;
            }
        }
        return queue_depth_mode_;
    }

    Quantity ExecutionPipeline::visible_queue_quantity(const Order& order) const {
        const auto depth_mode = queue_depth_mode_for(order);
        const double default_visible_qty = queue_default_visible_qty_for(order);
        const auto accumulate_book_side = [&](const auto& side_levels,
                                             const bool better_is_greater) -> Quantity {
            Quantity total = 0.0;
            Quantity level_match = 0.0;
            for (const auto& level : side_levels) {
                if (level.quantity <= 0.0 || level.price <= 0.0) {
                    continue;
                }
                const double delta = level.price - order.limit_price;
                const bool same_level = std::abs(delta) <= kQuantityEpsilon;
                const bool better_level = better_is_greater ? (delta > kQuantityEpsilon)
                                                            : (delta < -kQuantityEpsilon);
                if (depth_mode == QueueDepthMode::TopOnly) {
                    if (&level != &side_levels.front()) {
                        break;
                    }
                    if (same_level) {
                        return level.quantity;
                    }
                    return default_visible_qty;
                }
                if (depth_mode == QueueDepthMode::PriceLevel) {
                    if (same_level) {
                        level_match = level.quantity;
                        break;
                    }
                    continue;
                }
                if (better_level || same_level) {
                    total += level.quantity;
                }
            }
            if (depth_mode == QueueDepthMode::PriceLevel) {
                return level_match > 0.0 ? level_match : default_visible_qty;
            }
            return total > 0.0 ? total : default_visible_qty;
        };

        if (order.side == OrderSide::Buy) {
            if (order_books_) {
                if (const auto book = order_books_->latest(order.symbol);
                    book && !book->bids.empty()) {
                    return accumulate_book_side(book->bids, true);
                }
            }
            if (market_data_) {
                if (const auto quote = market_data_->latest_quote(order.symbol);
                    quote && std::abs(quote->bid - order.limit_price) <= kQuantityEpsilon
                    && quote->bid_size > 0.0) {
                    return quote->bid_size;
                }
            }
        } else {
            if (order_books_) {
                if (const auto book = order_books_->latest(order.symbol);
                    book && !book->asks.empty()) {
                    return accumulate_book_side(book->asks, false);
                }
            }
            if (market_data_) {
                if (const auto quote = market_data_->latest_quote(order.symbol);
                    quote && std::abs(quote->ask - order.limit_price) <= kQuantityEpsilon
                    && quote->ask_size > 0.0) {
                    return quote->ask_size;
                }
            }
        }
        return default_visible_qty;
    }

    bool ExecutionPipeline::is_touch_fill_candidate(const RestingOrderState& state,
                                                    const EvaluationContext context) const {
        const Order& order = state.order;
        const OrderType effective_type =
            state.stop_triggered && order.type == OrderType::Stop ? OrderType::Market
            : state.stop_triggered && order.type == OrderType::StopLimit ? OrderType::Limit
            : order.type;
        if (effective_type != OrderType::Limit) {
            return false;
        }
        const Price current_price = executable_price(order, context);
        return current_price > 0.0
            && std::abs(current_price - order.limit_price) <= kQuantityEpsilon;
    }

    bool ExecutionPipeline::is_price_through_limit(const RestingOrderState& state,
                                                   const EvaluationContext context) const {
        const Order& order = state.order;
        const OrderType effective_type =
            state.stop_triggered && order.type == OrderType::Stop ? OrderType::Market
            : state.stop_triggered && order.type == OrderType::StopLimit ? OrderType::Limit
            : order.type;
        if (effective_type != OrderType::Limit) {
            return false;
        }
        const Price current_price = executable_price(order, context);
        if (current_price <= 0.0) {
            return false;
        }
        if (order.side == OrderSide::Buy) {
            return current_price + kQuantityEpsilon < order.limit_price;
        }
        return current_price - kQuantityEpsilon > order.limit_price;
    }

    bool ExecutionPipeline::advance_queue(RestingOrderState& state,
                                          const EvaluationContext context) const {
        if (!queue_enabled_for(state.order) || !state.was_resting || !is_touch_fill_candidate(state, context)) {
            return true;
        }
        if (!state.queue_initialized) {
            state.queue_ahead = visible_queue_quantity(state.order);
            state.queue_initialized = true;
            state.last_visible_queue = state.queue_ahead;
        }
        if (state.queue_ahead <= kQuantityEpsilon) {
            return true;
        }

        double visible = visible_queue_quantity(state.order);
        if (visible <= kQuantityEpsilon) {
            visible = queue_default_visible_qty_for(state.order);
        }
        double progress = visible * queue_progress_fraction_for(state.order);
        if (progress <= kQuantityEpsilon) {
            progress = visible;
        }
        state.queue_ahead = std::max(0.0, state.queue_ahead - progress);
        state.last_visible_queue = visible;
        return state.queue_ahead <= kQuantityEpsilon;
    }

    void ExecutionPipeline::refresh_queue_state(RestingOrderState& state,
                                                const EvaluationContext context) const {
        if (!queue_enabled_for(state.order) || !state.was_resting) {
            return;
        }
        if (!is_touch_fill_candidate(state, context)) {
            if (!state.queue_initialized || queue_aging_fraction_ <= kQuantityEpsilon
                || is_price_through_limit(state, context)) {
                return;
            }
            double visible = visible_queue_quantity(state.order);
            if (visible <= kQuantityEpsilon) {
                visible = queue_default_visible_qty_for(state.order);
            }
            const double aging = visible * queue_aging_fraction_;
            if (aging > kQuantityEpsilon) {
                state.queue_ahead = std::max(0.0, state.queue_ahead - aging);
                state.last_visible_queue = visible;
            }
            return;
        }

        if (!state.queue_initialized) {
            state.queue_ahead = visible_queue_quantity(state.order);
            state.queue_initialized = true;
            state.last_visible_queue = state.queue_ahead;
            return;
        }

        if (queue_replenishment_fraction_ <= kQuantityEpsilon) {
            return;
        }
        double visible = visible_queue_quantity(state.order);
        if (visible <= kQuantityEpsilon) {
            visible = queue_default_visible_qty_for(state.order);
        }
        const double added_visible = std::max(0.0, visible - state.last_visible_queue);
        if (added_visible <= kQuantityEpsilon) {
            state.last_visible_queue = visible;
            return;
        }
        state.queue_ahead += added_visible * queue_replenishment_fraction_;
        state.last_visible_queue = visible;
    }

    bool ExecutionPipeline::session_allows_execution(const Order& order, const Timestamp timestamp) const {
        if (session_policy_.halt_all || session_policy_.halted_symbols.contains(order.symbol)
            || session_policy_.dynamic_halt_all
            || session_policy_.dynamic_halted_symbols.contains(order.symbol)) {
            return false;
        }
        if (!session_policy_.enabled) {
            return true;
        }
        if (!session_policy_.allowed_weekdays.empty()
            && !session_policy_.allowed_weekdays.contains(weekday_utc(timestamp))) {
            return false;
        }
        if (session_policy_.closed_dates.contains(day_stamp_utc(timestamp))) {
            return false;
        }
        const int minute = minute_of_day_utc(timestamp);
        if (!minute_in_window(minute, session_policy_.start_minute_utc, session_policy_.end_minute_utc)) {
            return false;
        }
        if (order.type == OrderType::MarketOnOpen) {
            const int auction_end = session_policy_.start_minute_utc + session_policy_.open_auction_minutes;
            return minute >= session_policy_.start_minute_utc && minute <= auction_end;
        }
        if (order.type == OrderType::MarketOnClose) {
            const int auction_start = session_policy_.end_minute_utc - session_policy_.close_auction_minutes;
            return minute >= auction_start && minute <= session_policy_.end_minute_utc;
        }
        return true;
    }

    void ExecutionPipeline::on_order_submitted(const Order& order)
    {
        if (!execution_model_ || !event_queue_) {
            return;
        }
        if (order.status == OrderStatus::Rejected || order.status == OrderStatus::Cancelled
            || order.status == OrderStatus::Filled) {
            resting_orders_.erase(order.id);
            return;
        }

        RestingOrderState state;
        state.order = order;
        state.effective_tif = effective_tif_for(order);
        const Price submit_price = executable_price(order);
        if (submit_price > 0.0) {
            state.requested_price = submit_price;
            state.has_requested_price = true;
        }
        const Timestamp submitted_at =
            order.created_at.microseconds() == 0 ? Timestamp::now() : order.created_at;
        Timestamp ts = submitted_at;
        if (const auto override_ms = metadata_int64(order, "venue_latency_ms")) {
            ts = ts + Duration::milliseconds(std::max<int64_t>(0, *override_ms));
        } else if (latency_model_) {
            ts = ts + latency_model_->latency();
        }
        state.activation_time = ts;
        state.was_resting = !can_fill_now(state);

        if (ts > submitted_at) {
            resting_orders_[order.id] = state;
            return;
        }
        if (!session_allows_execution(order, ts)) {
            resting_orders_[order.id] = state;
            return;
        }

        if (order.type == OrderType::Stop || order.type == OrderType::StopLimit) {
            if (!should_trigger_stop(state)) {
                if (state.effective_tif == TimeInForce::IOC) {
                    const auto cancel = events::make_order_event(
                        events::OrderEventKind::Cancel,
                        ts,
                        order.id,
                        0,
                        0.0,
                        0.0,
                        order.symbol);
                    event_queue_->push(cancel);
                    return;
                }
                if (state.effective_tif == TimeInForce::FOK) {
                    const auto reject = events::make_order_event(
                        events::OrderEventKind::Reject,
                        ts,
                        order.id,
                        0,
                        0.0,
                        0.0,
                        order.symbol);
                    event_queue_->push(reject);
                    return;
                }
                state.was_resting = true;
                resting_orders_[order.id] = state;
                return;
            }
            state.stop_triggered = true;
        }

        if (!can_fill_now(state)) {
            if (state.effective_tif == TimeInForce::IOC) {
                const auto cancel = events::make_order_event(
                    events::OrderEventKind::Cancel,
                    ts,
                    order.id,
                    0,
                    0.0,
                    0.0,
                    order.symbol);
                event_queue_->push(cancel);
                return;
            }
            if (state.effective_tif == TimeInForce::FOK) {
                const auto reject = events::make_order_event(
                    events::OrderEventKind::Reject,
                    ts,
                    order.id,
                    0,
                    0.0,
                    0.0,
                    order.symbol);
                event_queue_->push(reject);
                return;
            }
            state.was_resting = true;
            resting_orders_[order.id] = state;
            return;
        }

        resting_orders_[order.id] = state;
        process_resting_order(order.id, ts);
    }

    void ExecutionPipeline::on_order_update(const Order& order) {
        if (order.status == OrderStatus::Cancelled || order.status == OrderStatus::Rejected
            || order.status == OrderStatus::Filled) {
            resting_orders_.erase(order.id);
        }
    }

    void ExecutionPipeline::on_market_update(const SymbolId symbol, const Timestamp timestamp) {
        std::vector<OrderId> to_process;
        to_process.reserve(resting_orders_.size());
        for (const auto& [id, state] : resting_orders_) {
            if (state.order.symbol == symbol) {
                to_process.push_back(id);
            }
        }
        for (const auto id : to_process) {
            process_resting_order(id, timestamp);
        }
    }

    void ExecutionPipeline::on_bar(const data::Bar& bar) {
        if (bar_simulation_mode_ == BarSimulationMode::CloseOnly) {
            on_market_update(bar.symbol, bar.timestamp);
            return;
        }

        std::vector<OrderId> to_process;
        to_process.reserve(resting_orders_.size());
        for (const auto& [id, state] : resting_orders_) {
            if (state.order.symbol == bar.symbol) {
                to_process.push_back(id);
            }
        }
        if (to_process.empty()) {
            return;
        }

        std::vector<Price> price_path;
        price_path.reserve(4);
        const auto append_price = [&price_path](const Price price) {
            if (price <= 0.0) {
                return;
            }
            if (!price_path.empty() && std::abs(price_path.back() - price) <= kQuantityEpsilon) {
                return;
            }
            price_path.emplace_back(price);
        };

        if (bar_simulation_mode_ == BarSimulationMode::OpenOnly) {
            append_price(bar.open);
        } else {
            append_price(bar.open);
            if (bar.close >= bar.open) {
                append_price(bar.low);
                append_price(bar.high);
            } else {
                append_price(bar.high);
                append_price(bar.low);
            }
            append_price(bar.close);
        }

        for (const auto synthetic_price : price_path) {
            EvaluationContext context;
            context.executable_price_override = synthetic_price;
            context.has_price_override = true;
            context.use_order_book = false;
            for (const auto id : to_process) {
                process_resting_order(id, bar.timestamp, context);
            }
        }
    }

    std::vector<Fill> ExecutionPipeline::simulate_immediate_fills(const Order& order,
                                                                  const Price reference_price,
                                                                  const Timestamp timestamp,
                                                                  const bool is_maker) const {
        if (reference_price <= 0.0 || order.symbol == 0 || order.quantity <= 0.0) {
            return {};
        }

        Timestamp execution_ts = timestamp;
        if (const auto override_ms = metadata_int64(order, "venue_latency_ms")) {
            execution_ts = execution_ts + Duration::milliseconds(std::max<int64_t>(0, *override_ms));
        } else if (latency_model_) {
            execution_ts = execution_ts + latency_model_->latency();
        }

        std::vector<Fill> fills;
        if (execution_model_) {
            fills = execution_model_->execute(order, reference_price, execution_ts);
        }
        if (fills.empty()) {
            Fill fill;
            fill.order_id = order.id;
            fill.symbol = order.symbol;
            fill.quantity = order.quantity * (order.side == OrderSide::Buy ? 1.0 : -1.0);
            fill.price = reference_price;
            fill.timestamp = execution_ts;
            fill.is_maker = is_maker;
            fills.emplace_back(fill);
        }
        return enrich_fills(order, fills, is_maker);
    }

    std::vector<Fill> ExecutionPipeline::enrich_fills(const Order& order,
                                                      const std::vector<Fill>& fills,
                                                      const bool is_maker) const {
        const data::OrderBook* book_ptr = nullptr;
        std::optional<data::OrderBook> book_holder;
        if (order_books_) {
            book_holder = order_books_->latest(order.symbol);
            if (book_holder.has_value()) {
                book_ptr = &book_holder.value();
            }
        }

        std::vector<Fill> enriched_fills;
        enriched_fills.reserve(fills.size());
        for (auto fill : fills) {
            fill.is_maker = is_maker;
            fill.parent_order_id = order.parent_id;
            if (const auto it = order.metadata.find("venue"); it != order.metadata.end()) {
                fill.venue = it->second;
            }
            if (market_impact_model_) {
                const double impact = market_impact_model_->impact_bps(order, book_ptr) / 10000.0;
                fill.price *= (order.side == OrderSide::Buy) ? (1.0 + impact)
                                                             : (1.0 - impact);
            }
            if (commission_model_) {
                fill.commission = commission_model_->commission(order, fill);
            }
            if (transaction_cost_model_) {
                fill.transaction_cost = transaction_cost_model_->cost(order, fill);
            }
            enriched_fills.emplace_back(std::move(fill));
        }
        return enriched_fills;
    }

    bool ExecutionPipeline::can_fill_now(const RestingOrderState& state,
                                         const EvaluationContext context) const {
        const Order& order = state.order;
        const OrderType effective_type =
            state.stop_triggered && order.type == OrderType::Stop ? OrderType::Market
            : state.stop_triggered && order.type == OrderType::StopLimit ? OrderType::Limit
            : order.type;

        if (effective_type == OrderType::Market || effective_type == OrderType::MarketOnClose
            || effective_type == OrderType::MarketOnOpen) {
            return executable_price(order, context) > 0.0;
        }

        if (effective_type != OrderType::Limit) {
            return false;
        }

        const Price current_price = executable_price(order, context);
        if (current_price <= 0.0) {
            return false;
        }
        if (order.side == OrderSide::Buy) {
            return current_price <= order.limit_price;
        }
        return current_price >= order.limit_price;
    }

    bool ExecutionPipeline::should_trigger_stop(const RestingOrderState& state,
                                                const EvaluationContext context) const {
        const Order& order = state.order;
        const Price current_price = executable_price(order, context);
        if (current_price <= 0.0 || order.stop_price <= 0.0) {
            return false;
        }
        if (order.side == OrderSide::Buy) {
            return current_price >= order.stop_price;
        }
        return current_price <= order.stop_price;
    }

    Price ExecutionPipeline::executable_price(const Order& order, const EvaluationContext context) const {
        Price price = 0.0;
        if (context.has_price_override && context.executable_price_override > 0.0) {
            price = context.executable_price_override;
        } else if (context.use_order_book && order_books_) {
            if (const auto book = order_books_->latest(order.symbol)) {
                if (order.side == OrderSide::Buy && !book->asks.empty()) {
                    price = book->asks.front().price;
                } else if (order.side == OrderSide::Sell && !book->bids.empty()) {
                    price = book->bids.front().price;
                }
            }
        }
        if (price <= 0.0 && market_data_) {
            if (const auto quote = market_data_->latest_quote(order.symbol)) {
                if (order.side == OrderSide::Buy && quote->ask > 0.0) {
                    price = quote->ask;
                } else if (order.side == OrderSide::Sell && quote->bid > 0.0) {
                    price = quote->bid;
                } else if (quote->mid() > 0.0) {
                    price = quote->mid();
                }
            }
        }
        if (price <= 0.0 && market_data_) {
            if (const auto tick = market_data_->latest_tick(order.symbol)) {
                if (tick->price > 0.0) {
                    price = tick->price;
                }
            }
        }
        if (price <= 0.0 && market_data_) {
            if (const auto bar = market_data_->latest_bar(order.symbol)) {
                if (bar->close > 0.0) {
                    price = bar->close;
                }
            }
        }
        if (price <= 0.0) {
            return 0.0;
        }
        if (const auto adjustment_bps = metadata_double(order, "venue_price_adjustment_bps")) {
            const double factor = *adjustment_bps / 10000.0;
            price *= order.side == OrderSide::Buy ? (1.0 + factor) : (1.0 - factor);
        }
        return price;
    }

    std::vector<Fill> ExecutionPipeline::build_fills(const RestingOrderState& state,
                                                     const Timestamp timestamp,
                                                     const EvaluationContext context) const {
        Order executable_order = state.order;
        if (state.stop_triggered) {
            if (executable_order.type == OrderType::Stop) {
                executable_order.type = OrderType::Market;
            } else if (executable_order.type == OrderType::StopLimit) {
                executable_order.type = OrderType::Limit;
            }
        }

        const Price ref_price = reference_price(executable_order, context);
        if (context.use_order_book && order_books_) {
            if (auto book = order_books_->latest(executable_order.symbol)) {
                auto model = execution::OrderBookExecutionModel(
                    std::make_shared<data::OrderBook>(*book));
                return model.execute(executable_order, ref_price, timestamp);
            }
        }
        return execution_model_->execute(executable_order, ref_price, timestamp);
    }

    void ExecutionPipeline::process_resting_order(const OrderId id,
                                                  Timestamp timestamp,
                                                  const EvaluationContext context) {
        const auto it = resting_orders_.find(id);
        if (it == resting_orders_.end()) {
            return;
        }

        RestingOrderState state = it->second;
        if (state.activation_time.microseconds() > 0 && timestamp < state.activation_time) {
            resting_orders_[id] = state;
            return;
        }
        refresh_queue_state(state, context);
        if (!session_allows_execution(state.order, timestamp)) {
            resting_orders_[id] = state;
            return;
        }
        if ((state.order.type == OrderType::Stop || state.order.type == OrderType::StopLimit)
            && !state.stop_triggered) {
            if (!should_trigger_stop(state, context)) {
                resting_orders_[id] = state;
                return;
            }
            state.stop_triggered = true;
        }

        if (!can_fill_now(state, context)) {
            resting_orders_[id] = state;
            return;
        }

        const Price current_price = executable_price(state.order, context);
        if (max_deviation_bps_ > 0.0 && price_drift_action_ != PriceDriftAction::Ignore
            && state.has_requested_price && state.requested_price > 0.0 && current_price > 0.0) {
            const double tolerance = max_deviation_bps_ / 10000.0;
            bool exceeds = false;
            if (state.order.side == OrderSide::Buy) {
                exceeds = current_price > state.requested_price * (1.0 + tolerance);
            } else {
                exceeds = current_price < state.requested_price * (1.0 - tolerance);
            }
            if (exceeds) {
                if (price_drift_action_ == PriceDriftAction::Reject) {
                    resting_orders_.erase(id);
                    const auto reject = events::make_order_event(
                        events::OrderEventKind::Reject,
                        timestamp,
                        state.order.id,
                        0,
                        0.0,
                        0.0,
                        state.order.symbol);
                    event_queue_->push(reject);
                    return;
                }

                state.requested_price = current_price;
                state.has_requested_price = true;
                resting_orders_[id] = state;
                const auto update = events::make_order_event(
                    events::OrderEventKind::Update,
                    timestamp,
                    state.order.id,
                    0,
                    0.0,
                    current_price,
                    state.order.symbol);
                event_queue_->push(update);
                return;
            }
        }

        bool maker_fill = false;
        if (queue_enabled_for(state.order) && state.was_resting && is_touch_fill_candidate(state, context)) {
            if (!advance_queue(state, context)) {
                resting_orders_[id] = state;
                return;
            }
            maker_fill = true;
        }

        auto fills = build_fills(state, timestamp, context);
        double filled = 0.0;
        for (const auto& fill : fills) {
            filled += std::abs(fill.quantity);
        }

        const auto emit_fills = [this](const std::vector<Fill>& emitted_fills,
                                       const Order& order_ref,
                                       const bool mark_maker) {
            for (const auto& enriched : enrich_fills(order_ref, emitted_fills, mark_maker)) {
                const auto event = events::make_order_event(
                    events::OrderEventKind::Fill,
                    enriched.timestamp,
                    enriched.order_id,
                    enriched.id,
                    enriched.quantity,
                    enriched.price,
                    enriched.symbol,
                    enriched.commission,
                    enriched.is_maker,
                    enriched.transaction_cost,
                    enriched.parent_order_id,
                    enriched.venue);
                event_queue_->push(event);
            }
        };

        const auto emit_cancel = [this](const Order& order_ref, const Timestamp ts) {
            const auto cancel = events::make_order_event(
                events::OrderEventKind::Cancel,
                ts,
                order_ref.id,
                0,
                0.0,
                0.0,
                order_ref.symbol);
            event_queue_->push(cancel);
        };

        const auto emit_reject = [this](const Order& order_ref, const Timestamp ts) {
            const auto reject = events::make_order_event(
                events::OrderEventKind::Reject,
                ts,
                order_ref.id,
                0,
                0.0,
                0.0,
                order_ref.symbol);
            event_queue_->push(reject);
        };

        if (state.effective_tif == TimeInForce::FOK && filled + kQuantityEpsilon < state.order.quantity) {
            resting_orders_.erase(id);
            emit_reject(state.order, timestamp);
            return;
        }

        if (state.effective_tif == TimeInForce::IOC) {
            if (!fills.empty()) {
                emit_fills(fills, state.order, false);
            }
            resting_orders_.erase(id);
            emit_cancel(state.order, timestamp);
            return;
        }

        if (!fills.empty()) {
            emit_fills(fills, state.order, maker_fill);
        }

        const double remaining = state.order.quantity - filled;
        const bool market_like = state.order.type == OrderType::Market
            || state.order.type == OrderType::MarketOnClose
            || state.order.type == OrderType::MarketOnOpen
            || (state.stop_triggered && state.order.type == OrderType::Stop);

        if (remaining <= kQuantityEpsilon) {
            resting_orders_.erase(id);
            return;
        }

        if (market_like) {
            resting_orders_.erase(id);
            emit_cancel(state.order, timestamp);
            return;
        }

        state.order.quantity = remaining;
        if (current_price > 0.0) {
            state.requested_price = current_price;
            state.has_requested_price = true;
        }
        resting_orders_[id] = state;

        const auto update = events::make_order_event(
            events::OrderEventKind::Update,
            timestamp,
            state.order.id,
            0,
            filled,
            0.0,
            state.order.symbol);
        event_queue_->push(update);
    }

    Price ExecutionPipeline::reference_price(const Order& order, const EvaluationContext context) const {
        const Price current_price = executable_price(order, context);
        if (current_price > 0.0) {
            return current_price;
        }
        return order.limit_price;
    }
}  // namespace regimeflow::engine
