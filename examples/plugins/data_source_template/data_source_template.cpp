#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
    class VectorBarIterator final : public regimeflow::data::DataIterator {
    public:
        explicit VectorBarIterator(std::vector<regimeflow::data::Bar> bars)
            : bars_(std::move(bars)) {}

        [[nodiscard]] bool has_next() const override {
            return index_ < bars_.size();
        }

        regimeflow::data::Bar next() override {
            return bars_.at(index_++);
        }

        void reset() override {
            index_ = 0;
        }

    private:
        std::vector<regimeflow::data::Bar> bars_;
        std::size_t index_ = 0;
    };

    class DataSourceTemplate final : public regimeflow::data::DataSource {
    public:
        explicit DataSourceTemplate(std::string symbol)
            : symbol_(regimeflow::SymbolRegistry::instance().intern(symbol)) {
            bars_.push_back(regimeflow::data::Bar{
                regimeflow::Timestamp(1'700'000'000'000'000), symbol_, 100.0, 101.0, 99.5, 100.5, 1'000, 10, 100.25});
            bars_.push_back(regimeflow::data::Bar{
                regimeflow::Timestamp(1'700'086'400'000'000), symbol_, 100.5, 102.0, 100.0, 101.75, 1'200, 12, 101.1});
        }

        [[nodiscard]] std::vector<regimeflow::data::SymbolInfo> get_available_symbols() const override {
            return {regimeflow::data::SymbolInfo{
                symbol_, regimeflow::SymbolRegistry::instance().lookup(symbol_), "TEMPLATE", regimeflow::AssetClass::Equity,
                "USD", 0.01, 1.0, 1.0, get_available_range(symbol_), "", ""}};
        }

        [[nodiscard]] regimeflow::TimeRange get_available_range(regimeflow::SymbolId symbol) const override {
            if (symbol != symbol_ || bars_.empty()) {
                return {};
            }
            return {bars_.front().timestamp, bars_.back().timestamp};
        }

        std::vector<regimeflow::data::Bar> get_bars(regimeflow::SymbolId symbol,
                                                    regimeflow::TimeRange range,
                                                    regimeflow::data::BarType) override {
            std::vector<regimeflow::data::Bar> out;
            for (const auto& bar : bars_) {
                if (bar.symbol == symbol && range.contains(bar.timestamp)) {
                    out.push_back(bar);
                }
            }
            return out;
        }

        std::vector<regimeflow::data::Tick> get_ticks(regimeflow::SymbolId, regimeflow::TimeRange) override {
            return {};
        }

        std::unique_ptr<regimeflow::data::DataIterator> create_iterator(
            const std::vector<regimeflow::SymbolId>& symbols,
            regimeflow::TimeRange range,
            regimeflow::data::BarType bar_type) override {
            std::vector<regimeflow::data::Bar> out;
            for (const auto symbol : symbols) {
                auto bars = get_bars(symbol, range, bar_type);
                out.insert(out.end(), bars.begin(), bars.end());
            }
            std::sort(out.begin(), out.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.timestamp < rhs.timestamp;
            });
            return std::make_unique<VectorBarIterator>(std::move(out));
        }

        std::vector<regimeflow::data::CorporateAction> get_corporate_actions(regimeflow::SymbolId,
                                                                              regimeflow::TimeRange) override {
            return {};
        }

    private:
        regimeflow::SymbolId symbol_ = 0;
        std::vector<regimeflow::data::Bar> bars_;
    };

    class DataSourceTemplatePlugin final : public regimeflow::plugins::DataSourcePlugin {
    public:
        [[nodiscard]] regimeflow::plugins::PluginInfo info() const override {
            return {"data_source_template", "0.1.0", "Minimal data source plugin template", "RegimeFlow", {}};
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

        std::unique_ptr<regimeflow::data::DataSource> create_data_source() override {
            auto symbol = config_.get_as<std::string>("symbol").value_or("AAPL");
            return std::make_unique<DataSourceTemplate>(std::move(symbol));
        }

    private:
        regimeflow::Config config_;
    };
}  // namespace

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new DataSourceTemplatePlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "data_source";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "data_source_template";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
