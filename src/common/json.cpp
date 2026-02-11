#include "regimeflow/common/json.h"

#include <cctype>
#include <cstdlib>
#include <limits>

namespace regimeflow::common {

namespace {

class Parser {
public:
    explicit Parser(std::string_view input)
        : start_(input.data()), cur_(input.data()), end_(input.data() + input.size()) {}

    Result<JsonValue> parse() {
        skip_ws();
        auto value = parse_value();
        if (value.is_err()) {
            return value;
        }
        skip_ws();
        if (cur_ != end_) {
            return Error(Error::Code::ParseError, "Extra data after JSON value");
        }
        return value;
    }

private:
    const char* start_;
    const char* cur_;
    const char* end_;

    void skip_ws() {
        while (cur_ < end_ && std::isspace(static_cast<unsigned char>(*cur_))) {
            ++cur_;
        }
    }

    Result<JsonValue> parse_value() {
        if (cur_ >= end_) {
            return Error(Error::Code::ParseError, "Unexpected end of JSON");
        }
        char ch = *cur_;
        if (ch == '{') {
            return parse_object();
        }
        if (ch == '[') {
            return parse_array();
        }
        if (ch == '"') {
            return parse_string();
        }
        if (ch == 't' || ch == 'f') {
            return parse_bool();
        }
        if (ch == 'n') {
            return parse_null();
        }
        if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch))) {
            return parse_number();
        }
        return Error(Error::Code::ParseError, "Invalid JSON token");
    }

    Result<JsonValue> parse_null() {
        if (end_ - cur_ < 4 || std::string_view(cur_, 4) != "null") {
            return Error(Error::Code::ParseError, "Invalid JSON null");
        }
        cur_ += 4;
        return JsonValue(nullptr);
    }

    Result<JsonValue> parse_bool() {
        if (end_ - cur_ >= 4 && std::string_view(cur_, 4) == "true") {
            cur_ += 4;
            return JsonValue(true);
        }
        if (end_ - cur_ >= 5 && std::string_view(cur_, 5) == "false") {
            cur_ += 5;
            return JsonValue(false);
        }
        return Error(Error::Code::ParseError, "Invalid JSON boolean");
    }

    Result<JsonValue> parse_number() {
        const char* start = cur_;
        char* end = nullptr;
        double value = std::strtod(start, &end);
        if (end == start || end > end_) {
            return Error(Error::Code::ParseError, "Invalid JSON number");
        }
        cur_ = end;
        return JsonValue(value);
    }

    Result<std::string> parse_string_raw() {
        if (*cur_ != '"') {
            return Error(Error::Code::ParseError, "Expected string");
        }
        ++cur_;
        std::string out;
        while (cur_ < end_) {
            char ch = *cur_++;
            if (ch == '"') {
                return out;
            }
            if (ch == '\\') {
                if (cur_ >= end_) {
                    return Error(Error::Code::ParseError, "Invalid JSON escape");
                }
                char esc = *cur_++;
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if (end_ - cur_ < 4) {
                            return Error(Error::Code::ParseError, "Invalid JSON unicode escape");
                        }
                        unsigned int code = 0;
                        for (int i = 0; i < 4; ++i) {
                            char hex = *cur_++;
                            code <<= 4;
                            if (hex >= '0' && hex <= '9') code |= static_cast<unsigned>(hex - '0');
                            else if (hex >= 'a' && hex <= 'f') code |= static_cast<unsigned>(10 + hex - 'a');
                            else if (hex >= 'A' && hex <= 'F') code |= static_cast<unsigned>(10 + hex - 'A');
                            else {
                                return Error(Error::Code::ParseError, "Invalid JSON unicode escape");
                            }
                        }
                        if (code <= 0x7F) {
                            out.push_back(static_cast<char>(code));
                        } else {
                            out.push_back('?');
                        }
                        break;
                    }
                    default:
                        return Error(Error::Code::ParseError, "Invalid JSON escape");
                }
                continue;
            }
            if (static_cast<unsigned char>(ch) < 0x20) {
                return Error(Error::Code::ParseError, "Invalid control char in JSON string");
            }
            out.push_back(ch);
        }
        return Error(Error::Code::ParseError, "Unterminated JSON string");
    }

    Result<JsonValue> parse_string() {
        auto raw = parse_string_raw();
        if (raw.is_err()) {
            const auto& err = raw.error();
            Error copy(err.code, err.message, err.location);
            copy.details = err.details;
            return copy;
        }
        return JsonValue(std::move(raw).value());
    }

    Result<JsonValue> parse_array() {
        if (*cur_ != '[') {
            return Error(Error::Code::ParseError, "Expected array");
        }
        ++cur_;
        JsonValue::Array array;
        skip_ws();
        if (cur_ < end_ && *cur_ == ']') {
            ++cur_;
            return JsonValue(std::move(array));
        }
        while (cur_ < end_) {
            skip_ws();
            auto value = parse_value();
            if (value.is_err()) {
                return value;
            }
            array.push_back(std::move(value).value());
            skip_ws();
            if (cur_ >= end_) {
                break;
            }
            if (*cur_ == ',') {
                ++cur_;
                continue;
            }
            if (*cur_ == ']') {
                ++cur_;
                return JsonValue(std::move(array));
            }
            return Error(Error::Code::ParseError, "Expected ',' or ']' in JSON array");
        }
        return Error(Error::Code::ParseError, "Unterminated JSON array");
    }

    Result<JsonValue> parse_object() {
        if (*cur_ != '{') {
            return Error(Error::Code::ParseError, "Expected object");
        }
        ++cur_;
        JsonValue::Object object;
        skip_ws();
        if (cur_ < end_ && *cur_ == '}') {
            ++cur_;
            return JsonValue(std::move(object));
        }
        while (cur_ < end_) {
            skip_ws();
                auto key_val = parse_string_raw();
                if (key_val.is_err()) {
                    const auto& err = key_val.error();
                    Error copy(err.code, err.message, err.location);
                    copy.details = err.details;
                    return copy;
                }
            std::string key = std::move(key_val).value();
            skip_ws();
            if (cur_ >= end_ || *cur_ != ':') {
                return Error(Error::Code::ParseError, "Expected ':' after object key");
            }
            ++cur_;
            skip_ws();
                auto value = parse_value();
                if (value.is_err()) {
                    return value;
                }
            object.emplace(std::move(key), std::move(value).value());
            skip_ws();
            if (cur_ >= end_) {
                break;
            }
            if (*cur_ == ',') {
                ++cur_;
                continue;
            }
            if (*cur_ == '}') {
                ++cur_;
                return JsonValue(std::move(object));
            }
            return Error(Error::Code::ParseError, "Expected ',' or '}' in JSON object");
        }
        return Error(Error::Code::ParseError, "Unterminated JSON object");
    }
};

}  // namespace

Result<JsonValue> parse_json(std::string_view input) {
    Parser parser(input);
    return parser.parse();
}

}  // namespace regimeflow::common
