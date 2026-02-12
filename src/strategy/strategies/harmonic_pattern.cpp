#include "regimeflow/strategy/strategies/harmonic_pattern.h"

#include "regimeflow/common/types.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::strategy {

namespace {

double ratio(double num, double den) {
    if (den == 0.0) {
        return 0.0;
    }
    return std::abs(num / den);
}

bool in_range(double value, double min_v, double max_v, double tol) {
    double low = min_v * (1.0 - tol);
    double high = max_v * (1.0 + tol);
    return value >= low && value <= high;
}

double score_ratio(double value, double min_v, double max_v, double tol) {
    if (!in_range(value, min_v, max_v, tol)) {
        return 0.0;
    }
    double target = 0.5 * (min_v + max_v);
    double span = std::abs(max_v - min_v);
    double max_diff = span > 0.0 ? span * 0.5 : std::max(1e-6, std::abs(target) * tol);
    double diff = std::abs(value - target);
    double score = 1.0 - std::min(diff / max_diff, 1.0);
    return std::clamp(score, 0.0, 1.0);
}

}  // namespace

void HarmonicPatternStrategy::initialize(StrategyContext& ctx) {
    ctx_ = &ctx;
    load_symbol_from_config();
    if (auto v = ctx.config().get_as<double>("pivot_threshold_pct")) {
        pivot_threshold_pct_ = std::max(0.001, *v);
    }
    if (auto v = ctx.config().get_as<double>("tolerance")) {
        tolerance_ = std::clamp(*v, 0.0, 0.5);
    }
    if (auto v = ctx.config().get_as<int64_t>("min_bars")) {
        min_bars_ = static_cast<size_t>(std::max<int64_t>(30, *v));
    }
    if (auto v = ctx.config().get_as<int64_t>("cooldown_bars")) {
        cooldown_bars_ = static_cast<size_t>(std::max<int64_t>(0, *v));
    }
    if (auto v = ctx.config().get_as<bool>("use_limit")) {
        use_limit_ = *v;
    }
    if (auto v = ctx.config().get_as<double>("limit_offset_bps")) {
        limit_offset_bps_ = std::max(0.0, *v);
    }
    if (auto v = ctx.config().get_as<double>("vol_threshold_pct")) {
        vol_threshold_pct_ = std::max(0.0, *v);
    }
    if (auto v = ctx.config().get_as<double>("min_confidence")) {
        min_confidence_ = std::clamp(*v, 0.0, 1.0);
    }
    if (auto v = ctx.config().get_as<double>("min_qty_scale")) {
        min_qty_scale_ = std::max(0.1, *v);
    }
    if (auto v = ctx.config().get_as<double>("max_qty_scale")) {
        max_qty_scale_ = std::max(min_qty_scale_, *v);
    }
    if (auto v = ctx.config().get_as<double>("aggressive_confidence_threshold")) {
        aggressive_confidence_threshold_ = std::clamp(*v, 0.0, 1.0);
    }
    if (auto v = ctx.config().get_as<double>("venue_switch_confidence")) {
        venue_switch_confidence_ = std::clamp(*v, 0.0, 1.0);
    }
    if (auto v = ctx.config().get_as<double>("passive_venue_weight")) {
        passive_venue_weight_ = std::clamp(*v, 0.0, 1.0);
    }
    if (auto v = ctx.config().get_as<double>("aggressive_venue_weight")) {
        aggressive_venue_weight_ = std::clamp(*v, 0.0, 1.0);
    }
    if (auto v = ctx.config().get_as<bool>("allow_short")) {
        allow_short_ = *v;
    }
    if (auto v = ctx.config().get_as<int64_t>("order_qty")) {
        order_qty_ = static_cast<Quantity>(std::max<int64_t>(1, *v));
    }
}

bool HarmonicPatternStrategy::load_symbol_from_config() {
    if (!ctx_) {
        return false;
    }
    if (auto v = ctx_->config().get_as<std::string>("symbol")) {
        symbol_ = *v;
    }
    if (!symbol_.empty()) {
        symbol_id_ = SymbolRegistry::instance().intern(symbol_);
        return true;
    }
    return false;
}

