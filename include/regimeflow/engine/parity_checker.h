/**
 * @file parity_checker.h
 * @brief RegimeFlow parity checker declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/engine/parity_report.h"

#include <string>

namespace regimeflow::engine
{
    /**
     * @brief Validate parity between backtest and live configurations.
     */
    class ParityChecker {
    public:
        /**
         * @brief Check parity between in-memory configs.
         * @param backtest Backtest configuration.
         * @param live Live configuration.
         * @return Parity report.
         */
        static ParityReport check(const Config& backtest, const Config& live);
        /**
         * @brief Check parity between YAML files.
         * @param backtest_path Backtest YAML path.
         * @param live_path Live YAML path.
         * @return Parity report.
         */
        static ParityReport check_files(const std::string& backtest_path,
                                        const std::string& live_path);
    };
}  // namespace regimeflow::engine
