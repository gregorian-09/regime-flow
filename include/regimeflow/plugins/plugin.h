/**
 * @file plugin.h
 * @brief RegimeFlow regimeflow plugin declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/common/config_schema.h"
#include "regimeflow/common/result.h"

#include <string>
#include <vector>

namespace regimeflow::plugins {

/**
 * @brief Metadata describing a plugin.
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> dependencies;
};

/**
 * @brief Plugin lifecycle state.
 */
enum class PluginState {
    Unloaded,
    Loaded,
    Initialized,
    Active,
    Stopped,
    Error
};

/**
 * @brief Base class for all plugins.
 */
class Plugin {
public:
    virtual ~Plugin() = default;

    /**
     * @brief Return plugin metadata.
     */
    virtual PluginInfo info() const = 0;

    /**
     * @brief Called when the plugin is loaded.
     */
    virtual Result<void> on_load() { return Ok(); }
    /**
     * @brief Called when the plugin is unloaded.
     */
    virtual Result<void> on_unload() { return Ok(); }
    /**
     * @brief Called when plugin is initialized with config.
     * @param config Plugin configuration.
     */
    virtual Result<void> on_initialize([[maybe_unused]] const Config& config) { return Ok(); }
    /**
     * @brief Called when plugin is started.
     */
    virtual Result<void> on_start() { return Ok(); }
    /**
     * @brief Called when plugin is stopped.
     */
    virtual Result<void> on_stop() { return Ok(); }

    /**
     * @brief Optional config schema for validation.
     */
    virtual std::optional<ConfigSchema> config_schema() const { return std::nullopt; }

    /**
     * @brief Current plugin state.
     */
    PluginState state() const { return state_; }
    /**
     * @brief Set plugin state.
     */
    void set_state(PluginState state) { state_ = state; }

protected:
    PluginState state_ = PluginState::Unloaded;
};

#if defined(_WIN32)
#define REGIMEFLOW_EXPORT __declspec(dllexport)
#else
#define REGIMEFLOW_EXPORT __attribute__((visibility("default")))
#endif

#define REGIMEFLOW_ABI_VERSION "1.0"

}  // namespace regimeflow::plugins
