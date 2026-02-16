#include "regimeflow/common/yaml_config.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdlib>

namespace regimeflow {

namespace {

std::string trim(const std::string& value) {
    auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

std::string unescape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '\\' && i + 1 < value.size()) {
            char n = value[++i];
            switch (n) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case '\\': out.push_back('\\'); break;
                case '"': out.push_back('"'); break;
                case '\'': out.push_back('\''); break;
                default: out.push_back(n); break;
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

ConfigValue parse_scalar(const std::string& raw);

ConfigValue::Array parse_inline_array(const std::string& raw) {
    std::string content = trim(raw.substr(1, raw.size() - 2));
    ConfigValue::Array array;
    std::string token;
    bool in_quotes = false;
    char quote_char = '\0';
    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if ((c == '"' || c == '\'') && (i == 0 || content[i - 1] != '\\')) {
            if (!in_quotes) {
                in_quotes = true;
                quote_char = c;
            } else if (quote_char == c) {
                in_quotes = false;
            }
            token.push_back(c);
            continue;
        }
        if (!in_quotes && c == ',') {
            if (!token.empty()) {
                array.emplace_back(parse_scalar(token));
                token.clear();
            }
            continue;
        }
        token.push_back(c);
    }
    if (!token.empty()) {
        array.emplace_back(parse_scalar(token));
    }
    return array;
}

ConfigValue parse_scalar(const std::string& raw) {
    std::string value = trim(raw);
    if (value == "true") return ConfigValue(true);
    if (value == "false") return ConfigValue(false);
    if (value.size() >= 2 && value.front() == '[' && value.back() == ']') {
        return ConfigValue(parse_inline_array(value));
    }
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        return ConfigValue(unescape(value.substr(1, value.size() - 2)));
    }
    if (!value.empty() && value.front() == '\'' && value.back() == '\'') {
        return ConfigValue(unescape(value.substr(1, value.size() - 2)));
    }

    char* end = nullptr;
    long long int_val = std::strtoll(value.c_str(), &end, 10);
    if (end && *end == '\0') {
        return ConfigValue(static_cast<int64_t>(int_val));
    }

    char* end2 = nullptr;
    double dbl_val = std::strtod(value.c_str(), &end2);
    if (end2 && *end2 == '\0') {
        return ConfigValue(dbl_val);
    }

    return ConfigValue(value);
}

int indent_of(const std::string& line) {
    int count = 0;
    for (char c : line) {
        if (c == ' ') {
            count++;
        } else {
            break;
        }
    }
    return count;
}

}  // namespace

Config YamlConfigLoader::load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open config file: " + path);
    }

    Config config;
    std::vector<std::string> key_stack;
    std::vector<int> indent_stack;
    std::string current_array_path;
    int current_array_indent = -1;
    bool current_array_item_object = false;
    std::string current_array_object_key;
    int current_array_object_indent = -1;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || starts_with(trim(line), "#")) {
            continue;
        }
        int indent = indent_of(line);
        std::string trimmed = trim(line);

        if (current_array_item_object && indent <= current_array_indent) {
            current_array_item_object = false;
            current_array_path.clear();
            current_array_indent = -1;
            current_array_object_key.clear();
            current_array_object_indent = -1;
        }

        if (starts_with(trimmed, "- ")) {
            std::string value = trim(trimmed.substr(2));
            if (!key_stack.empty()) {
                std::string path_key = key_stack.back();
                auto existing = config.get_path(path_key);
                ConfigValue::Array array;
                if (existing && existing->get_if<ConfigValue::Array>()) {
                    array = *existing->get_if<ConfigValue::Array>();
                }

                auto pos = value.find(':');
                if (pos != std::string::npos) {
                    std::string obj_key = trim(value.substr(0, pos));
                    std::string obj_val = trim(value.substr(pos + 1));
                    ConfigValue::Object obj;
                    if (!obj_val.empty()) {
                        obj[obj_key] = parse_scalar(obj_val);
                    } else {
                        obj[obj_key] = ConfigValue::Object{};
                    }
                    array.emplace_back(obj);
                    current_array_path = path_key;
                    current_array_indent = indent;
                    current_array_item_object = true;
                    current_array_object_key.clear();
                    current_array_object_indent = -1;
                } else {
                    array.emplace_back(parse_scalar(value));
                }
                config.set_path(path_key, ConfigValue(array));
            }
            continue;
        }

        auto pos = trimmed.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = trim(trimmed.substr(0, pos));
        std::string value = trim(trimmed.substr(pos + 1));

        while (!indent_stack.empty() && indent <= indent_stack.back()) {
            indent_stack.pop_back();
            if (!key_stack.empty()) {
                key_stack.pop_back();
            }
        }

        std::string full_key = key;
        if (!key_stack.empty()) {
            full_key = key_stack.back() + "." + key;
        }

        if (current_array_item_object && !current_array_path.empty() && indent > current_array_indent) {
            auto existing = config.get_path(current_array_path);
            if (existing && existing->get_if<ConfigValue::Array>()) {
                auto array = *existing->get_if<ConfigValue::Array>();
                if (!array.empty()) {
                    auto* obj = array.back().get_if<ConfigValue::Object>();
                    if (!obj) {
                        ConfigValue::Object new_obj;
                        array.back() = new_obj;
                        obj = array.back().get_if<ConfigValue::Object>();
                    }
                    if (!current_array_object_key.empty() && indent > current_array_object_indent) {
                        auto nested_it = obj->find(current_array_object_key);
                        if (nested_it == obj->end() ||
                            !nested_it->second.get_if<ConfigValue::Object>()) {
                            (*obj)[current_array_object_key] = ConfigValue::Object{};
                        }
                        auto* nested = (*obj)[current_array_object_key].get_if<ConfigValue::Object>();
                        if (value.empty()) {
                            (*nested)[key] = ConfigValue::Object{};
                            current_array_object_key = key;
                            current_array_object_indent = indent;
                        } else {
                            (*nested)[key] = parse_scalar(value);
                        }
                        config.set_path(current_array_path, ConfigValue(array));
                        continue;
                    }
                    if (value.empty()) {
                        (*obj)[key] = ConfigValue::Object{};
                        current_array_object_key = key;
                        current_array_object_indent = indent;
                        config.set_path(current_array_path, ConfigValue(array));
                        continue;
                    }
                    (*obj)[key] = parse_scalar(value);
                    config.set_path(current_array_path, ConfigValue(array));
                    continue;
                }
            }
        }

        if (value.empty()) {
            key_stack.push_back(full_key);
            indent_stack.push_back(indent);
            continue;
        }

        config.set_path(full_key, parse_scalar(value));
    }

    return config;
}

}  // namespace regimeflow
