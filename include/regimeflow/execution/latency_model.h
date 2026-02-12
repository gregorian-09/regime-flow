/**
 * @file latency_model.h
 * @brief RegimeFlow regimeflow latency model declarations.
 */

#pragma once

#include "regimeflow/common/time.h"

namespace regimeflow::execution {

/**
 * @brief Base class for latency models.
 */
class LatencyModel {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~LatencyModel() = default;
    /**
     * @brief Current latency estimate.
     * @return Duration of latency.
     */
    virtual Duration latency() const = 0;
};

/**
 * @brief Fixed latency model.
 */
class FixedLatencyModel final : public LatencyModel {
public:
    /**
     * @brief Construct with a fixed latency.
     * @param latency Latency duration.
     */
    explicit FixedLatencyModel(Duration latency) : latency_(latency) {}
    /**
     * @brief Return the fixed latency.
     */
    Duration latency() const override { return latency_; }

private:
    Duration latency_ = Duration::milliseconds(0);
};

}  // namespace regimeflow::execution