std::vector<HarmonicPatternStrategy::Pivot>
HarmonicPatternStrategy::detect_pivots(const std::vector<double>& prices) const {
    std::vector<Pivot> pivots;
    if (prices.size() < 2) {
        return pivots;
    }
    size_t last_idx = 0;
    double last_price = prices[0];
    int trend = 0;
    pivots.push_back({last_idx, last_price});

    for (size_t i = 1; i < prices.size(); ++i) {
        double price = prices[i];
        double change = (price - last_price) / (last_price == 0.0 ? 1.0 : last_price);
        if (trend == 0) {
            if (std::abs(change) >= pivot_threshold_pct_) {
                trend = change > 0 ? 1 : -1;
                last_idx = i;
                last_price = price;
            }
            continue;
        }
        if (trend > 0) {
            if (price > last_price) {
                last_price = price;
                last_idx = i;
            } else if ((last_price - price) / last_price >= pivot_threshold_pct_) {
                pivots.push_back({last_idx, last_price});
                trend = -1;
                last_price = price;
                last_idx = i;
            }
        } else {
            if (price < last_price) {
                last_price = price;
                last_idx = i;
            } else if ((price - last_price) / last_price >= pivot_threshold_pct_) {
                pivots.push_back({last_idx, last_price});
                trend = 1;
                last_price = price;
                last_idx = i;
            }
        }
    }

    if (pivots.empty() || pivots.back().index != last_idx) {
        pivots.push_back({last_idx, last_price});
    }
    return pivots;
}

bool HarmonicPatternStrategy::match_pattern(PatternType type,
                                            double ab_xa,
                                            double bc_ab,
                                            double cd_bc,
                                            double ad_xa,
                                            double cd_xc) const {
    switch (type) {
        case PatternType::Gartley:
            return in_range(ab_xa, 0.618, 0.618, tolerance_) &&
                in_range(bc_ab, 0.382, 0.886, tolerance_) &&
                in_range(cd_bc, 1.27, 1.618, tolerance_) &&
                in_range(ad_xa, 0.786, 0.786, tolerance_);
        case PatternType::Bat:
            return in_range(ab_xa, 0.382, 0.5, tolerance_) &&
                in_range(bc_ab, 0.382, 0.886, tolerance_) &&
                in_range(cd_bc, 1.618, 2.618, tolerance_) &&
                in_range(ad_xa, 0.886, 0.886, tolerance_);
        case PatternType::Butterfly:
            return in_range(ab_xa, 0.786, 0.786, tolerance_) &&
                in_range(bc_ab, 0.382, 0.886, tolerance_) &&
                in_range(cd_bc, 1.618, 2.618, tolerance_) &&
                in_range(ad_xa, 1.27, 1.618, tolerance_);
        case PatternType::Crab:
            return in_range(ab_xa, 0.382, 0.618, tolerance_) &&
                in_range(bc_ab, 0.382, 0.886, tolerance_) &&
                in_range(cd_bc, 2.24, 3.618, tolerance_) &&
                in_range(ad_xa, 1.618, 1.618, tolerance_);
        case PatternType::Cypher:
            return in_range(ab_xa, 0.382, 0.618, tolerance_) &&
                in_range(bc_ab, 1.13, 1.414, tolerance_) &&
                in_range(cd_xc, 0.786, 0.786, tolerance_);
        default:
            return false;
    }
}

