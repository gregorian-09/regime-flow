/**
 * @file validation_config.h
 * @brief RegimeFlow regimeflow validation config declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

namespace regimeflow::data {

/**
 * @brief Action to take on validation events.
 */
enum class ValidationAction : uint8_t {
    Fail,
    Skip,
    Fill,
    Continue
};

/**
 * @brief Validation configuration for data ingestion.
 */
struct ValidationConfig {
    /**
     * @brief Maximum allowed gap between bars/ticks.
     */
    Duration max_gap = Duration::days(2);
    /**
     * @brief Maximum allowed price jump as a fraction.
     */
    double max_jump_pct = 0.2;
    /**
     * @brief Maximum allowed future timestamp skew.
     */
    Duration max_future_skew = Duration::seconds(0);
    /**
     * @brief Maximum allowed volume.
     */
    uint64_t max_volume = 0;
    /**
     * @brief Maximum allowed price.
     */
    double max_price = 0.0;
    /**
     * @brief Z-score threshold for outlier detection.
     */
    double outlier_zscore = 5.0;
    /**
     * @brief Warmup period before applying outlier checks.
     */
    size_t outlier_warmup = 30;
    /**
     * @brief Start of trading hours in seconds since midnight.
     */
    int trading_start_seconds = 0;
    /**
     * @brief End of trading hours in seconds since midnight.
     */
    int trading_end_seconds = 24 * 60 * 60;
    /**
     * @brief Require strictly non-decreasing timestamps.
     */
    bool require_monotonic_timestamps = true;
    /**
     * @brief Check for price bounds.
     */
    bool check_price_bounds = true;
    /**
     * @brief Check for time gaps.
     */
    bool check_gap = false;
    /**
     * @brief Check for large price jumps.
     */
    bool check_price_jump = false;
    /**
     * @brief Check for timestamps in the future.
     */
    bool check_future_timestamps = false;
    /**
     * @brief Check for data outside trading hours.
     */
    bool check_trading_hours = false;
    /**
     * @brief Check for volume bounds.
     */
    bool check_volume_bounds = false;
    /**
     * @brief Check for statistical outliers.
     */
    bool check_outliers = false;
    /**
     * @brief Action when an error occurs.
     */
    ValidationAction on_error = ValidationAction::Fail;
    /**
     * @brief Action when a gap is detected.
     */
    ValidationAction on_gap = ValidationAction::Fill;
    /**
     * @brief Action when a warning is detected.
     */
    ValidationAction on_warning = ValidationAction::Continue;
};

}  // namespace regimeflow::data
