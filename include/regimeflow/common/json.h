/**
 * @file json.h
 * @brief RegimeFlow json declarations.
 */

#pragma once

#include "regimeflow/common/result.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace regimeflow::common {

/**
 * @brief Lightweight JSON value representation.
 *
 * @details Supports null, bool, number, string, array, and object values.
 * Arrays/objects are stored via shared_ptr to avoid deep copies.
 */
class JsonValue {
public:
    /**
     * @brief Object map type.
     */
    using Object = std::unordered_map<std::string, JsonValue>;
    /**
     * @brief Array type.
     */
    using Array = std::vector<JsonValue>;
    /**
     * @brief Shared pointer to Array.
     */
    using ArrayPtr = std::shared_ptr<Array>;
    /**
     * @brief Shared pointer to Object.
     */
    using ObjectPtr = std::shared_ptr<Object>;
    /**
     * @brief Variant for supported JSON types.
     */
    using Value = std::variant<std::monostate, bool, double, std::string, ArrayPtr, ObjectPtr>;

    /**
     * @brief Construct a null JSON value.
     */
    JsonValue() = default;
    /**
     * @brief Construct a null JSON value explicitly.
     */
    explicit JsonValue(std::nullptr_t) : value_(std::monostate{}) {}
    /**
     * @brief Construct a boolean JSON value.
     * @param v Boolean value.
     */
    explicit JsonValue(bool v) : value_(v) {}
    /**
     * @brief Construct a number JSON value.
     * @param v Number value.
     */
    explicit JsonValue(double v) : value_(v) {}
    /**
     * @brief Construct a string JSON value.
     * @param v String value.
     */
    explicit JsonValue(std::string v) : value_(std::move(v)) {}
    /**
     * @brief Construct an array JSON value.
     * @param v Array value.
     */
    explicit JsonValue(Array v) : value_(std::make_shared<Array>(std::move(v))) {}
    /**
     * @brief Construct an object JSON value.
     * @param v Object value.
     */
    explicit JsonValue(Object v) : value_(std::make_shared<Object>(std::move(v))) {}

    /**
     * @brief Check if the value is null.
     */
    bool is_null() const { return std::holds_alternative<std::monostate>(value_); }
    /**
     * @brief Check if the value is boolean.
     */
    bool is_bool() const { return std::holds_alternative<bool>(value_); }
    /**
     * @brief Check if the value is numeric.
     */
    bool is_number() const { return std::holds_alternative<double>(value_); }
    /**
     * @brief Check if the value is string.
     */
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    /**
     * @brief Check if the value is array.
     */
    bool is_array() const { return std::holds_alternative<ArrayPtr>(value_); }
    /**
     * @brief Check if the value is object.
     */
    bool is_object() const { return std::holds_alternative<ObjectPtr>(value_); }

    /**
     * @brief Get the boolean value if present.
     * @return Pointer to bool, or null if type mismatch.
     */
    const bool* as_bool() const { return std::get_if<bool>(&value_); }
    /**
     * @brief Get the numeric value if present.
     * @return Pointer to number, or null if type mismatch.
     */
    const double* as_number() const { return std::get_if<double>(&value_); }
    /**
     * @brief Get the string value if present.
     * @return Pointer to string, or null if type mismatch.
     */
    const std::string* as_string() const { return std::get_if<std::string>(&value_); }
    /**
     * @brief Get the array value if present.
     * @return Pointer to array, or null if type mismatch.
     */
    const Array* as_array() const {
        if (auto ptr = std::get_if<ArrayPtr>(&value_)) {
            return ptr->get();
        }
        return nullptr;
    }
    /**
     * @brief Get the object value if present.
     * @return Pointer to object, or null if type mismatch.
     */
    const Object* as_object() const {
        if (auto ptr = std::get_if<ObjectPtr>(&value_)) {
            return ptr->get();
        }
        return nullptr;
    }

private:
    Value value_;
};

/**
 * @brief Parse JSON text into a JsonValue.
 * @param input JSON text.
 * @return Parsed JsonValue or ParseError.
 */
Result<JsonValue> parse_json(std::string_view input);

}  // namespace regimeflow::common