double HarmonicPatternStrategy::pattern_confidence(PatternType type,
                                                   double ab_xa,
                                                   double bc_ab,
                                                   double cd_bc,
                                                   double ad_xa,
                                                   double cd_xc) const {
    double score = 0.0;
    int count = 0;
    auto add = [&](double s) {
        score += s;
        ++count;
    };
    switch (type) {
        case PatternType::Gartley:
            add(score_ratio(ab_xa, 0.618, 0.618, tolerance_));
            add(score_ratio(bc_ab, 0.382, 0.886, tolerance_));
            add(score_ratio(cd_bc, 1.27, 1.618, tolerance_));
            add(score_ratio(ad_xa, 0.786, 0.786, tolerance_));
            break;
        case PatternType::Bat:
            add(score_ratio(ab_xa, 0.382, 0.5, tolerance_));
            add(score_ratio(bc_ab, 0.382, 0.886, tolerance_));
            add(score_ratio(cd_bc, 1.618, 2.618, tolerance_));
            add(score_ratio(ad_xa, 0.886, 0.886, tolerance_));
            break;
        case PatternType::Butterfly:
            add(score_ratio(ab_xa, 0.786, 0.786, tolerance_));
            add(score_ratio(bc_ab, 0.382, 0.886, tolerance_));
            add(score_ratio(cd_bc, 1.618, 2.618, tolerance_));
            add(score_ratio(ad_xa, 1.27, 1.618, tolerance_));
            break;
        case PatternType::Crab:
            add(score_ratio(ab_xa, 0.382, 0.618, tolerance_));
            add(score_ratio(bc_ab, 0.382, 0.886, tolerance_));
            add(score_ratio(cd_bc, 2.24, 3.618, tolerance_));
            add(score_ratio(ad_xa, 1.618, 1.618, tolerance_));
            break;
        case PatternType::Cypher:
            add(score_ratio(ab_xa, 0.382, 0.618, tolerance_));
            add(score_ratio(bc_ab, 1.13, 1.414, tolerance_));
            add(score_ratio(cd_xc, 0.786, 0.786, tolerance_));
            break;
        default:
            break;
    }
    return count > 0 ? (score / static_cast<double>(count)) : 0.0;
}

bool HarmonicPatternStrategy::should_use_limit(const data::Bar& bar) const {
    if (!use_limit_) {
        return false;
    }
    if (bar.close <= 0.0) {
        return false;
    }
    double range = bar.high - bar.low;
    double vol = range / bar.close;
    return vol >= vol_threshold_pct_;
}

engine::Order HarmonicPatternStrategy::build_order(engine::OrderSide side,
                                                   const data::Bar& bar,
                                                   PatternType pattern,
                                                   double confidence) const {
    bool use_limit = should_use_limit(bar);
    bool aggressive = confidence >= aggressive_confidence_threshold_;
    if (pattern == PatternType::Butterfly || pattern == PatternType::Crab) {
        aggressive = true;
    }
    std::string venue = "lit_primary";
    if (confidence >= venue_switch_confidence_) {
        venue = aggressive ? "lit_sweep" : "lit_mid";
    }
    double passive_weight = passive_venue_weight_;
    double aggressive_weight = aggressive_venue_weight_;
    if (passive_weight + aggressive_weight > 1.0) {
        double scale = 1.0 / (passive_weight + aggressive_weight);
        passive_weight *= scale;
        aggressive_weight *= scale;
    }
    double lit_weight = 1.0 - passive_weight - aggressive_weight;
    std::string weight_tag = "p:" + std::to_string(passive_weight)
        + "|a:" + std::to_string(aggressive_weight)
        + "|l:" + std::to_string(lit_weight);
    if (use_limit && !aggressive) {
        double offset = limit_offset_bps_ / 10'000.0;
        double price = bar.close;
        if (side == engine::OrderSide::Buy) {
            price = bar.close * (1.0 - offset);
        } else {
            price = bar.close * (1.0 + offset);
        }
        auto order = engine::Order::limit(symbol_id_, side, order_qty_, price);
        order.tif = engine::TimeInForce::Day;
        order.metadata["router"] = "limit";
        order.metadata["route_style"] = "passive";
        order.metadata["venue"] = venue;
        order.metadata["venue_weights"] = weight_tag;
        return order;
    }
    auto order = engine::Order::market(symbol_id_, side, order_qty_);
    order.tif = engine::TimeInForce::IOC;
    order.metadata["router"] = "market";
    order.metadata["route_style"] = "aggressive";
    order.metadata["venue"] = venue;
    order.metadata["venue_weights"] = weight_tag;
    return order;
}

