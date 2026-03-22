#include <gtest/gtest.h>

#include "regimeflow/metrics/report.h"
#include "regimeflow/metrics/report_writer.h"

namespace
{
    TEST(ReportWriterTest, WritesCsvAndJson) {
        regimeflow::metrics::Report report;
        report.performance.total_return = 0.1;
        report.max_drawdown = 0.05;
        report.regime_performance[regimeflow::regime::RegimeType::Bull] = {0.1, 5};

        auto csv = regimeflow::metrics::ReportWriter::to_csv(report);
        EXPECT_NE(csv.find("total_return"), std::string::npos);

        auto json = regimeflow::metrics::ReportWriter::to_json(report);
        EXPECT_NE(json.find("\"performance\""), std::string::npos);
    }

    TEST(ReportBuilderTest, UsesConsistentAnnualizationForIntradayCurves) {
        regimeflow::metrics::MetricsTracker tracker;
        const auto start = regimeflow::Timestamp(0);

        tracker.update(start, 100000.0);
        tracker.update(start + regimeflow::Duration::hours(1), 100500.0);
        tracker.update(start + regimeflow::Duration::hours(2), 100250.0);
        tracker.update(start + regimeflow::Duration::hours(3), 100900.0);
        tracker.update(start + regimeflow::Duration::hours(4), 101100.0);

        const auto report = regimeflow::metrics::build_report(tracker, std::vector<regimeflow::engine::Fill>{}, 0.0);

        EXPECT_NEAR(report.performance.total_return, report.performance_summary.total_return, 1e-9);
        EXPECT_NEAR(report.performance.max_drawdown, report.performance_summary.max_drawdown, 1e-9);
        EXPECT_NEAR(report.performance.sharpe, report.performance_summary.sharpe_ratio, 1e-9);
        EXPECT_NEAR(report.performance.sortino, report.performance_summary.sortino_ratio, 1e-9);
        EXPECT_NEAR(report.performance.calmar, report.performance_summary.calmar_ratio, 1e-9);
    }
}  // namespace
