#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/order_book.h"

#include <deque>
#include <unordered_map>
#include <vector>

namespace regimeflow::regime {

using FeatureVector = std::vector<double>;

enum class FeatureType {
    Return,
    Volatility,
    Volume,
    LogReturn,
    VolumeZScore,
    Range,
    RangeZScore,
    VolumeRatio,
    VolatilityRatio,
    OnBalanceVolume,
    UpDownVolumeRatio,
    BidAskSpread,
    SpreadZScore,
    OrderImbalance,
    MarketBreadth,
    SectorRotation,
    CorrelationEigen,
    RiskAppetite
};

enum class NormalizationMode {
    None,
    ZScore,
    MinMax,
    Robust
};

class FeatureExtractor {
public:
    explicit FeatureExtractor(int window = 20);

    void set_window(int window);
    void set_features(std::vector<FeatureType> features);
    void set_normalize(bool normalize);
    void set_normalization_mode(NormalizationMode mode);
    const std::vector<FeatureType>& features() const { return features_; }
    NormalizationMode normalization_mode() const { return normalization_mode_; }

    FeatureVector on_bar(const data::Bar& bar);
    FeatureVector on_book(const data::OrderBook& book);
    void update_cross_asset_features(double market_breadth, double sector_rotation,
                                     double correlation_eigen, double risk_appetite);

private:
    FeatureVector build_features(double ret, double log_ret, double range, double volume,
                                 double current_vol, double spread, double imbalance);
    double compute_volatility() const;
    double compute_zscore(const std::deque<double>& series, double value) const;
    double normalize_value(FeatureType feature, double value);
    double compute_median(std::vector<double> values) const;
    double compute_percentile(std::vector<double> values, double percentile) const;

    int window_ = 20;
    std::vector<FeatureType> features_;
    double last_close_ = 0.0;
    std::deque<double> returns_;
    std::deque<double> volumes_;
    std::deque<double> ranges_;
    std::deque<double> volatilities_;
    std::deque<double> signed_volumes_;
    std::deque<double> spreads_;
    double obv_ = 0.0;
    NormalizationMode normalization_mode_ = NormalizationMode::None;
    std::unordered_map<FeatureType, std::deque<double>> normalization_history_;
    double market_breadth_ = 0.0;
    double sector_rotation_ = 0.0;
    double correlation_eigen_ = 0.0;
    double risk_appetite_ = 0.0;
};

}  // namespace regimeflow::regime
