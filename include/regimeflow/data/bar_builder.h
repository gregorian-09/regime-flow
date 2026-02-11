#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"

#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::data {

class BarBuilder {
public:
    struct Config {
        BarType type = BarType::Time_1Min;
        int64_t time_interval_ms = 60'000;
        uint64_t volume_threshold = 0;
        uint64_t tick_threshold = 0;
        double dollar_threshold = 0.0;
    };

    explicit BarBuilder(const Config& config);

    std::optional<Bar> process(const Tick& tick);
    std::optional<Bar> flush();
    void reset();

private:
    bool is_bar_complete(const Tick& tick) const;
    Bar build_bar() const;
    void update_state(const Tick& tick);
    void reset_state(const Tick& tick);

    Config config_;
    SymbolId symbol_ = 0;
    Timestamp bar_start_;
    Price open_ = std::numeric_limits<double>::quiet_NaN();
    Price high_ = std::numeric_limits<double>::lowest();
    Price low_ = std::numeric_limits<double>::max();
    Price close_ = 0;
    Volume volume_ = 0;
    uint64_t tick_count_ = 0;
    double dollar_volume_ = 0.0;
    bool has_state_ = false;
};

class MultiSymbolBarBuilder {
public:
    explicit MultiSymbolBarBuilder(const BarBuilder::Config& config);

    std::optional<Bar> process(const Tick& tick);
    std::vector<Bar> flush_all();

private:
    BarBuilder::Config config_;
    std::unordered_map<SymbolId, BarBuilder> builders_;
};

}  // namespace regimeflow::data
