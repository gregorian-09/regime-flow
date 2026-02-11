#pragma once

#include "regimeflow/metrics/report.h"

#include <string>

namespace regimeflow::metrics {

class ReportWriter {
public:
    static std::string to_csv(const Report& report);
    static std::string to_json(const Report& report);
};

}  // namespace regimeflow::metrics
