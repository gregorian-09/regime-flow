#pragma once

#include "regimeflow/common/time.h"

namespace regimeflow::execution {

class LatencyModel {
public:
    virtual ~LatencyModel() = default;
    virtual Duration latency() const = 0;
};

class FixedLatencyModel final : public LatencyModel {
public:
    explicit FixedLatencyModel(Duration latency) : latency_(latency) {}
    Duration latency() const override { return latency_; }

private:
    Duration latency_ = Duration::milliseconds(0);
};

}  // namespace regimeflow::execution
