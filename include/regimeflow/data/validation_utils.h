#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/data/validation_config.h"

#include <optional>
#include <vector>

namespace regimeflow::data {

std::optional<Duration> bar_interval_for(BarType bar_type);

std::vector<Bar> fill_missing_time_bars(const std::vector<Bar>& bars, Duration interval);

std::vector<Bar> validate_bars(std::vector<Bar> bars,
                               BarType bar_type,
                               const ValidationConfig& config,
                               bool fill_missing_bars,
                               bool collect_report,
                               ValidationReport* report);

std::vector<Tick> validate_ticks(std::vector<Tick> ticks,
                                 const ValidationConfig& config,
                                 bool collect_report,
                                 ValidationReport* report);

}  // namespace regimeflow::data
