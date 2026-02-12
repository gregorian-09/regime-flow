/**
 * @file registry.h
 * @brief RegimeFlow regimeflow registry declarations.
 */

#pragma once

#include "regimeflow/plugins/plugin.h"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::plugins {

/**
 * @brief Registry for static and dynamic plugins.
 */
class PluginRegistry {
public:
    /**
     * @brief Access the singleton registry.
     */
    static PluginRegistry& instance();

    /**
     * @brief Plugin pointer with custom deleter.
     */
    using PluginPtr = std::unique_ptr<Plugin, std::function<void(Plugin*)>>;

    template<typename PluginT>
    /**
     * @brief Register a plugin class for a type/name.
     * @tparam PluginT Plugin concrete type.
     * @param type Plugin category.
     * @param name Plugin name.
     * @return True if registration succeeds.
     */
    bool register_plugin(const std::string& type, const std::string& name) {
        auto factory = []() {
            return PluginPtr(new PluginT(), [](Plugin* p) { delete p; });
        };
        return register_factory(type, name, std::move(factory));
    }

    /**
     * @brief Register a factory for a plugin.
     * @param type Plugin category.
     * @param name Plugin name.
     * @param factory Factory function.
     * @return True if registration succeeds.
     */
    bool register_factory(const std::string& type,
                          const std::string& name,
                          std::function<PluginPtr()> factory);

    template<typename PluginT>
    /**
     * @brief Create and initialize a plugin by type/name.
     * @tparam PluginT Expected plugin type.
     * @param type Plugin category.
     * @param name Plugin name.
     * @param config Plugin configuration.
     * @return Initialized plugin or nullptr.
     */
    std::unique_ptr<PluginT, std::function<void(PluginT*)>> create(
        const std::string& type, const std::string& name, const Config& config = {}) {
        auto plugin = create_plugin(type, name);
        if (!plugin) {
            return nullptr;
        }
        auto* typed = dynamic_cast<PluginT*>(plugin.get());
        if (!typed) {
            return nullptr;
        }
        if (auto schema = plugin->config_schema()) {
            auto normalized = ::regimeflow::apply_defaults(config, *schema);
            if (auto res = ::regimeflow::validate_config(normalized, *schema); res.is_err()) {
                plugin->set_state(PluginState::Error);
                return nullptr;
            }
            plugin->set_state(PluginState::Loaded);
            if (auto res = plugin->on_initialize(normalized); res.is_err()) {
                plugin->set_state(PluginState::Error);
                return nullptr;
            }
        } else {
            if (auto res = plugin->on_initialize(config); res.is_err()) {
                plugin->set_state(PluginState::Error);
                return nullptr;
            }
        }
        auto deleter = plugin.get_deleter();
        plugin.release();
        auto result = std::unique_ptr<PluginT, std::function<void(PluginT*)>>(
            typed, [deleter](PluginT* p) { deleter(p); });
        result->set_state(PluginState::Initialized);
        return result;
    }

    /**
     * @brief List registered plugin types.
     */
    std::vector<std::string> list_types() const;
    /**
     * @brief List plugins for a type.
     */
    std::vector<std::string> list_plugins(const std::string& type) const;
    /**
     * @brief Get plugin metadata.
     */
    std::optional<PluginInfo> get_info(const std::string& type,
                                       const std::string& name) const;

    /**
     * @brief Load a dynamic plugin library.
     */
    Result<void> load_dynamic_plugin(const std::string& path);
    /**
     * @brief Unload a dynamic plugin by name.
     */
    Result<void> unload_dynamic_plugin(const std::string& name);
    /**
     * @brief Scan a directory for plugins.
     */
    void scan_plugin_directory(const std::string& path);

    /**
     * @brief Start a plugin (invoke on_start).
     */
    Result<void> start_plugin(Plugin& plugin);
    /**
     * @brief Stop a plugin (invoke on_stop).
     */
    Result<void> stop_plugin(Plugin& plugin);

private:
    PluginRegistry() = default;

    using PluginFactory = std::function<PluginPtr()>;

    PluginPtr create_plugin(const std::string& type,
                            const std::string& name) const;

    /**
     * @brief Metadata for a dynamically loaded plugin.
     */
    struct DynamicPlugin {
        void* handle = nullptr;
        std::function<Plugin*()> create;
        std::function<void(Plugin*)> destroy;
        std::string type;
        std::string name;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string,
        std::unordered_map<std::string, PluginFactory>> factories_;
    std::unordered_map<std::string, DynamicPlugin> dynamic_plugins_;
};

#define REGISTER_PLUGIN(PluginClass, plugin_type, plugin_name) \
    namespace { \
    static bool _registered_##PluginClass = \
        ::regimeflow::plugins::PluginRegistry::instance().register_plugin<PluginClass>( \
            #plugin_type, #plugin_name); \
    }

}  // namespace regimeflow::plugins
