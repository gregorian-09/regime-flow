#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/common/config_schema.h"
#include "regimeflow/common/result.h"

#include <string>
#include <vector>

namespace regimeflow::plugins {

struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> dependencies;
};

enum class PluginState {
    Unloaded,
    Loaded,
    Initialized,
    Active,
    Stopped,
    Error
};

class Plugin {
public:
    virtual ~Plugin() = default;

    virtual PluginInfo info() const = 0;

    virtual Result<void> on_load() { return Ok(); }
    virtual Result<void> on_unload() { return Ok(); }
    virtual Result<void> on_initialize([[maybe_unused]] const Config& config) { return Ok(); }
    virtual Result<void> on_start() { return Ok(); }
    virtual Result<void> on_stop() { return Ok(); }

    virtual std::optional<ConfigSchema> config_schema() const { return std::nullopt; }

    PluginState state() const { return state_; }
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
