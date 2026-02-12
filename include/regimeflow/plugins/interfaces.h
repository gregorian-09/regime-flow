/**
 * @file interfaces.h
 * @brief RegimeFlow regimeflow interfaces declarations.
 */

#pragma once

#include "regimeflow/plugins/plugin.h"
#include "regimeflow/regime/regime_detector.h"
#include "regimeflow/execution/slippage.h"
#include "regimeflow/execution/commission.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/metrics/performance_metric.h"
#include "regimeflow/risk/risk_limits.h"
#include "regimeflow/strategy/strategy.h"

#include <memory>

namespace regimeflow::plugins {

/**
 * @brief Plugin interface for custom regime detectors.
 */
class RegimeDetectorPlugin : public Plugin {
public:
    /**
     * @brief Create a regime detector instance.
     */
    virtual std::unique_ptr<regime::RegimeDetector> create_detector() = 0;
};

/**
 * @brief Plugin interface for execution models.
 */
class ExecutionModelPlugin : public Plugin {
public:
    /**
     * @brief Create a slippage model.
     */
    virtual std::unique_ptr<execution::SlippageModel> create_slippage_model() = 0;
    /**
     * @brief Create a commission model.
     */
    virtual std::unique_ptr<execution::CommissionModel> create_commission_model() = 0;
};

/**
 * @brief Plugin interface for data sources.
 */
class DataSourcePlugin : public Plugin {
public:
    /**
     * @brief Create a data source instance.
     */
    virtual std::unique_ptr<data::DataSource> create_data_source() = 0;
};

/**
 * @brief Plugin interface for risk managers.
 */
class RiskManagerPlugin : public Plugin {
public:
    /**
     * @brief Create a risk manager instance.
     */
    virtual std::unique_ptr<risk::RiskManager> create_risk_manager() = 0;
};

/**
 * @brief Plugin interface for strategies.
 */
class StrategyPlugin : public Plugin {
public:
    /**
     * @brief Create a strategy instance.
     */
    virtual std::unique_ptr<strategy::Strategy> create_strategy() = 0;
};

/**
 * @brief Plugin interface for custom performance metrics.
 */
class MetricsPlugin : public Plugin {
public:
    /**
     * @brief Create a performance metric instance.
     */
    virtual std::unique_ptr<metrics::PerformanceMetric> create_metric() = 0;
};

}  // namespace regimeflow::plugins
