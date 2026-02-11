#include "regimeflow/plugins/registry.h"

#if !defined(_WIN32)
#include <dlfcn.h>
#endif
#include <filesystem>

namespace regimeflow::plugins {

PluginRegistry& PluginRegistry::instance() {
    static PluginRegistry registry;
    return registry;
}

bool PluginRegistry::register_factory(const std::string& type,
                                      const std::string& name,
                                      std::function<PluginPtr()> factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& type_map = factories_[type];
    if (type_map.find(name) != type_map.end()) {
        return false;
    }
    type_map[name] = std::move(factory);
    return true;
}

PluginRegistry::PluginPtr PluginRegistry::create_plugin(const std::string& type,
                                                        const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto type_it = factories_.find(type);
    if (type_it == factories_.end()) {
        return nullptr;
    }
    auto plugin_it = type_it->second.find(name);
    if (plugin_it == type_it->second.end()) {
        return nullptr;
    }
    auto plugin = plugin_it->second();
    if (!plugin) {
        return nullptr;
    }
    if (auto res = plugin->on_load(); res.is_err()) {
        return nullptr;
    }
    plugin->set_state(PluginState::Loaded);
    return plugin;
}

std::vector<std::string> PluginRegistry::list_types() const {
    std::vector<std::string> types;
    std::lock_guard<std::mutex> lock(mutex_);
    types.reserve(factories_.size());
    for (const auto& [type, entries] : factories_) {
        types.push_back(type);
    }
    return types;
}

std::vector<std::string> PluginRegistry::list_plugins(const std::string& type) const {
    std::vector<std::string> names;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = factories_.find(type);
    if (it == factories_.end()) {
        return names;
    }
    names.reserve(it->second.size());
    for (const auto& [name, factory] : it->second) {
        names.push_back(name);
    }
    return names;
}

std::optional<PluginInfo> PluginRegistry::get_info(const std::string& type,
                                                   const std::string& name) const {
    auto plugin = create_plugin(type, name);
    if (!plugin) {
        return std::nullopt;
    }
    return plugin->info();
}

Result<void> PluginRegistry::load_dynamic_plugin(const std::string& path) {
#if defined(_WIN32)
    return Error(Error::Code::InvalidState, "Dynamic loading not supported on Windows build");
#else
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        return Error(Error::Code::IoError, "Failed to load plugin library");
    }

    using CreateFn = Plugin* (*)();
    using DestroyFn = void (*)(Plugin*);
    using StrFn = const char* (*)();
    auto create_fn = reinterpret_cast<CreateFn>(dlsym(handle, "create_plugin"));
    auto destroy_fn = reinterpret_cast<DestroyFn>(dlsym(handle, "destroy_plugin"));
    if (!create_fn || !destroy_fn) {
        dlclose(handle);
        return Error(Error::Code::InvalidState, "Plugin missing create/destroy entry points");
    }

    auto type_fn = reinterpret_cast<StrFn>(dlsym(handle, "plugin_type"));
    auto name_fn = reinterpret_cast<StrFn>(dlsym(handle, "plugin_name"));
    auto abi_fn = reinterpret_cast<StrFn>(dlsym(handle, "regimeflow_abi_version"));
    if (abi_fn) {
        const char* version = abi_fn();
        if (!version || std::string(version) != REGIMEFLOW_ABI_VERSION) {
            dlclose(handle);
            return Error(Error::Code::InvalidState, "Plugin ABI version mismatch");
        }
    }

    PluginPtr probe(create_fn(), [destroy_fn](Plugin* p) { destroy_fn(p); });
    if (!probe) {
        dlclose(handle);
        return Error(Error::Code::InvalidState, "Plugin creation failed");
    }

    auto info = probe->info();
    std::string plugin_name = info.name;
    if (name_fn) {
        if (const char* name = name_fn()) {
            plugin_name = name;
        }
    }
    std::string plugin_type = "dynamic";
    if (type_fn) {
        if (const char* type = type_fn()) {
            plugin_type = type;
        }
    }
    if (plugin_name.empty()) {
        probe.reset();
        dlclose(handle);
        return Error(Error::Code::InvalidArgument, "Plugin has no name");
    }

    DynamicPlugin record;
    record.handle = handle;
    record.create = [create_fn]() { return create_fn(); };
    record.destroy = [destroy_fn](Plugin* p) { destroy_fn(p); };
    record.type = plugin_type;
    record.name = plugin_name;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (dynamic_plugins_.find(plugin_name) != dynamic_plugins_.end()) {
            destroy_fn(probe.release());
            dlclose(handle);
            return Error(Error::Code::AlreadyExists, "Plugin already loaded");
        }
        dynamic_plugins_[plugin_name] = std::move(record);
        factories_[plugin_type][plugin_name] = [create_fn, destroy_fn]() {
            return PluginPtr(create_fn(), [destroy_fn](Plugin* p) { destroy_fn(p); });
        };
    }

    probe.reset();
    return Ok();
#endif
}

Result<void> PluginRegistry::unload_dynamic_plugin(const std::string& name) {
#if defined(_WIN32)
    return Error(Error::Code::InvalidState, "Dynamic loading not supported on Windows build");
#else
    DynamicPlugin plugin;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = dynamic_plugins_.find(name);
        if (it == dynamic_plugins_.end()) {
            return Error(Error::Code::NotFound, "Plugin not loaded");
        }
        plugin = it->second;
        dynamic_plugins_.erase(it);
        auto type_it = factories_.find(plugin.type);
        if (type_it != factories_.end()) {
            type_it->second.erase(name);
            if (type_it->second.empty()) {
                factories_.erase(type_it);
            }
        }
    }
    if (plugin.handle) {
        dlclose(plugin.handle);
    }
    return Ok();
#endif
}

void PluginRegistry::scan_plugin_directory(const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) {
        return;
    }
    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        auto ext = entry.path().extension().string();
#if defined(_WIN32)
        bool is_plugin = (ext == ".dll");
#elif defined(__APPLE__)
        bool is_plugin = (ext == ".dylib" || ext == ".so");
#else
        bool is_plugin = (ext == ".so");
#endif
        if (!is_plugin) {
            continue;
        }
        load_dynamic_plugin(entry.path().string());
    }
}

Result<void> PluginRegistry::start_plugin(Plugin& plugin) {
    if (auto res = plugin.on_start(); res.is_err()) {
        plugin.set_state(PluginState::Error);
        return res;
    }
    plugin.set_state(PluginState::Active);
    return Ok();
}

Result<void> PluginRegistry::stop_plugin(Plugin& plugin) {
    if (auto res = plugin.on_stop(); res.is_err()) {
        plugin.set_state(PluginState::Error);
        return res;
    }
    plugin.set_state(PluginState::Stopped);
    return Ok();
}

}  // namespace regimeflow::plugins
