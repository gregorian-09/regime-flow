#include "regimeflow/regime/features.h"

#include <algorithm>
#include <cmath>

namespace regimeflow::regime {

FeatureExtractor::FeatureExtractor(int window) : window_(window) {}

void FeatureExtractor::set_window(int window) {
    window_ = window;
    returns_.clear();
    volumes_.clear();
    ranges_.clear();
    volatilities_.clear();
    signed_volumes_.clear();
    spreads_.clear();
    obv_ = 0.0;
    normalization_history_.clear();
}

void FeatureExtractor::set_features(std::vector<FeatureType> features) {
    features_ = std::move(features);
}

void FeatureExtractor::set_normalize(bool normalize) {
    normalization_mode_ = normalize ? NormalizationMode::ZScore : NormalizationMode::None;
}

void FeatureExtractor::set_normalization_mode(NormalizationMode mode) {
    normalization_mode_ = mode;
}

void FeatureExtractor::update_cross_asset_features(double market_breadth,
                                                   double sector_rotation,
                                                   double correlation_eigen,
                                                   double risk_appetite) {
    market_breadth_ = market_breadth;
    sector_rotation_ = sector_rotation;
    correlation_eigen_ = correlation_eigen;
    risk_appetite_ = risk_appetite;
}

FeatureVector FeatureExtractor::on_bar(const data::Bar& bar) {
    double ret = 0.0;
    if (last_close_ > 0.0) {
        ret = (bar.close - last_close_) / last_close_;
    }
    double log_ret = 0.0;
    if (last_close_ > 0.0 && bar.close > 0.0) {
        log_ret = std::log(bar.close / last_close_);
    }
    last_close_ = bar.close;
    double range = bar.high - bar.low;

    returns_.push_back(ret);
    volumes_.push_back(static_cast<double>(bar.volume));
    ranges_.push_back(range);
    double current_vol = compute_volatility();
    volatilities_.push_back(current_vol);
    double signed_volume = 0.0;
    if (ret > 0.0) {
        signed_volume = static_cast<double>(bar.volume);
    } else if (ret < 0.0) {
        signed_volume = -static_cast<double>(bar.volume);
    }
    signed_volumes_.push_back(signed_volume);
    obv_ += signed_volume;
    if (static_cast<int>(returns_.size()) > window_) {
        returns_.pop_front();
    }
    if (static_cast<int>(volumes_.size()) > window_) {
        volumes_.pop_front();
    }
    if (static_cast<int>(ranges_.size()) > window_) {
        ranges_.pop_front();
    }
    if (static_cast<int>(volatilities_.size()) > window_) {
        volatilities_.pop_front();
    }
    if (static_cast<int>(signed_volumes_.size()) > window_) {
        signed_volumes_.pop_front();
    }

    return build_features(ret, log_ret, range, static_cast<double>(bar.volume), current_vol, 0.0,
                          0.0);
}

FeatureVector FeatureExtractor::on_book(const data::OrderBook& book) {
    double bid = book.bids[0].price;
    double ask = book.asks[0].price;
    double bid_qty = book.bids[0].quantity;
    double ask_qty = book.asks[0].quantity;
    double mid = (bid + ask) / 2.0;
    double spread = mid > 0.0 ? (ask - bid) / mid : 0.0;
    double imbalance = (bid_qty + ask_qty) > 0.0
        ? (bid_qty - ask_qty) / (bid_qty + ask_qty)
        : 0.0;

    data::Bar bar{};
    bar.timestamp = book.timestamp;
    bar.symbol = book.symbol;
    bar.open = mid;
    bar.high = mid;
    bar.low = mid;
    bar.close = mid;
    bar.volume = static_cast<Volume>(bid_qty + ask_qty);

    double ret = 0.0;
    if (last_close_ > 0.0) {
        ret = (bar.close - last_close_) / last_close_;
    }
    double log_ret = 0.0;
    if (last_close_ > 0.0 && bar.close > 0.0) {
        log_ret = std::log(bar.close / last_close_);
    }
    last_close_ = bar.close;
    double range = 0.0;

    returns_.push_back(ret);
    volumes_.push_back(static_cast<double>(bar.volume));
    ranges_.push_back(range);
    double current_vol = compute_volatility();
    volatilities_.push_back(current_vol);
    double signed_volume = 0.0;
    if (ret > 0.0) {
        signed_volume = static_cast<double>(bar.volume);
    } else if (ret < 0.0) {
        signed_volume = -static_cast<double>(bar.volume);
    }
    signed_volumes_.push_back(signed_volume);
    obv_ += signed_volume;
    spreads_.push_back(spread);
    if (static_cast<int>(returns_.size()) > window_) {
        returns_.pop_front();
    }
    if (static_cast<int>(volumes_.size()) > window_) {
        volumes_.pop_front();
    }
    if (static_cast<int>(ranges_.size()) > window_) {
        ranges_.pop_front();
    }
    if (static_cast<int>(volatilities_.size()) > window_) {
        volatilities_.pop_front();
    }
    if (static_cast<int>(signed_volumes_.size()) > window_) {
        signed_volumes_.pop_front();
    }
    if (static_cast<int>(spreads_.size()) > window_) {
        spreads_.pop_front();
    }

    return build_features(ret, log_ret, range, static_cast<double>(bar.volume), current_vol,
                          spread, imbalance);
}

FeatureVector FeatureExtractor::build_features(double ret, double log_ret, double range,
                                               double volume, double current_vol,
                                               double spread, double imbalance) {
    if (features_.empty()) {
        features_ = {FeatureType::Return, FeatureType::Volatility};
    }

    FeatureVector values;
    values.reserve(features_.size());
    for (auto feature : features_) {
        double value = 0.0;
        switch (feature) {
            case FeatureType::Return:
                value = ret;
                break;
            case FeatureType::LogReturn:
                value = log_ret;
                break;
            case FeatureType::Volatility:
                value = current_vol;
                break;
            case FeatureType::Volume:
                value = volume;
                break;
            case FeatureType::VolumeZScore:
                value = compute_zscore(volumes_, volumes_.empty() ? 0.0 : volumes_.back());
                break;
            case FeatureType::Range:
                value = range;
                break;
            case FeatureType::RangeZScore:
                value = compute_zscore(ranges_, range);
                break;
            case FeatureType::VolumeRatio: {
                double mean = 0.0;
                for (double v : volumes_) mean += v;
                mean /= volumes_.empty() ? 1.0 : static_cast<double>(volumes_.size());
                value = mean > 0.0 ? (volumes_.back() / mean) : 0.0;
                break;
            }
            case FeatureType::VolatilityRatio: {
                double mean = 0.0;
                for (double v : volatilities_) mean += v;
                mean /= volatilities_.empty() ? 1.0 : static_cast<double>(volatilities_.size());
                value = mean > 0.0 ? (current_vol / mean) : 0.0;
                break;
            }
            case FeatureType::OnBalanceVolume:
                value = obv_;
                break;
            case FeatureType::UpDownVolumeRatio: {
                double up = 0.0;
                double total = 0.0;
                for (double v : signed_volumes_) {
                    if (v > 0) up += v;
                    total += std::abs(v);
                }
                value = total > 0.0 ? (up / total) : 0.0;
                break;
            }
            case FeatureType::BidAskSpread:
                value = spread;
                break;
            case FeatureType::SpreadZScore:
                value = compute_zscore(spreads_, spread);
                break;
            case FeatureType::OrderImbalance:
                value = imbalance;
                break;
            case FeatureType::MarketBreadth:
                value = market_breadth_;
                break;
            case FeatureType::SectorRotation:
                value = sector_rotation_;
                break;
            case FeatureType::CorrelationEigen:
                value = correlation_eigen_;
                break;
            case FeatureType::RiskAppetite:
                value = risk_appetite_;
                break;
        }
        value = normalize_value(feature, value);
        values.push_back(value);
    }
    return values;
}

double FeatureExtractor::compute_volatility() const {
    if (returns_.size() < 2) {
        return 0.0;
    }
    double mean = 0.0;
    for (double r : returns_) {
        mean += r;
    }
    mean /= static_cast<double>(returns_.size());

    double var = 0.0;
    for (double r : returns_) {
        double diff = r - mean;
        var += diff * diff;
    }
    var /= static_cast<double>(returns_.size() - 1);
    return std::sqrt(var);
}

double FeatureExtractor::compute_zscore(const std::deque<double>& series, double value) const {
    if (series.size() < 2) {
        return 0.0;
    }
    double mean = 0.0;
    for (double v : series) {
        mean += v;
    }
    mean /= static_cast<double>(series.size());
    double var = 0.0;
    for (double v : series) {
        double diff = v - mean;
        var += diff * diff;
    }
    var /= static_cast<double>(series.size() - 1);
    double stddev = std::sqrt(var);
    if (stddev == 0.0) {
        return 0.0;
    }
    return (value - mean) / stddev;
}

double FeatureExtractor::normalize_value(FeatureType feature, double value) {
    if (normalization_mode_ == NormalizationMode::None) {
        return value;
    }
    auto& series = normalization_history_[feature];
    series.push_back(value);
    if (static_cast<int>(series.size()) > window_) {
        series.pop_front();
    }
    if (series.size() < 2) {
        return 0.0;
    }
    std::vector<double> sample(series.begin(), series.end());
    switch (normalization_mode_) {
        case NormalizationMode::ZScore: {
            double mean = 0.0;
            for (double v : sample) mean += v;
            mean /= static_cast<double>(sample.size());
            double var = 0.0;
            for (double v : sample) {
                double diff = v - mean;
                var += diff * diff;
            }
            double stddev = sample.size() > 1 ? std::sqrt(var / (sample.size() - 1)) : 1.0;
            if (stddev == 0.0) {
                return 0.0;
            }
            return (value - mean) / stddev;
        }
        case NormalizationMode::MinMax: {
            auto [min_it, max_it] = std::minmax_element(sample.begin(), sample.end());
            double range = *max_it - *min_it;
            if (range == 0.0) {
                return 0.0;
            }
            return (value - *min_it) / range;
        }
        case NormalizationMode::Robust: {
            double median = compute_median(sample);
            double q1 = compute_percentile(sample, 25.0);
            double q3 = compute_percentile(sample, 75.0);
            double iqr = q3 - q1;
            if (iqr == 0.0) {
                return 0.0;
            }
            return (value - median) / iqr;
        }
        case NormalizationMode::None:
        default:
            return value;
    }
}

double FeatureExtractor::compute_median(std::vector<double> values) const {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    size_t mid = values.size() / 2;
    if (values.size() % 2 == 0) {
        return (values[mid - 1] + values[mid]) / 2.0;
    }
    return values[mid];
}

double FeatureExtractor::compute_percentile(std::vector<double> values, double percentile) const {
    if (values.empty()) {
        return 0.0;
    }
    if (percentile <= 0.0) {
        return *std::min_element(values.begin(), values.end());
    }
    if (percentile >= 100.0) {
        return *std::max_element(values.begin(), values.end());
    }
    std::sort(values.begin(), values.end());
    double rank = (percentile / 100.0) * (values.size() - 1);
    size_t low = static_cast<size_t>(std::floor(rank));
    size_t high = static_cast<size_t>(std::ceil(rank));
    if (low == high) {
        return values[low];
    }
    double weight = rank - static_cast<double>(low);
    return values[low] * (1.0 - weight) + values[high] * weight;
}

}  // namespace regimeflow::regime