void HarmonicPatternStrategy::on_bar(const data::Bar& bar) {
    if (!ctx_) {
        return;
    }
    if (symbol_id_ == 0) {
        symbol_id_ = bar.symbol;
    }
    if (bar.symbol != symbol_id_) {
        return;
    }
    auto bars = ctx_->recent_bars(symbol_id_, min_bars_);
    if (bars.size() < min_bars_) {
        return;
    }
    if (bars.size() - 1 <= last_signal_index_ + cooldown_bars_) {
        return;
    }

    std::vector<double> closes;
    closes.reserve(bars.size());
    for (const auto& b : bars) {
        closes.push_back(b.close);
    }
    auto pivots = detect_pivots(closes);
    if (pivots.size() < 5) {
        return;
    }
    auto p = pivots[pivots.size() - 5];
    auto a = pivots[pivots.size() - 4];
    auto b = pivots[pivots.size() - 3];
    auto c = pivots[pivots.size() - 2];
    auto d = pivots[pivots.size() - 1];

    double XA = a.price - p.price;
    double AB = b.price - a.price;
    double BC = c.price - b.price;
    double CD = d.price - c.price;
    double XC = c.price - p.price;
    double AD = d.price - p.price;

    double ab_xa = ratio(AB, XA);
    double bc_ab = ratio(BC, AB);
    double cd_bc = ratio(CD, BC);
    double ad_xa = ratio(AD, XA);
    double cd_xc = ratio(CD, XC);

    struct Candidate {
        PatternType type;
        double confidence;
    };
    std::vector<Candidate> candidates;
    const PatternType types[] = {PatternType::Gartley, PatternType::Bat, PatternType::Butterfly,
                                 PatternType::Crab, PatternType::Cypher};
    for (auto type : types) {
        if (!match_pattern(type, ab_xa, bc_ab, cd_bc, ad_xa, cd_xc)) {
            continue;
        }
        double conf = pattern_confidence(type, ab_xa, bc_ab, cd_bc, ad_xa, cd_xc);
        candidates.push_back({type, conf});
    }
    if (candidates.empty()) {
        return;
    }
    auto best = *std::max_element(candidates.begin(), candidates.end(),
                                  [](const Candidate& a, const Candidate& b) {
                                      return a.confidence < b.confidence;
                                  });
    if (best.confidence < min_confidence_) {
        return;
    }

    auto pos = ctx_->portfolio().get_position(symbol_id_);
    Quantity current = pos ? pos->quantity : 0;

    engine::OrderSide side = CD < 0.0 ? engine::OrderSide::Buy : engine::OrderSide::Sell;
    if (side == engine::OrderSide::Sell && !allow_short_ && current <= 0) {
        return;
    }
    if (side == engine::OrderSide::Buy && current >= order_qty_) {
        return;
    }
    if (side == engine::OrderSide::Sell && current <= -order_qty_) {
        return;
    }

    double scale = min_qty_scale_ + (max_qty_scale_ - min_qty_scale_) * best.confidence;
    Quantity scaled_qty = static_cast<Quantity>(std::max(1.0, std::round(order_qty_ * scale)));

    auto order = build_order(side, bar, best.type, best.confidence);
    order.quantity = scaled_qty;
    order.metadata["strategy"] = "harmonic_pattern";
    order.metadata["pattern_count"] = "5";
    order.metadata["confidence"] = std::to_string(best.confidence);
    switch (best.type) {
        case PatternType::Gartley: order.metadata["pattern"] = "gartley"; break;
        case PatternType::Bat: order.metadata["pattern"] = "bat"; break;
        case PatternType::Butterfly: order.metadata["pattern"] = "butterfly"; break;
        case PatternType::Crab: order.metadata["pattern"] = "crab"; break;
        case PatternType::Cypher: order.metadata["pattern"] = "cypher"; break;
    }
    if (ctx_->submit_order(order).is_ok()) {
        last_signal_index_ = bars.size() - 1;
    }
}

}  // namespace regimeflow::strategy
