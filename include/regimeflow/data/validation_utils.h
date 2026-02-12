/**
 * @file validation_utils.h
 * @brief RegimeFlow regimeflow validation utils declarations.
 */

#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/data/validation_config.h"

#include <optional>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Resolve the expected interval for a bar type.
 * @param bar_type Bar type.
 * @return Optional duration, empty for non-time bars.
 */
std::optional<Duration> bar_interval_for(BarType bar_type);

/**
 * @brief Fill gaps for time-based bars.
 * @param bars Input bars (sorted).
 * @param interval Expected bar interval.
 * @return Bars with missing intervals filled.
 */
std::vector<Bar> fill_missing_time_bars(const std::vector<Bar>& bars, Duration interval);

/**
 * @brief Validate and optionally repair bar data.
 * @param bars Input bars.
 * @param bar_type Bar type.
 * @param config Validation configuration.
 * @param fill_missing_bars Whether to fill time gaps.
 * @param collect_report Whether to populate report.
 * @param report Optional report output.
 * @return Validated (and possibly modified) bars.
 */
std::vector<Bar> validate_bars(std::vector<Bar> bars,
                               BarType bar_type,
                               const ValidationConfig& config,
                               bool fill_missing_bars,
                               bool collect_report,
                               ValidationReport* report);

/**
 * @brief Validate tick data.
 * @param ticks Input ticks.
 * @param config Validation configuration.
 * @param collect_report Whether to populate report.
 * @param report Optional report output.
 * @return Validated ticks.
 */
std::vector<Tick> validate_ticks(std::vector<Tick> ticks,
                                 const ValidationConfig& config,
                                 bool collect_report,
                                 ValidationReport* report);

}  // namespace regimeflow::data
