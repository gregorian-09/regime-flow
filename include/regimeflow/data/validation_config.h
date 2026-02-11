#pragma once

#include "regimeflow/common/time.h"

namespace regimeflow::data {

enum class ValidationAction : uint8_t {
    Fail,
    Skip,
    Fill,
    Continue
};

struct ValidationConfig {
    Duration max_gap = Duration::days(2);
    double max_jump_pct = 0.2;
    Duration max_future_skew = Duration::seconds(0);
    uint64_t max_volume = 0;
    double max_price = 0.0;
    double outlier_zscore = 5.0;
    size_t outlier_warmup = 30;
    int trading_start_seconds = 0;
    int trading_end_seconds = 24 * 60 * 60;
    bool require_monotonic_timestamps = true;
    bool check_price_bounds = true;
    bool check_gap = false;
    bool check_price_jump = false;
    bool check_future_timestamps = false;
    bool check_trading_hours = false;
    bool check_volume_bounds = false;
    bool check_outliers = false;
    ValidationAction on_error = ValidationAction::Fail;
    ValidationAction on_gap = ValidationAction::Fill;
    ValidationAction on_warning = ValidationAction::Continue;
};

}  // namespace regimeflow::data
