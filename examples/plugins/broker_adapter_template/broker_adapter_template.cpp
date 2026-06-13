#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/plugin.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
    class BrokerAdapterTemplate final : public regimeflow::live::BrokerAdapter {
    public:
        explicit BrokerAdapterTemplate(std::string broker_name)
            : broker_name_(std::move(broker_name)) {}

        regimeflow::Result<void> connect() override {
            connected_ = true;
            return regimeflow::Ok();
        }

        regimeflow::Result<void> disconnect() override {
            connected_ = false;
            return regimeflow::Ok();
        }

        [[nodiscard]] bool is_connected() const override { return connected_; }

        void subscribe_market_data(const std::vector<std::string>& symbols) override {
            subscribed_symbols_ = symbols;
        }

        void unsubscribe_market_data(const std::vector<std::string>&) override {
            subscribed_symbols_.clear();
        }

        regimeflow::Result<std::string> submit_order(const regimeflow::engine::Order& order) override {
            if (!connected_) {
                return regimeflow::Result<std::string>(regimeflow::Error(
                    regimeflow::Error::Code::InvalidState, "Broker adapter template is disconnected"));
            }
            return regimeflow::Result<std::string>("template-" + std::to_string(order.id));
        }

        regimeflow::Result<void> cancel_order(const std::string&) override { return regimeflow::Ok(); }

        regimeflow::Result<void> modify_order(const std::string&, const regimeflow::engine::OrderModification&) override {
            return regimeflow::Ok();
        }

        regimeflow::live::AccountInfo get_account_info() override {
            regimeflow::live::AccountInfo account;
            account.equity = 100000.0;
            account.cash = 100000.0;
            account.buying_power = 100000.0;
            return account;
        }

        std::vector<regimeflow::live::Position> get_positions() override { return {}; }
        std::vector<regimeflow::live::ExecutionReport> get_open_orders() override { return {}; }

        void on_market_data(std::function<void(const regimeflow::live::MarketDataUpdate&)> cb) override {
            market_cb_ = std::move(cb);
        }

        void on_execution_report(std::function<void(const regimeflow::live::ExecutionReport&)> cb) override {
            execution_cb_ = std::move(cb);
        }

        void on_position_update(std::function<void(const regimeflow::live::Position&)> cb) override {
            position_cb_ = std::move(cb);
        }

        [[nodiscard]] int max_orders_per_second() const override { return 1; }
        [[nodiscard]] int max_messages_per_second() const override { return 10; }

        [[nodiscard]] bool supports_tif(regimeflow::engine::OrderType type,
                                        regimeflow::engine::TimeInForce tif) const override {
            return capabilities().supports(type, tif);
        }

        [[nodiscard]] regimeflow::live::BrokerCapabilities capabilities() const override {
            return regimeflow::live::BrokerCapabilities{
                broker_name_,
                {regimeflow::AssetClass::Equity},
                {{regimeflow::engine::OrderType::Market, {regimeflow::engine::TimeInForce::Day}},
                 {regimeflow::engine::OrderType::Limit, {regimeflow::engine::TimeInForce::Day,
                                                         regimeflow::engine::TimeInForce::GTC}}},
                false,
                false,
                false,
                false,
                1,
                10};
        }

        void poll() override {}

    private:
        std::string broker_name_;
        bool connected_ = false;
        std::vector<std::string> subscribed_symbols_;
        std::function<void(const regimeflow::live::MarketDataUpdate&)> market_cb_;
        std::function<void(const regimeflow::live::ExecutionReport&)> execution_cb_;
        std::function<void(const regimeflow::live::Position&)> position_cb_;
    };

    class BrokerAdapterTemplatePlugin final : public regimeflow::plugins::BrokerAdapterPlugin {
    public:
        [[nodiscard]] regimeflow::plugins::PluginInfo info() const override {
            return {"broker_adapter_template", "0.1.0", "Minimal broker adapter plugin template", "RegimeFlow", {}};
        }

        regimeflow::Result<void> on_load() override {
            set_state(regimeflow::plugins::PluginState::Loaded);
            return regimeflow::Ok();
        }

        regimeflow::Result<void> on_initialize(const regimeflow::Config& config) override {
            broker_name_ = config.get_as<std::string>("broker_type").value_or("broker_adapter_template");
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

        std::unique_ptr<regimeflow::live::BrokerAdapter> create_broker_adapter() override {
            return std::make_unique<BrokerAdapterTemplate>(broker_name_);
        }

    private:
        std::string broker_name_ = "broker_adapter_template";
    };
}  // namespace

extern "C" REGIMEFLOW_EXPORT regimeflow::plugins::Plugin* create_plugin() {
    return new BrokerAdapterTemplatePlugin();
}

extern "C" REGIMEFLOW_EXPORT void destroy_plugin(regimeflow::plugins::Plugin* plugin) {
    delete plugin;
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_type() {
    return "broker_adapter";
}

extern "C" REGIMEFLOW_EXPORT const char* plugin_name() {
    return "broker_adapter_template";
}

extern "C" REGIMEFLOW_EXPORT const char* regimeflow_abi_version() {
    return REGIMEFLOW_ABI_VERSION;
}
