#pragma once

#include "regimeflow/engine/order.h"
#include "regimeflow/regime/types.h"

#include <memory>
#include <optional>
#include <unordered_map>

namespace regimeflow::execution {

class SlippageModel {
public:
    virtual ~SlippageModel() = default;
    virtual Price execution_price(const engine::Order& order, Price reference_price) const = 0;
};

class ZeroSlippageModel final : public SlippageModel {
public:
    Price execution_price(const engine::Order& order, Price reference_price) const override;
};

class FixedBpsSlippageModel final : public SlippageModel {
public:
    explicit FixedBpsSlippageModel(double bps);

    Price execution_price(const engine::Order& order, Price reference_price) const override;

private:
    double bps_ = 0.0;
};

class RegimeBpsSlippageModel final : public SlippageModel {
public:
    RegimeBpsSlippageModel(double default_bps,
                           std::unordered_map<regime::RegimeType, double> bps_map);

    Price execution_price(const engine::Order& order, Price reference_price) const override;

private:
    static std::optional<regime::RegimeType> parse_regime(const std::string& value);

    double default_bps_ = 0.0;
    std::unordered_map<regime::RegimeType, double> bps_map_;
};

}  // namespace regimeflow::execution
