#pragma once

#include "regimeflow/common/time.h"

#include <string>
#include <vector>

namespace regimeflow::data {

enum class ValidationSeverity : uint8_t {
    Error,
    Warning
};

struct ValidationIssue {
    ValidationSeverity severity = ValidationSeverity::Error;
    size_t line = 0;
    std::string message;
};

class ValidationReport {
public:
    void add_issue(ValidationIssue issue);

    bool ok() const { return errors_ == 0; }
    int error_count() const { return errors_; }
    int warning_count() const { return warnings_; }
    const std::vector<ValidationIssue>& issues() const { return issues_; }

    std::string summary() const;

private:
    std::vector<ValidationIssue> issues_;
    int errors_ = 0;
    int warnings_ = 0;
};

}  // namespace regimeflow::data
