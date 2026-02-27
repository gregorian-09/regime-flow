/**
 * @file parity_report.h
 * @brief RegimeFlow parity report declarations.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::engine
{
    /**
     * @brief Status for parity checking.
     */
    enum class ParityStatus {
        Pass,
        Warn,
        Fail
    };

    /**
     * @brief Result of a parity check between backtest and live configs.
     */
    struct ParityReport {
        ParityStatus status = ParityStatus::Pass;
        std::vector<std::string> hard_errors;
        std::vector<std::string> warnings;
        std::unordered_map<std::string, std::string> backtest_values;
        std::unordered_map<std::string, std::string> live_values;
        std::unordered_map<std::string, std::string> compat_matrix;

        [[nodiscard]] bool ok() const { return status == ParityStatus::Pass; }
        [[nodiscard]] std::string status_name() const {
            switch (status) {
            case ParityStatus::Pass: return "pass";
            case ParityStatus::Warn: return "warn";
            case ParityStatus::Fail: return "fail";
            default: return "unknown";
            }
        }
    };
}  // namespace regimeflow::engine
