#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace regimeflow {

class ConfigValue {
public:
    using Array = std::vector<ConfigValue>;
    using Object = std::unordered_map<std::string, ConfigValue>;
    using Value = std::variant<std::monostate, bool, int64_t, double, std::string, Array, Object>;

    ConfigValue() = default;
    ConfigValue(bool v) : value_(v) {}
    ConfigValue(int64_t v) : value_(v) {}
    ConfigValue(double v) : value_(v) {}
    ConfigValue(std::string v) : value_(std::move(v)) {}
    ConfigValue(const char* v) : value_(std::string(v)) {}
    ConfigValue(Array v) : value_(std::move(v)) {}
    ConfigValue(Object v) : value_(std::move(v)) {}

    const Value& raw() const { return value_; }

    template<typename T>
    const T* get_if() const { return std::get_if<T>(&value_); }
    template<typename T>
    T* get_if() { return std::get_if<T>(&value_); }

private:
    Value value_;
};

class Config {
public:
    using Object = ConfigValue::Object;

    Config() = default;
    explicit Config(Object values) : values_(std::move(values)) {}

    bool has(const std::string& key) const;
    const ConfigValue* get(const std::string& key) const;
    const ConfigValue* get_path(const std::string& path) const;

    template<typename T>
    std::optional<T> get_as(const std::string& key) const {
        const auto* value = get(key);
        if (!value) {
            value = get_path(key);
        }
        if (!value) {
            return std::nullopt;
        }
        if (const auto* typed = value->get_if<T>()) {
            return *typed;
        }
        return std::nullopt;
    }

    void set(std::string key, ConfigValue value);
    void set_path(const std::string& path, ConfigValue value);

private:
    Object values_;
};

}  // namespace regimeflow
