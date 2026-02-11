#include "regimeflow/common/config.h"

namespace regimeflow {

bool Config::has(const std::string& key) const {
    return values_.find(key) != values_.end();
}

const ConfigValue* Config::get(const std::string& key) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return nullptr;
    }
    return &it->second;
}

namespace {

std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

}  // namespace

const ConfigValue* Config::get_path(const std::string& path) const {
    auto parts = split_path(path);
    if (parts.empty()) {
        return nullptr;
    }
    const Object* object = &values_;
    const ConfigValue* current = nullptr;
    for (size_t i = 0; i < parts.size(); ++i) {
        auto it = object->find(parts[i]);
        if (it == object->end()) {
            return nullptr;
        }
        current = &it->second;
        if (i + 1 < parts.size()) {
            const auto* next = current->get_if<ConfigValue::Object>();
            if (!next) {
                return nullptr;
            }
            object = next;
        }
    }
    return current;
}

void Config::set(std::string key, ConfigValue value) {
    values_[std::move(key)] = std::move(value);
}

void Config::set_path(const std::string& path, ConfigValue value) {
    auto parts = split_path(path);
    if (parts.empty()) {
        return;
    }
    Object* object = &values_;
    for (size_t i = 0; i < parts.size(); ++i) {
        const auto& part = parts[i];
        if (i + 1 == parts.size()) {
            (*object)[part] = std::move(value);
            return;
        }
        auto it = object->find(part);
        if (it == object->end()) {
            auto [inserted, ok] = object->emplace(part, ConfigValue::Object{});
            it = inserted;
        }
        auto* next = it->second.get_if<ConfigValue::Object>();
        if (!next) {
            it->second = ConfigValue::Object{};
            next = it->second.get_if<ConfigValue::Object>();
        }
        object = next;
    }
}

}  // namespace regimeflow
