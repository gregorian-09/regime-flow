/**
 * @file data_validation.h
 * @brief RegimeFlow regimeflow data validation declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

#include <string>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Severity level for validation issues.
 */
enum class ValidationSeverity : uint8_t {
    Error,
    Warning
};

/**
 * @brief Validation issue descriptor.
 */
struct ValidationIssue {
    ValidationSeverity severity = ValidationSeverity::Error;
    size_t line = 0;
    std::string message;
};

/**
 * @brief Aggregated validation report.
 */
class ValidationReport {
public:
    /**
     * @brief Add an issue to the report.
     * @param issue Validation issue.
     */
    void add_issue(ValidationIssue issue);

    /**
     * @brief True if no errors were recorded.
     */
    bool ok() const { return errors_ == 0; }
    /**
     * @brief Number of errors.
     */
    int error_count() const { return errors_; }
    /**
     * @brief Number of warnings.
     */
    int warning_count() const { return warnings_; }
    /**
     * @brief All recorded issues.
     */
    const std::vector<ValidationIssue>& issues() const { return issues_; }

    /**
     * @brief Summary string of validation results.
     * @return Summary text.
     */
    std::string summary() const;

private:
    std::vector<ValidationIssue> issues_;
    int errors_ = 0;
    int warnings_ = 0;
};

}  // namespace regimeflow::data
