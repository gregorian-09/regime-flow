#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/common/result.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace regimeflow {

struct ConfigProperty {
    std::string type;
    std::optional<ConfigValue> default_value;
    bool required = false;
};

struct ConfigSchema {
    std::unordered_map<std::string, ConfigProperty> properties;
};

inline bool config_value_matches(const ConfigValue& value, const std::string& type) {
    if (type == "string") return value.get_if<std::string>() != nullptr;
    if (type == "number") return value.get_if<double>() != nullptr || value.get_if<int64_t>() != nullptr;
    if (type == "integer") return value.get_if<int64_t>() != nullptr;
    if (type == "boolean") return value.get_if<bool>() != nullptr;
    if (type == "array") return value.get_if<ConfigValue::Array>() != nullptr;
    if (type == "object") return value.get_if<ConfigValue::Object>() != nullptr;
    return true;
}

inline Result<void> validate_config(const Config& config, const ConfigSchema& schema) {
    for (const auto& [key, prop] : schema.properties) {
        const auto* value = config.get_path(key);
        if (!value) {
            if (prop.required && !prop.default_value.has_value()) {
                return Error(Error::Code::ConfigError, "Missing required config field: " + key);
            }
            continue;
        }
        if (!prop.type.empty() && !config_value_matches(*value, prop.type)) {
            return Error(Error::Code::ConfigError, "Config field type mismatch: " + key);
        }
    }
    return Ok();
}

inline Config apply_defaults(const Config& input, const ConfigSchema& schema) {
    Config output = input;
    for (const auto& [key, prop] : schema.properties) {
        if (!prop.default_value.has_value()) {
            continue;
        }
        if (!output.get_path(key)) {
            output.set_path(key, *prop.default_value);
        }
    }
    return output;
}

}  // namespace regimeflow
