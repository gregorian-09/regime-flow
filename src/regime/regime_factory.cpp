#include "regimeflow/regime/regime_factory.h"

#include "regimeflow/regime/hmm.h"
#include "regimeflow/regime/ensemble.h"
#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/registry.h"

#include <string>

namespace regimeflow::regime {

std::unique_ptr<RegimeDetector> RegimeFactory::create_detector(const Config& config) {
    auto type = config.get_as<std::string>("detector")
        .value_or(config.get_as<std::string>("type").value_or("constant"));
    if (type == "constant") {
        auto regime_name = config.get_as<std::string>("regime").value_or("neutral");
        RegimeType regime = RegimeType::Neutral;
        if (regime_name == "bull") regime = RegimeType::Bull;
        else if (regime_name == "bear") regime = RegimeType::Bear;
        else if (regime_name == "crisis") regime = RegimeType::Crisis;
        return std::make_unique<ConstantRegimeDetector>(regime);
    }
    if (type == "hmm") {
        int states = static_cast<int>(config.get_as<int64_t>("hmm.states").value_or(4));
        int window = static_cast<int>(config.get_as<int64_t>("hmm.window").value_or(20));
        auto detector = std::make_unique<HMMRegimeDetector>(states, window);

        if (auto features = config.get_as<ConfigValue::Array>("hmm.features")) {
            std::vector<FeatureType> feature_list;
            for (const auto& value : *features) {
                if (const auto* str = value.get_if<std::string>()) {
                    if (*str == "return") feature_list.push_back(FeatureType::Return);
                    else if (*str == "volatility") feature_list.push_back(FeatureType::Volatility);
                    else if (*str == "volume") feature_list.push_back(FeatureType::Volume);
                    else if (*str == "log_return") feature_list.push_back(FeatureType::LogReturn);
                    else if (*str == "volume_zscore") feature_list.push_back(FeatureType::VolumeZScore);
                    else if (*str == "range") feature_list.push_back(FeatureType::Range);
                    else if (*str == "range_zscore") feature_list.push_back(FeatureType::RangeZScore);
                    else if (*str == "volume_ratio") feature_list.push_back(FeatureType::VolumeRatio);
                    else if (*str == "volatility_ratio") {
                        feature_list.push_back(FeatureType::VolatilityRatio);
                    } else if (*str == "obv") {
                        feature_list.push_back(FeatureType::OnBalanceVolume);
                    } else if (*str == "up_down_volume_ratio") {
                        feature_list.push_back(FeatureType::UpDownVolumeRatio);
                    } else if (*str == "bid_ask_spread") {
                        feature_list.push_back(FeatureType::BidAskSpread);
                    } else if (*str == "spread_zscore") {
                        feature_list.push_back(FeatureType::SpreadZScore);
                    } else if (*str == "order_imbalance") {
                        feature_list.push_back(FeatureType::OrderImbalance);
                    }
                }
            }
            if (!feature_list.empty()) {
                detector->set_features(std::move(feature_list));
            }
        }

        if (auto normalize = config.get_as<bool>("hmm.normalize_features")) {
            detector->set_normalize_features(*normalize);
        }
        if (auto norm_mode = config.get_as<std::string>("hmm.normalization")) {
            if (*norm_mode == "zscore") {
                detector->set_normalization_mode(NormalizationMode::ZScore);
            } else if (*norm_mode == "minmax") {
                detector->set_normalization_mode(NormalizationMode::MinMax);
            } else if (*norm_mode == "robust") {
                detector->set_normalization_mode(NormalizationMode::Robust);
            } else {
                detector->set_normalization_mode(NormalizationMode::None);
            }
        }

        std::vector<GaussianParams> params;
        params.resize(states);
        for (int i = 0; i < states; ++i) {
            double mean_ret = config.get_as<double>("hmm.state" + std::to_string(i) + ".mean_return")
                .value_or(0.0);
            double mean_vol = config.get_as<double>("hmm.state" + std::to_string(i) + ".mean_vol")
                .value_or(0.01);
            double var_ret = config.get_as<double>("hmm.state" + std::to_string(i) + ".var_return")
                .value_or(1e-6);
            double var_vol = config.get_as<double>("hmm.state" + std::to_string(i) + ".var_vol")
                .value_or(1e-4);
            params[i].mean = {mean_ret, mean_vol};
            params[i].variance = {var_ret, var_vol};
        }
        detector->set_emission_params(std::move(params));
        return detector;
    }
    if (type == "ensemble") {
        auto ensemble = std::make_unique<EnsembleRegimeDetector>();
        if (auto voting = config.get_as<std::string>("ensemble.voting_method")) {
            if (*voting == "majority") {
                ensemble->set_voting_method(EnsembleRegimeDetector::VotingMethod::Majority);
            } else if (*voting == "confidence_weighted") {
                ensemble->set_voting_method(
                    EnsembleRegimeDetector::VotingMethod::ConfidenceWeighted);
            } else if (*voting == "bayesian") {
                ensemble->set_voting_method(EnsembleRegimeDetector::VotingMethod::Bayesian);
            } else {
                ensemble->set_voting_method(EnsembleRegimeDetector::VotingMethod::WeightedAverage);
            }
        }
        if (auto detectors = config.get_as<ConfigValue::Array>("ensemble.detectors")) {
            for (const auto& entry : *detectors) {
                const auto* obj = entry.get_if<ConfigValue::Object>();
                if (!obj) {
                    continue;
                }
                Config det_cfg(*obj);
                auto det = RegimeFactory::create_detector(det_cfg);
                double weight = det_cfg.get_as<double>("weight").value_or(1.0);
                if (det) {
                    ensemble->add_detector(std::move(det), weight);
                }
            }
        }
        return ensemble;
    }
    Config plugin_cfg = config;
    if (auto params = config.get_as<ConfigValue::Object>("params")) {
        plugin_cfg = Config(*params);
    }
    auto plugin = plugins::PluginRegistry::instance()
        .create<plugins::RegimeDetectorPlugin>("regime_detector", type, plugin_cfg);
    if (plugin) {
        return plugin->create_detector();
    }
    return nullptr;
}

}  // namespace regimeflow::regime
