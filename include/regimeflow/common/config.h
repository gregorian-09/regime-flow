/**
 * @file config.h
 * @brief RegimeFlow regimeflow config declarations.
 */

#pragma once

#include <optional>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace regimeflow
{
    /**
     * @brief Variant value used by the config system.
     *
     * @details Supports scalar types plus arrays/objects for hierarchical configs.
     */
    class ConfigValue {
    public:
        /**
         * @brief Array of config values.
         */
        using Array = std::vector<ConfigValue>;
        /**
         * @brief Object (map) of config values.
         */
        using Object = std::unordered_map<std::string, ConfigValue>;
        using ArrayPtr = std::shared_ptr<Array>;
        using ObjectPtr = std::shared_ptr<Object>;
        /**
         * @brief Variant for supported config types.
         */
        using Value = std::variant<std::monostate, bool, int64_t, double, std::string, ArrayPtr, ObjectPtr>;

        /**
         * @brief Construct an empty (monostate) value.
         */
        ConfigValue() = default;
        ConfigValue(const ConfigValue& other) { copy_from(other); }
        ConfigValue& operator=(const ConfigValue& other) {
            if (this != &other) {
                copy_from(other);
            }
            return *this;
        }
        ConfigValue(ConfigValue&&) noexcept = default;
        ConfigValue& operator=(ConfigValue&&) noexcept = default;
        /**
         * @brief Construct a boolean config value.
         * @param v Boolean value.
         */
        ConfigValue(bool v) : value_(v) {}
        /**
         * @brief Construct an integer config value.
         * @param v Integer value.
         */
        ConfigValue(int64_t v) : value_(v) {}
        /**
         * @brief Construct a double config value.
         * @param v Double value.
         */
        ConfigValue(double v) : value_(v) {}
        /**
         * @brief Construct a string config value.
         * @param v String value.
         */
        ConfigValue(std::string v) : value_(std::move(v)) {}
        /**
         * @brief Construct a string config value from C-string.
         * @param v C-string value.
         */
        ConfigValue(const char* v) : value_(std::string(v)) {}
        /**
         * @brief Construct an array config value.
         * @param v Array value.
         */
        ConfigValue(Array v) : value_(std::make_shared<Array>(std::move(v))) {}
        /**
         * @brief Construct an object config value.
         * @param v Object value.
         */
        ConfigValue(Object v) : value_(std::make_shared<Object>(std::move(v))) {}

        /**
         * @brief Access the raw variant.
         * @return Reference to the underlying Value variant.
         */
        const Value& raw() const { return value_; }

        /**
         * @brief Try to get a typed pointer to the stored value.
         * @tparam T Requested type.
         * @return Pointer to value if the type matches, otherwise null.
         */
        template<typename T>
        const T* get_if() const {
            if constexpr (std::is_same_v<T, Array>) {
                if (const auto* ptr = std::get_if<ArrayPtr>(&value_)) {
                    return ptr->get();
                }
                return nullptr;
            } else if constexpr (std::is_same_v<T, Object>) {
                if (const auto* ptr = std::get_if<ObjectPtr>(&value_)) {
                    return ptr->get();
                }
                return nullptr;
            } else {
                return std::get_if<T>(&value_);
            }
        }
        /**
         * @brief Try to get a mutable typed pointer to the stored value.
         * @tparam T Requested type.
         * @return Pointer to value if the type matches, otherwise null.
         */
        template<typename T>
        T* get_if() {
            if constexpr (std::is_same_v<T, Array>) {
                if (auto* ptr = std::get_if<ArrayPtr>(&value_)) {
                    return ptr->get();
                }
                return nullptr;
            } else if constexpr (std::is_same_v<T, Object>) {
                if (auto* ptr = std::get_if<ObjectPtr>(&value_)) {
                    return ptr->get();
                }
                return nullptr;
            } else {
                return std::get_if<T>(&value_);
            }
        }

    private:
        void copy_from(const ConfigValue& other) {
            if (const auto* array = other.get_if<Array>()) {
                value_ = std::make_shared<Array>(*array);
                return;
            }
            if (const auto* object = other.get_if<Object>()) {
                value_ = std::make_shared<Object>(*object);
                return;
            }
            value_ = other.value_;
        }

        Value value_;
    };

    /**
     * @brief Hierarchical configuration container.
     */
    class Config {
    public:
        /**
         * @brief Object alias for root configuration map.
         */
        using Object = ConfigValue::Object;

        /**
         * @brief Construct an empty configuration.
         */
        Config() = default;
        /**
         * @brief Construct a configuration from a map of values.
         * @param values Root object.
         */
        explicit Config(Object values) : values_(std::move(values)) {}

        /**
         * @brief Check if a top-level key exists.
         * @param key Key to check.
         * @return True if present.
         */
        bool has(const std::string& key) const;
        /**
         * @brief Get a top-level value by key.
         * @param key Key to retrieve.
         * @return Pointer to value or null if absent.
         */
        const ConfigValue* get(const std::string& key) const;
        /**
         * @brief Get a nested value by dotted path.
         * @param path Dotted path (e.g., "risk.limits.max_drawdown").
         * @return Pointer to value or null if absent.
         */
        const ConfigValue* get_path(const std::string& path) const;

        /**
         * @brief Retrieve a typed value by key or dotted path.
         * @tparam T Requested type.
         * @param key Key or dotted path.
         * @return Optional typed value, empty if missing or type mismatch.
         */
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

        /**
         * @brief Set a top-level key.
         * @param key Key to set.
         * @param value Value to assign.
         */
        void set(std::string key, ConfigValue value);
        /**
         * @brief Set a nested value by dotted path, creating objects as needed.
         * @param path Dotted path.
         * @param value Value to assign.
         */
        void set_path(const std::string& path, ConfigValue value);

    private:
        Object values_;
    };
}  // namespace regimeflow
