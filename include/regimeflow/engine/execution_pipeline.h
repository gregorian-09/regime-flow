/**
 * @file execution_pipeline.h
 * @brief RegimeFlow regimeflow execution pipeline declarations.
 */

#pragma once

#include "regimeflow/execution/execution_model.h"
#include "regimeflow/execution/basic_execution_model.h"
#include "regimeflow/execution/order_book_execution_model.h"
#include "regimeflow/execution/commission.h"
#include "regimeflow/execution/market_impact.h"
#include "regimeflow/execution/latency_model.h"
#include "regimeflow/execution/slippage.h"
#include "regimeflow/execution/transaction_cost.h"
#include "regimeflow/engine/order_manager.h"
#include "regimeflow/engine/market_data_cache.h"
#include "regimeflow/engine/order_book_cache.h"
#include "regimeflow/events/event_queue.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace regimeflow::engine
{
    /**
     * @brief Composes execution, commission, cost, impact, and latency models.
     */
    class ExecutionPipeline {
    public:
        /**
         * @brief Default fill policy applied when the order itself does not request IOC/FOK.
         */
        enum class FillPolicy : uint8_t {
            PreserveOrder,
            Return,
            IOC,
            FOK
        };

        /**
         * @brief Action to take when executable price drifts beyond the allowed deviation.
         */
        enum class PriceDriftAction : uint8_t {
            Ignore,
            Reject,
            Requote
        };

        /**
         * @brief Queue depth model used for resting maker orders.
         */
        enum class QueueDepthMode : uint8_t {
            TopOnly,
            PriceLevel,
            FullDepth
        };

        /**
         * @brief Bar replay modes for bar-only execution simulation.
         */
        enum class BarSimulationMode : uint8_t {
            CloseOnly,
            OpenOnly,
            IntrabarOhlc
        };

        /**
         * @brief Session execution policy for calendar-aware fills.
         */
        struct SessionPolicy {
            bool enabled = false;
            int start_minute_utc = 9 * 60 + 30;
            int end_minute_utc = 16 * 60;
            int open_auction_minutes = 1;
            int close_auction_minutes = 1;
            bool halt_all = false;
            std::unordered_set<SymbolId> halted_symbols;
            std::unordered_set<int> allowed_weekdays;
            std::unordered_set<std::string> closed_dates;
            bool dynamic_halt_all = false;
            std::unordered_set<SymbolId> dynamic_halted_symbols;
        };

        /**
         * @brief Construct the pipeline.
         * @param order_manager Order manager to update fills/orders.
         * @param market_data Market data cache.
         * @param order_books Order book cache.
         * @param event_queue Event queue for fill events.
         */
        ExecutionPipeline(MarketDataCache* market_data,
                          OrderBookCache* order_books,
                          events::EventQueue* event_queue);

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
         * @brief Set the default fill policy applied to Day/GTC orders.
         * @param policy Fill policy.
         */
        void set_default_fill_policy(FillPolicy policy);
        /**
         * @brief Configure how adverse price drift is handled.
         * @param max_deviation_bps Allowed drift before a rule is applied.
         * @param action Reject, requote, or ignore.
         */
        void set_price_drift_rule(double max_deviation_bps, PriceDriftAction action);
        /**
         * @brief Configure queue progression for resting maker orders.
         * @param enabled True to simulate queue ahead at the touch.
         * @param progress_fraction Fraction of visible queue cleared per qualifying event.
         * @param default_visible_qty Fallback visible size when quotes/books do not provide one.
         * @param mode Queue depth sampling mode.
         */
        void set_queue_model(bool enabled,
                             double progress_fraction,
                             double default_visible_qty,
                             QueueDepthMode mode = QueueDepthMode::TopOnly,
                             double aging_fraction = 0.0,
                             double replenishment_fraction = 0.0);
        /**
         * @brief Select how bars are replayed into the execution simulator.
         * @param mode Bar simulation mode.
         */
        void set_bar_simulation_mode(BarSimulationMode mode);
        /**
         * @brief Configure session-aware execution gates and halts.
         * @param policy Session policy.
         */
        void set_session_policy(SessionPolicy policy);
        /**
         * @brief Toggle a dynamic symbol halt sourced from system events.
         * @param symbol Symbol to halt/resume.
         * @param halted True to halt, false to resume.
         */
        void set_symbol_halt(SymbolId symbol, bool halted);
        /**
         * @brief Toggle a global dynamic halt sourced from system events.
         * @param halted True to halt, false to resume.
         */
        void set_global_halt(bool halted);

        /**
         * @brief Handle order submission and generate fill events.
         * @param order Submitted order.
         */
        void on_order_submitted(const Order& order);
        /**
         * @brief Update execution state when the order manager changes status.
         * @param order Latest order snapshot.
         */
        void on_order_update(const Order& order);
        /**
         * @brief Re-evaluate resting orders when market data changes.
         * @param symbol Symbol that changed.
         * @param timestamp Current event time.
         */
        void on_market_update(SymbolId symbol, Timestamp timestamp);
        /**
         * @brief Re-evaluate resting orders using a full OHLC bar snapshot.
         * @param bar Latest bar.
         */
        void on_bar(const data::Bar& bar);
        /**
         * @brief Simulate an immediate fill using the configured execution, impact, commission,
         * and transaction-cost models without routing through the event queue.
         * @param order Order to simulate.
         * @param reference_price Reference price for execution.
         * @param timestamp Evaluation timestamp.
         * @param is_maker Whether the fill should be modeled as maker-side.
         * @return Enriched fills using the same pricing/cost models as normal execution.
         */
        [[nodiscard]] std::vector<Fill> simulate_immediate_fills(const Order& order,
                                                                 Price reference_price,
                                                                 Timestamp timestamp,
                                                                 bool is_maker = false) const;

    private:
        struct RestingOrderState {
            Order order;
            TimeInForce effective_tif = TimeInForce::GTC;
            bool stop_triggered = false;
            Price requested_price = 0.0;
            bool has_requested_price = false;
            Timestamp activation_time;
            bool was_resting = false;
            bool queue_initialized = false;
            Quantity queue_ahead = 0.0;
            Quantity last_visible_queue = 0.0;
        };
        struct EvaluationContext {
            Price executable_price_override;
            bool has_price_override;
            bool use_order_book;

            EvaluationContext()
                : executable_price_override(0.0),
                  has_price_override(false),
                  use_order_book(true) {}
        };

        [[nodiscard]] TimeInForce effective_tif_for(const Order& order) const;
        [[nodiscard]] bool queue_enabled_for(const Order& order) const;
        [[nodiscard]] double queue_progress_fraction_for(const Order& order) const;
        [[nodiscard]] double queue_default_visible_qty_for(const Order& order) const;
        [[nodiscard]] QueueDepthMode queue_depth_mode_for(const Order& order) const;
        [[nodiscard]] Quantity visible_queue_quantity(const Order& order) const;
        [[nodiscard]] bool is_touch_fill_candidate(const RestingOrderState& state,
                                                   EvaluationContext context = EvaluationContext()) const;
        [[nodiscard]] bool is_price_through_limit(const RestingOrderState& state,
                                                  EvaluationContext context = EvaluationContext()) const;
        void refresh_queue_state(RestingOrderState& state,
                                 EvaluationContext context = EvaluationContext()) const;
        bool advance_queue(RestingOrderState& state,
                           EvaluationContext context = EvaluationContext()) const;
        [[nodiscard]] bool session_allows_execution(const Order& order,
                                                    Timestamp timestamp) const;
        [[nodiscard]] bool can_fill_now(const RestingOrderState& state,
                                        EvaluationContext context = EvaluationContext()) const;
        [[nodiscard]] bool should_trigger_stop(const RestingOrderState& state,
                                               EvaluationContext context = EvaluationContext()) const;
        [[nodiscard]] Price executable_price(const Order& order,
                                             EvaluationContext context = EvaluationContext()) const;
        [[nodiscard]] std::vector<Fill> build_fills(const RestingOrderState& state,
                                                    Timestamp timestamp,
                                                    EvaluationContext context = EvaluationContext()) const;
        [[nodiscard]] std::vector<Fill> enrich_fills(const Order& order,
                                                     const std::vector<Fill>& fills,
                                                     bool is_maker) const;
        void process_resting_order(OrderId id,
                                   Timestamp timestamp,
                                   EvaluationContext context = EvaluationContext());
        [[nodiscard]] Price reference_price(const Order& order,
                                            EvaluationContext context = EvaluationContext()) const;

        MarketDataCache* market_data_ = nullptr;
        OrderBookCache* order_books_ = nullptr;
        events::EventQueue* event_queue_ = nullptr;
        std::unique_ptr<execution::ExecutionModel> execution_model_;
        std::unique_ptr<execution::CommissionModel> commission_model_;
        std::unique_ptr<execution::TransactionCostModel> transaction_cost_model_;
        std::unique_ptr<execution::MarketImpactModel> market_impact_model_;
        std::unique_ptr<execution::LatencyModel> latency_model_;
        FillPolicy default_fill_policy_ = FillPolicy::PreserveOrder;
        PriceDriftAction price_drift_action_ = PriceDriftAction::Ignore;
        double max_deviation_bps_ = 0.0;
        bool queue_model_enabled_ = false;
        double queue_progress_fraction_ = 1.0;
        double queue_default_visible_qty_ = 1.0;
        double queue_aging_fraction_ = 0.0;
        double queue_replenishment_fraction_ = 0.0;
        QueueDepthMode queue_depth_mode_ = QueueDepthMode::TopOnly;
        BarSimulationMode bar_simulation_mode_ = BarSimulationMode::CloseOnly;
        SessionPolicy session_policy_;
        std::unordered_map<OrderId, RestingOrderState> resting_orders_;
    };
}  // namespace regimeflow::engine
