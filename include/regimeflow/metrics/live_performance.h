/**
 * @file live_performance.h
 * @brief Live performance tracking declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

#include <string>
#include <vector>

namespace regimeflow::metrics
{
    /**
     * @brief Live performance tracker configuration.
     */
    struct LivePerformanceConfig {
        bool enabled = false;
        std::string baseline_report;
        std::string output_dir = "./logs";
        std::vector<std::string> sinks {"file"};
        std::string postgres_connection_string;
        std::string postgres_table = "regimeflow_live_performance";
        int postgres_pool_size = 2;
    };

    /**
     * @brief Live performance snapshot.
     */
    struct LivePerformanceSnapshot {
        Timestamp timestamp;
        double equity = 0.0;
        double daily_pnl = 0.0;
        double total_return = 0.0;
        double drawdown = 0.0;
        double shortfall_vs_backtest = 0.0;
    };

    /**
     * @brief Summary of live performance.
     */
    struct LivePerformanceSummary {
        double baseline_total_return = 0.0;
        double live_total_return = 0.0;
        double max_drawdown = 0.0;
        double max_drawdown_pct = 0.0;
        double last_equity = 0.0;
        Timestamp last_timestamp;
    };

    /**
     * @brief Track live performance vs baseline.
     */
    class LivePerformanceTracker {
    public:
        /**
         * @brief Construct a tracker with configuration.
         */
        explicit LivePerformanceTracker(LivePerformanceConfig config);

        /**
         * @brief Start tracking with initial equity.
         */
        void start(double initial_equity);

        /**
         * @brief Update tracker with latest equity snapshot.
         */
        void update(Timestamp timestamp, double equity, double daily_pnl);

        /**
         * @brief Flush outputs.
         */
        void flush();

        /**
         * @brief Access current summary.
         */
        [[nodiscard]] const LivePerformanceSummary& summary() const { return summary_; }

        /**
         * @brief Access config.
         */
        [[nodiscard]] const LivePerformanceConfig& config() const { return config_; }

    private:
        LivePerformanceConfig config_;
        LivePerformanceSummary summary_;
        double starting_equity_ = 0.0;
        double peak_equity_ = 0.0;
        bool started_ = false;

        void ensure_output_dir() const;
        void write_file_snapshot(const LivePerformanceSnapshot& snapshot);
        void write_file_summary();
        void write_postgres_snapshot(const LivePerformanceSnapshot& snapshot);
        void load_baseline();
    };
}  // namespace regimeflow::metrics
