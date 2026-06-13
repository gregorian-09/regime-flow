/**
 * @file prometheus_exporter.h
 * @brief Prometheus text exposition helpers for live operations.
 */

#pragma once

#include "regimeflow/engine/dashboard_snapshot.h"
#include "regimeflow/common/result.h"
#include "regimeflow/live/execution_quality.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace regimeflow::live
{
    /**
     * @brief Configuration for the built-in Prometheus scrape endpoint.
     */
    struct PrometheusScrapeEndpointConfig {
        std::string host = "127.0.0.1";
        uint16_t port = 9090;
        std::string path = "/metrics";
    };

    /**
     * @brief Minimal HTTP endpoint serving Prometheus text exposition.
     */
    class PrometheusScrapeEndpoint {
    public:
        using MetricsProvider = std::function<std::string()>;

        explicit PrometheusScrapeEndpoint(PrometheusScrapeEndpointConfig config = {});
        ~PrometheusScrapeEndpoint();

        PrometheusScrapeEndpoint(const PrometheusScrapeEndpoint&) = delete;
        PrometheusScrapeEndpoint& operator=(const PrometheusScrapeEndpoint&) = delete;
        PrometheusScrapeEndpoint(PrometheusScrapeEndpoint&&) noexcept;
        PrometheusScrapeEndpoint& operator=(PrometheusScrapeEndpoint&&) noexcept;

        [[nodiscard]] Result<void> start(MetricsProvider provider);
        void stop();
        [[nodiscard]] bool is_running() const;
        [[nodiscard]] uint16_t port() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    /**
     * @brief Export a dashboard snapshot in Prometheus text exposition format.
     */
    [[nodiscard]] std::string dashboard_snapshot_to_prometheus(
        const engine::DashboardSnapshot& snapshot);

    /**
     * @brief Export dashboard and execution-quality snapshots in one scrape payload.
     */
    [[nodiscard]] std::string live_metrics_to_prometheus(
        const engine::DashboardSnapshot& dashboard,
        const ExecutionQualitySnapshot& quality);
}  // namespace regimeflow::live
