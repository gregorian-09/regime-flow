#include "regimeflow/engine/execution_pipeline.h"

namespace regimeflow::engine {

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

void ExecutionPipeline::on_order_submitted(const Order& order) {
    if (!execution_model_ || !event_queue_) {
        return;
    }
    if (order.status == OrderStatus::Rejected || order.status == OrderStatus::Cancelled) {
        return;
    }
    Price ref_price = reference_price(order);
    if (order_books_) {
        if (auto book = order_books_->latest(order.symbol)) {
            auto model = execution::OrderBookExecutionModel(
                std::make_shared<data::OrderBook>(*book));
            Timestamp ts = order.created_at.microseconds() == 0
                ? Timestamp::now()
                : order.created_at;
            if (latency_model_) {
                ts = ts + latency_model_->latency();
            }
            auto fills = model.execute(order, ref_price, ts);
            for (const auto& fill : fills) {
                engine::Fill enriched = fill;
                if (market_impact_model_) {
                    double impact = market_impact_model_->impact_bps(order, &(*book)) / 10000.0;
                    enriched.price *= (order.side == engine::OrderSide::Buy) ? (1.0 + impact)
                                                                             : (1.0 - impact);
                }
                if (commission_model_) {
                    enriched.commission = commission_model_->commission(order, fill);
                }
                if (transaction_cost_model_) {
                    enriched.commission += transaction_cost_model_->cost(order, fill);
                }
                events::Event event = events::make_order_event(
                    events::OrderEventKind::Fill,
                    enriched.timestamp,
                    enriched.order_id,
                    enriched.id,
                    enriched.quantity,
                    enriched.price,
                    enriched.symbol,
                    enriched.commission);
                event_queue_->push(std::move(event));
            }
            if (!fills.empty()) {
                double filled = 0.0;
                for (const auto& f : fills) {
                    filled += std::abs(f.quantity);
                }
                if (filled < order.quantity) {
                    events::Event update = events::make_order_event(
                        events::OrderEventKind::Update,
                        ts,
                        order.id,
                        0,
                        filled,
                        0.0,
                        order.symbol);
                    event_queue_->push(std::move(update));
                }
            }
            return;
        }
    }
    Timestamp ts = order.created_at.microseconds() == 0
        ? Timestamp::now()
        : order.created_at;
    if (latency_model_) {
        ts = ts + latency_model_->latency();
    }
    auto fills = execution_model_->execute(order, ref_price, ts);
    for (const auto& fill : fills) {
        engine::Fill enriched = fill;
        if (market_impact_model_) {
            double impact = market_impact_model_->impact_bps(order, nullptr) / 10000.0;
            enriched.price *= (order.side == engine::OrderSide::Buy) ? (1.0 + impact)
                                                                     : (1.0 - impact);
        }
        if (commission_model_) {
            enriched.commission = commission_model_->commission(order, fill);
        }
        if (transaction_cost_model_) {
            enriched.commission += transaction_cost_model_->cost(order, fill);
        }
        events::Event event = events::make_order_event(
            events::OrderEventKind::Fill,
            enriched.timestamp,
            enriched.order_id,
            enriched.id,
            enriched.quantity,
            enriched.price,
            enriched.symbol,
            enriched.commission);
        event_queue_->push(std::move(event));
    }
    if (!fills.empty()) {
        double filled = 0.0;
        for (const auto& f : fills) {
            filled += std::abs(f.quantity);
        }
        if (filled < order.quantity) {
            events::Event update = events::make_order_event(
                events::OrderEventKind::Update,
                ts,
                order.id,
                0,
                filled,
                0.0,
                order.symbol);
            event_queue_->push(std::move(update));
        }
    }
}

Price ExecutionPipeline::reference_price(const Order& order) const {
    if (!market_data_) {
        return order.limit_price;
    }
    if (auto bar = market_data_->latest_bar(order.symbol)) {
        return bar->close;
    }
    if (auto tick = market_data_->latest_tick(order.symbol)) {
        return tick->price;
    }
    return order.limit_price;
}

}  // namespace regimeflow::engine
