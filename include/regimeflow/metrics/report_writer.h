/**
 * @file report_writer.h
 * @brief RegimeFlow regimeflow report writer declarations.
 */

#pragma once

#include "regimeflow/metrics/report.h"

#include <string>

namespace regimeflow::metrics {

/**
 * @brief Serialize reports to common formats.
 */
class ReportWriter {
public:
    /**
     * @brief Serialize a report to CSV.
     * @param report Report to serialize.
     * @return CSV string.
     */
    static std::string to_csv(const Report& report);
    /**
     * @brief Serialize a report to JSON.
     * @param report Report to serialize.
     * @return JSON string.
     */
    static std::string to_json(const Report& report);
};

}  // namespace regimeflow::metrics
