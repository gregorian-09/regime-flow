#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"
#include "regimeflow/regime/regime_detector.h"

#include <memory>
#include <utility>

namespace {
    class RegimeDetectorTemplate final : public regimeflow::regime::RegimeDetector {
    public:
        regimeflow::regime::RegimeState on_bar(const regimeflow::data::Bar& bar) override {
            return state_for(bar.timestamp);
        }

        regimeflow::regime::RegimeState on_tick(const regimeflow::data::Tick& tick) override {
            return state_for(tick.timestamp);
        }

        void configure(const regimeflow::Config& config) override {
            (void)config;
            metadata_.detector_type = "regime_detector_template";
            metadata_.model_version = "0.1.0";
            metadata_.feature_schema = "template:v1";
        }

        [[nodiscard]] int num_states() const override { return 1; }

        [[nodiscard]] std::vector<std::string> state_names() const override {
            return {"neutral"};
        }

        [[nodiscard]] regimeflow::regime::ModelGovernanceMetadata model_metadata() const override {
            return metadata_;
        }

        void set_model_metadata(regimeflow::regime::ModelGovernanceMetadata metadata) override {
            metadata_ = std::move(metadata);
        }

    private:
        regimeflow::regime::ModelGovernanceMetadata metadata_{
            "regime_detector_template", "0.1.0", 0, 0, "template:v1", {}};

        [[nodiscard]] static regimeflow::regime::RegimeState state_for(const regimeflow::Timestamp timestamp) {
            regimeflow::regime::RegimeState state;
            state.regime = regimeflow::regime::RegimeType::Neutral;
            state.confidence = 1.0;
            state.timestamp = timestamp;
            return state;
        }
    };

    class RegimeDetectorTemplatePlugin final : public regimeflow::plugins::RegimeDetectorPlugin {
    public:
        [[nodiscard]] regimeflow::plugins::PluginInfo info() const override {
            return {"regime_detector_template", "0.1.0", "Minimal regime detector plugin template", "RegimeFlow", {}};
        }

        regimeflow::Result<void> on_load() override {
            set_state(regimeflow::plugins::PluginState::Loaded);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override {
            config_ = config;
            set_state(regimeflow::plugins::PluginState::Initialized);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_start() override {
            set_state(regimeflow::plugins::PluginState::Active);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_stop() override {
            set_state(regimeflow::plugins::PluginState::Stopped);
            return regimeflow::Ok();
        }

        std::unique_ptr<regimeflow::regime::RegimeDetector> create_detector() override {
            auto detector = std::make_unique<RegimeDetectorTemplate>();
            detector->configure(config_);
            return detector;
        }

    private:
        regimeflow::Config config_;
    };
}  // namespace

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new RegimeDetectorTemplatePlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "regime_detector";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "regime_detector_template";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
