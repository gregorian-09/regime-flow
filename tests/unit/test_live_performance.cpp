#include "regimeflow/metrics/live_performance.h"

#include <gtest/gtest.h>

#include <filesystem>

using regimeflow::metrics::LivePerformanceConfig;
using regimeflow::metrics::LivePerformanceTracker;
using regimeflow::Timestamp;

TEST(LivePerformanceTrackerTest, WritesFiles) {
    auto tmp = std::filesystem::temp_directory_path() / "regimeflow_live_metrics_test";
    std::filesystem::remove_all(tmp);

    LivePerformanceConfig cfg;
    cfg.enabled = true;
    cfg.output_dir = tmp.string();
    cfg.sinks = {"file"};

    LivePerformanceTracker tracker(cfg);
    tracker.start(100000.0);
    tracker.update(Timestamp::now(), 101000.0, 1000.0);
    tracker.flush();

    auto csv_path = tmp / "live_drift.csv";
    auto json_path = tmp / "live_performance.json";

    EXPECT_TRUE(std::filesystem::exists(csv_path));
    EXPECT_TRUE(std::filesystem::exists(json_path));

    std::filesystem::remove_all(tmp);
}
