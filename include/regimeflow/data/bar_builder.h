/**
 * @file bar_builder.h
 * @brief RegimeFlow regimeflow bar builder declarations.
 */

#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/tick.h"

#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Builds bars from tick data.
 */
class BarBuilder {
public:
    /**
     * @brief Builder configuration for bar types and thresholds.
     */
    struct Config {
        /**
         * @brief Bar aggregation type.
         */
        BarType type = BarType::Time_1Min;
        /**
         * @brief Time interval in milliseconds for time bars.
         */
        int64_t time_interval_ms = 60'000;
        /**
         * @brief Volume threshold for volume bars.
         */
        uint64_t volume_threshold = 0;
        /**
         * @brief Tick threshold for tick bars.
         */
        uint64_t tick_threshold = 0;
        /**
         * @brief Dollar volume threshold for dollar bars.
         */
        double dollar_threshold = 0.0;
    };

    /**
     * @brief Construct a bar builder with configuration.
     * @param config Builder config.
     */
    explicit BarBuilder(const Config& config);

    /**
     * @brief Process a tick and emit a bar if complete.
     * @param tick Tick input.
     * @return Optional bar when a bar closes.
     */
    std::optional<Bar> process(const Tick& tick);
    /**
     * @brief Flush the current in-progress bar.
     * @return Optional bar if a bar is in progress.
     */
    std::optional<Bar> flush();
    /**
     * @brief Reset the builder state.
     */
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

/**
 * @brief Builds bars for multiple symbols simultaneously.
 */
class MultiSymbolBarBuilder {
public:
    /**
     * @brief Construct a multi-symbol builder.
     * @param config Builder config shared across symbols.
     */
    explicit MultiSymbolBarBuilder(const BarBuilder::Config& config);

    /**
     * @brief Process a tick and emit a bar when complete.
     * @param tick Tick input.
     * @return Optional bar when a bar closes.
     */
    std::optional<Bar> process(const Tick& tick);
    /**
     * @brief Flush all in-progress bars for all symbols.
     * @return Vector of completed bars.
     */
    std::vector<Bar> flush_all();

private:
    BarBuilder::Config config_;
    std::unordered_map<SymbolId, BarBuilder> builders_;
};

}  // namespace regimeflow::data
