#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/strategy/strategy.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace regimeflow::strategy {

class StrategyFactory {
public:
    using Creator = std::function<std::unique_ptr<Strategy>(const Config&)>;

    static StrategyFactory& instance();

    void register_creator(std::string name, Creator creator);
    std::unique_ptr<Strategy> create(const Config& config) const;

private:
    std::unordered_map<std::string, Creator> creators_;
};

void register_builtin_strategies();

}  // namespace regimeflow::strategy
