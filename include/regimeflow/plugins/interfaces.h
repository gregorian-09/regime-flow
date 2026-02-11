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

class RegimeDetectorPlugin : public Plugin {
public:
    virtual std::unique_ptr<regime::RegimeDetector> create_detector() = 0;
};

class ExecutionModelPlugin : public Plugin {
public:
    virtual std::unique_ptr<execution::SlippageModel> create_slippage_model() = 0;
    virtual std::unique_ptr<execution::CommissionModel> create_commission_model() = 0;
};

class DataSourcePlugin : public Plugin {
public:
    virtual std::unique_ptr<data::DataSource> create_data_source() = 0;
};

class RiskManagerPlugin : public Plugin {
public:
    virtual std::unique_ptr<risk::RiskManager> create_risk_manager() = 0;
};

class StrategyPlugin : public Plugin {
public:
    virtual std::unique_ptr<strategy::Strategy> create_strategy() = 0;
};

class MetricsPlugin : public Plugin {
public:
    virtual std::unique_ptr<metrics::PerformanceMetric> create_metric() = 0;
};

}  // namespace regimeflow::plugins
