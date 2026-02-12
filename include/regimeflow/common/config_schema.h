/**
 * @file config_schema.h
 * @brief RegimeFlow regimeflow config schema declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/common/result.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace regimeflow {

/**
 * @brief Single configuration property description.
 */
struct ConfigProperty {
    /**
     * @brief Type name (string, number, integer, boolean, array, object).
     */
    std::string type;
    /**
     * @brief Optional default value applied when missing.
     */
    std::optional<ConfigValue> default_value;
    /**
     * @brief True if the property must be present when no default exists.
     */
    bool required = false;
};

/**
 * @brief Schema describing expected config properties.
 */
struct ConfigSchema {
    std::unordered_map<std::string, ConfigProperty> properties;
};

/**
 * @brief Check if a ConfigValue matches a schema type.
 * @param value Value to check.
 * @param type Schema type string.
 * @return True if the value matches or if type is empty/unknown.
 */
inline bool config_value_matches(const ConfigValue& value, const std::string& type) {
    if (type == "string") return value.get_if<std::string>() != nullptr;
    if (type == "number") return value.get_if<double>() != nullptr || value.get_if<int64_t>() != nullptr;
    if (type == "integer") return value.get_if<int64_t>() != nullptr;
    if (type == "boolean") return value.get_if<bool>() != nullptr;
    if (type == "array") return value.get_if<ConfigValue::Array>() != nullptr;
    if (type == "object") return value.get_if<ConfigValue::Object>() != nullptr;
    return true;
}

/**
 * @brief Validate a config against a schema.
 * @param config Configuration to validate.
 * @param schema Schema definition.
 * @return Ok on success, ConfigError on missing/invalid fields.
 */
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

/**
 * @brief Apply schema defaults to a config.
 * @param input Input config.
 * @param schema Schema defining defaults.
 * @return New config with defaults filled.
 */
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
