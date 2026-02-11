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

class JsonValue {
public:
    using Object = std::unordered_map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;
    using ArrayPtr = std::shared_ptr<Array>;
    using ObjectPtr = std::shared_ptr<Object>;
    using Value = std::variant<std::monostate, bool, double, std::string, ArrayPtr, ObjectPtr>;

    JsonValue() = default;
    explicit JsonValue(std::nullptr_t) : value_(std::monostate{}) {}
    explicit JsonValue(bool v) : value_(v) {}
    explicit JsonValue(double v) : value_(v) {}
    explicit JsonValue(std::string v) : value_(std::move(v)) {}
    explicit JsonValue(Array v) : value_(std::make_shared<Array>(std::move(v))) {}
    explicit JsonValue(Object v) : value_(std::make_shared<Object>(std::move(v))) {}

    bool is_null() const { return std::holds_alternative<std::monostate>(value_); }
    bool is_bool() const { return std::holds_alternative<bool>(value_); }
    bool is_number() const { return std::holds_alternative<double>(value_); }
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    bool is_array() const { return std::holds_alternative<ArrayPtr>(value_); }
    bool is_object() const { return std::holds_alternative<ObjectPtr>(value_); }

    const bool* as_bool() const { return std::get_if<bool>(&value_); }
    const double* as_number() const { return std::get_if<double>(&value_); }
    const std::string* as_string() const { return std::get_if<std::string>(&value_); }
    const Array* as_array() const {
        if (auto ptr = std::get_if<ArrayPtr>(&value_)) {
            return ptr->get();
        }
        return nullptr;
    }
    const Object* as_object() const {
        if (auto ptr = std::get_if<ObjectPtr>(&value_)) {
            return ptr->get();
        }
        return nullptr;
    }

private:
    Value value_;
};

Result<JsonValue> parse_json(std::string_view input);

}  // namespace regimeflow::common
