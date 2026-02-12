/**
 * @file harmonic_pattern.h
 * @brief RegimeFlow regimeflow harmonic pattern strategy declarations.
 */

#pragma once

#include "regimeflow/strategy/strategy.h"
#include "regimeflow/common/types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace regimeflow::strategy {

/**
 * @brief Harmonic pattern strategy with simple smart order routing.
 *
 * Detects common harmonic patterns (Gartley, Bat, Butterfly, Crab, Cypher)
 * on swing pivots and submits market or limit orders based on volatility.
 */
class HarmonicPatternStrategy final : public Strategy {
public:
    void initialize(StrategyContext& ctx) override;
    void on_bar(const data::Bar& bar) override;

private:
    struct Pivot {
        size_t index = 0;
        double price = 0.0;
    };

    enum class PatternType : uint8_t {
        Gartley,
        Bat,
        Butterfly,
        Crab,
        Cypher
    };

    bool load_symbol_from_config();
    std::vector<Pivot> detect_pivots(const std::vector<double>& prices) const;
    bool match_pattern(PatternType type,
                       double ab_xa,
                       double bc_ab,
                       double cd_bc,
                       double ad_xa,
                       double cd_xc) const;
    double pattern_confidence(PatternType type,
                              double ab_xa,
                              double bc_ab,
                              double cd_bc,
                              double ad_xa,
                              double cd_xc) const;
    bool should_use_limit(const data::Bar& bar) const;
    engine::Order build_order(engine::OrderSide side,
                              const data::Bar& bar,
                              PatternType pattern,
                              double confidence) const;

    SymbolId symbol_id_ = 0;
    std::string symbol_;
    double pivot_threshold_pct_ = 0.03;
    double tolerance_ = 0.1;
    size_t min_bars_ = 120;
    size_t cooldown_bars_ = 5;
    size_t last_signal_index_ = 0;
    bool use_limit_ = true;
    double limit_offset_bps_ = 2.0;
    double vol_threshold_pct_ = 0.01;
    double min_confidence_ = 0.45;
    double min_qty_scale_ = 0.5;
    double max_qty_scale_ = 1.5;
    double aggressive_confidence_threshold_ = 0.7;
    double venue_switch_confidence_ = 0.6;
    double passive_venue_weight_ = 0.7;
    double aggressive_venue_weight_ = 0.3;
    bool allow_short_ = false;
    Quantity order_qty_ = 10;
};

}  // namespace regimeflow::strategy
