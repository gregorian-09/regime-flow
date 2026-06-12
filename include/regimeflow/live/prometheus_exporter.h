/**
 * @file prometheus_exporter.h
 * @brief Prometheus text exposition helpers for live operations.
 */

#pragma once

#include "regimeflow/engine/dashboard_snapshot.h"
#include "regimeflow/live/execution_quality.h"

#include <string>

namespace regimeflow::live
{
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
