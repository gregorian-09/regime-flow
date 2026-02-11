#include <gtest/gtest.h>

#include "regimeflow/metrics/report_writer.h"

namespace {

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

}  // namespace
