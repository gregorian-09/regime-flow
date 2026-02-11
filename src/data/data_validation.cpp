#include "regimeflow/data/data_validation.h"

#include <sstream>

namespace regimeflow::data {

void ValidationReport::add_issue(ValidationIssue issue) {
    if (issue.severity == ValidationSeverity::Error) {
        ++errors_;
    } else {
        ++warnings_;
    }
    issues_.push_back(std::move(issue));
}

std::string ValidationReport::summary() const {
    std::ostringstream out;
    out << "Validation: " << errors_ << " errors, " << warnings_ << " warnings";
    return out.str();
}

}  // namespace regimeflow::data
