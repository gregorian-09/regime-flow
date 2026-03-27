#include "regimeflow/live/secret_hygiene.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace regimeflow::live
{
    namespace {
        std::mutex g_sensitive_mutex;
        std::vector<std::string> g_sensitive_values;

        std::string trim(std::string value) {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }
            const auto last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        std::optional<std::string> read_raw_env(const char* key) {
#if defined(_WIN32)
            char* raw = nullptr;
            size_t len = 0;
            if (_dupenv_s(&raw, &len, key) != 0 || raw == nullptr) {
                return std::nullopt;
            }
            std::string value(raw);
            std::free(raw);
            return value;
#else
            if (const char* value = std::getenv(key)) {
                return std::string(value);
            }
            return std::nullopt;
#endif
        }

        std::optional<std::string> read_secret_file(const std::string& path) {
            std::ifstream input(path);
            if (!input.is_open()) {
                return std::nullopt;
            }
            std::string value((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
            value = trim(std::move(value));
            if (value.empty()) {
                return std::nullopt;
            }
            return value;
        }

        std::string normalize_key(std::string_view key) {
            std::string normalized(key);
            std::ranges::transform(normalized, normalized.begin(), [](const unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return normalized;
        }

        bool starts_with(const std::string_view value, const std::string_view prefix) {
            return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
        }

        struct SecretReference {
            std::string scheme;
            std::string path;
            std::string fragment;
            std::map<std::string, std::string> query;
        };

        std::string unescape_json(std::string_view value) {
            std::string out;
            out.reserve(value.size());
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] != '\\' || i + 1 >= value.size()) {
                    out.push_back(value[i]);
                    continue;
                }
                const char next = value[++i];
                switch (next) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: out.push_back(next); break;
                }
            }
            return out;
        }

        std::vector<std::string> split(std::string_view value, const char delimiter) {
            std::vector<std::string> parts;
            std::string current;
            for (const char c : value) {
                if (c == delimiter) {
                    parts.emplace_back(current);
                    current.clear();
                    continue;
                }
                current.push_back(c);
            }
            parts.emplace_back(current);
            return parts;
        }

        std::optional<SecretReference> parse_secret_reference(std::string_view value) {
            const auto scheme_pos = value.find("://");
            if (scheme_pos == std::string_view::npos) {
                return std::nullopt;
            }
            SecretReference ref;
            ref.scheme = std::string(value.substr(0, scheme_pos));
            std::string_view remainder = value.substr(scheme_pos + 3);

            if (const auto fragment_pos = remainder.find('#'); fragment_pos != std::string_view::npos) {
                ref.fragment = std::string(remainder.substr(fragment_pos + 1));
                remainder = remainder.substr(0, fragment_pos);
            }
            if (const auto query_pos = remainder.find('?'); query_pos != std::string_view::npos) {
                const std::string_view query = remainder.substr(query_pos + 1);
                for (const auto& item : split(query, '&')) {
                    if (item.empty()) {
                        continue;
                    }
                    const auto eq_pos = item.find('=');
                    if (eq_pos == std::string::npos) {
                        ref.query[item] = "";
                        continue;
                    }
                    ref.query[item.substr(0, eq_pos)] = item.substr(eq_pos + 1);
                }
                remainder = remainder.substr(0, query_pos);
            }
            ref.path = trim(std::string(remainder));
            if (ref.path.empty()) {
                return std::nullopt;
            }
            return ref;
        }

        std::string shell_quote(const std::string_view arg) {
#if defined(_WIN32)
            std::string quoted = "\"";
            for (const char c : arg) {
                if (c == '"' || c == '\\') {
                    quoted.push_back('\\');
                }
                quoted.push_back(c);
            }
            quoted.push_back('"');
            return quoted;
#else
            std::string quoted = "'";
            for (const char c : arg) {
                if (c == '\'') {
                    quoted += "'\"'\"'";
                } else {
                    quoted.push_back(c);
                }
            }
            quoted.push_back('\'');
            return quoted;
#endif
        }

        std::string build_shell_command(const std::vector<std::string>& args) {
            std::ostringstream command;
            bool first = true;
            for (const auto& arg : args) {
                if (!first) {
                    command << ' ';
                }
                first = false;
                command << shell_quote(arg);
            }
            return command.str();
        }

        std::optional<std::string> run_command_capture(const std::vector<std::string>& args) {
            if (args.empty()) {
                return std::nullopt;
            }
            const std::string command = build_shell_command(args);
            struct PipeCloser {
                void operator()(FILE* handle) const {
                    if (handle == nullptr) {
                        return;
                    }
#if defined(_WIN32)
                    _pclose(handle);
#else
                    pclose(handle);
#endif
                }
            };
#if defined(_WIN32)
            std::unique_ptr<FILE, PipeCloser> pipe(_popen(command.c_str(), "r"));
#else
            std::unique_ptr<FILE, PipeCloser> pipe(popen(command.c_str(), "r"));
#endif
            if (!pipe) {
                return std::nullopt;
            }
            std::string output;
            char buffer[4096];
            while (std::fgets(buffer, static_cast<int>(sizeof(buffer)), pipe.get()) != nullptr) {
                output += buffer;
            }
            output = trim(std::move(output));
            if (output.empty()) {
                return std::nullopt;
            }
            return output;
        }

        std::string command_from_env(const char* env_key, const char* fallback) {
            if (const auto value = read_raw_env(env_key); value && !value->empty()) {
                return *value;
            }
            return fallback;
        }

        std::string_view trim_view(std::string_view value) {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
                value.remove_prefix(1);
            }
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
                value.remove_suffix(1);
            }
            return value;
        }

        std::optional<std::string_view> json_extract_path(std::string_view json,
                                                          const std::vector<std::string>& path_parts,
                                                          const size_t depth) {
            json = trim_view(json);
            if (depth >= path_parts.size() || json.size() < 2 || json.front() != '{' || json.back() != '}') {
                return std::nullopt;
            }

            size_t pos = 1;
            while (pos + 1 < json.size()) {
                while (pos < json.size() && (std::isspace(static_cast<unsigned char>(json[pos])) != 0 ||
                                             json[pos] == ',')) {
                    ++pos;
                }
                if (pos >= json.size() || json[pos] == '}') {
                    break;
                }
                if (json[pos] != '"') {
                    return std::nullopt;
                }
                const size_t key_start = ++pos;
                bool escaped = false;
                while (pos < json.size()) {
                    if (!escaped && json[pos] == '"') {
                        break;
                    }
                    escaped = !escaped && json[pos] == '\\';
                    if (escaped && pos + 1 < json.size()) {
                        ++pos;
                        escaped = false;
                    }
                    ++pos;
                }
                if (pos >= json.size()) {
                    return std::nullopt;
                }
                const std::string key = unescape_json(json.substr(key_start, pos - key_start));
                ++pos;
                while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
                    ++pos;
                }
                if (pos >= json.size() || json[pos] != ':') {
                    return std::nullopt;
                }
                ++pos;
                while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
                    ++pos;
                }
                const size_t value_start = pos;
                int object_depth = 0;
                int array_depth = 0;
                bool in_string = false;
                escaped = false;
                while (pos < json.size()) {
                    const char c = json[pos];
                    if (in_string) {
                        if (!escaped && c == '"') {
                            in_string = false;
                        }
                        escaped = !escaped && c == '\\';
                        ++pos;
                        continue;
                    }
                    if (c == '"') {
                        in_string = true;
                        ++pos;
                        continue;
                    }
                    if (c == '{') {
                        ++object_depth;
                    } else if (c == '}') {
                        if (object_depth == 0 && array_depth == 0) {
                            break;
                        }
                        --object_depth;
                    } else if (c == '[') {
                        ++array_depth;
                    } else if (c == ']') {
                        --array_depth;
                    } else if (c == ',' && object_depth == 0 && array_depth == 0) {
                        break;
                    }
                    ++pos;
                }
                const std::string_view raw_value = trim_view(json.substr(value_start, pos - value_start));
                if (key == path_parts[depth]) {
                    if (depth + 1 == path_parts.size()) {
                        return raw_value;
                    }
                    return json_extract_path(raw_value, path_parts, depth + 1);
                }
            }
            return std::nullopt;
        }

        std::optional<std::string> json_extract_string(const std::string_view json,
                                                       const std::string_view key_path) {
            if (key_path.empty()) {
                return std::string(trim_view(json));
            }
            const auto parts = split(key_path, '.');
            const auto raw_value = json_extract_path(json, parts, 0);
            if (!raw_value) {
                return std::nullopt;
            }
            const std::string_view trimmed = trim_view(*raw_value);
            if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
                return unescape_json(trimmed.substr(1, trimmed.size() - 2));
            }
            return std::string(trimmed);
        }

        Result<std::string> resolve_vault_reference(const SecretReference& ref) {
            const auto slash_pos = ref.path.find('/');
            if (slash_pos == std::string::npos || slash_pos == 0 || slash_pos + 1 >= ref.path.size()) {
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "Vault secret reference must be vault://<mount>/<path>#<field>"));
            }
            const std::string mount = ref.path.substr(0, slash_pos);
            const std::string path = ref.path.substr(slash_pos + 1);
            const std::string field = ref.fragment.empty() ? "value" : ref.fragment;
            const std::string vault_bin = command_from_env("REGIMEFLOW_VAULT_BIN", "vault");
            const auto output = run_command_capture(
                {vault_bin, "kv", "get", "-mount=" + mount, "-field=" + field, path});
            if (!output) {
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "Failed to resolve Vault secret reference"));
            }
            return Result<std::string>(*output);
        }

        Result<std::string> resolve_aws_reference(const SecretReference& ref) {
            const std::string aws_bin = command_from_env("REGIMEFLOW_AWS_BIN", "aws");
            std::vector<std::string> args = {
                aws_bin,
                "secretsmanager",
                "get-secret-value",
                "--secret-id",
                ref.path,
                "--query",
                "SecretString",
                "--output",
                "text"
            };
            if (const auto it = ref.query.find("version-stage"); it != ref.query.end() && !it->second.empty()) {
                args.emplace_back("--version-stage");
                args.emplace_back(it->second);
            }
            if (const auto it = ref.query.find("version-id"); it != ref.query.end() && !it->second.empty()) {
                args.emplace_back("--version-id");
                args.emplace_back(it->second);
            }
            if (const auto output = run_command_capture(args); output) {
                if (ref.fragment.empty()) {
                    return Result<std::string>(*output);
                }
                if (const auto value = json_extract_string(*output, ref.fragment)) {
                    return Result<std::string>(*value);
                }
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "AWS secret JSON field not found"));
            }
            return Result<std::string>(Error(Error::Code::ConfigError,
                                             "Failed to resolve AWS Secrets Manager reference"));
        }

        Result<std::string> resolve_gcp_reference(const SecretReference& ref) {
            const auto slash_pos = ref.path.find('/');
            if (slash_pos == std::string::npos || slash_pos == 0 || slash_pos + 1 >= ref.path.size()) {
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "GCP secret reference must be gcp-sm://<project>/<secret>"));
            }
            const std::string project = ref.path.substr(0, slash_pos);
            const std::string secret = ref.path.substr(slash_pos + 1);
            const std::string version = [&ref] {
                if (const auto it = ref.query.find("version"); it != ref.query.end() && !it->second.empty()) {
                    return it->second;
                }
                return std::string("latest");
            }();
            const std::string gcloud_bin = command_from_env("REGIMEFLOW_GCLOUD_BIN", "gcloud");
            const auto output = run_command_capture(
                {gcloud_bin,
                 "secrets",
                 "versions",
                 "access",
                 version,
                 "--secret=" + secret,
                 "--project=" + project});
            if (!output) {
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "Failed to resolve Google Secret Manager reference"));
            }
            if (ref.fragment.empty()) {
                return Result<std::string>(*output);
            }
            if (const auto value = json_extract_string(*output, ref.fragment)) {
                return Result<std::string>(*value);
            }
            return Result<std::string>(Error(Error::Code::ConfigError,
                                             "GCP secret JSON field not found"));
        }

        Result<std::string> resolve_azure_reference(const SecretReference& ref) {
            const auto slash_pos = ref.path.find('/');
            if (slash_pos == std::string::npos || slash_pos == 0 || slash_pos + 1 >= ref.path.size()) {
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "Azure Key Vault reference must be azure-kv://<vault>/<secret>"));
            }
            const std::string vault = ref.path.substr(0, slash_pos);
            const std::string secret = ref.path.substr(slash_pos + 1);
            std::vector<std::string> args = {
                command_from_env("REGIMEFLOW_AZ_BIN", "az"),
                "keyvault",
                "secret",
                "show",
                "--vault-name",
                vault,
                "--name",
                secret,
                "--query",
                "value",
                "-o",
                "tsv"
            };
            if (const auto it = ref.query.find("version"); it != ref.query.end() && !it->second.empty()) {
                args.emplace_back("--version");
                args.emplace_back(it->second);
            }
            if (const auto output = run_command_capture(args); output) {
                if (ref.fragment.empty()) {
                    return Result<std::string>(*output);
                }
                if (const auto value = json_extract_string(*output, ref.fragment)) {
                    return Result<std::string>(*value);
                }
                return Result<std::string>(Error(Error::Code::ConfigError,
                                                 "Azure Key Vault secret JSON field not found"));
            }
            return Result<std::string>(Error(Error::Code::ConfigError,
                                             "Failed to resolve Azure Key Vault reference"));
        }
    }  // namespace

    std::optional<std::string> read_secret_env(const char* key) {
        if (auto value = read_raw_env(key); value && !value->empty()) {
            return value;
        }
        const std::string file_key = std::string(key) + "_FILE";
        if (auto path = read_raw_env(file_key.c_str()); path && !path->empty()) {
            return read_secret_file(*path);
        }
        return std::nullopt;
    }

    bool is_secret_reference(const std::string_view value) {
        static constexpr std::string_view prefixes[] = {
            "vault://",
            "aws-sm://",
            "gcp-sm://",
            "azure-kv://"
        };
        return std::ranges::any_of(prefixes, [value](const std::string_view prefix) {
            return starts_with(value, prefix);
        });
    }

    Result<std::string> resolve_secret_reference(const std::string_view value) {
        const auto ref = parse_secret_reference(value);
        if (!ref) {
            return Result<std::string>(Error(Error::Code::ConfigError, "Invalid secret reference"));
        }
        if (ref->scheme == "vault") {
            return resolve_vault_reference(*ref);
        }
        if (ref->scheme == "aws-sm") {
            return resolve_aws_reference(*ref);
        }
        if (ref->scheme == "gcp-sm") {
            return resolve_gcp_reference(*ref);
        }
        if (ref->scheme == "azure-kv") {
            return resolve_azure_reference(*ref);
        }
        return Result<std::string>(Error(Error::Code::ConfigError,
                                         "Unsupported secret reference scheme: " + ref->scheme));
    }

    Result<void> resolve_secret_config(std::map<std::string, std::string>& values) {
        for (auto& [key, value] : values) {
            if (!is_secret_reference(value)) {
                continue;
            }
            const auto resolved = resolve_secret_reference(value);
            if (resolved.is_err()) {
                return Result<void>(Error(Error::Code::ConfigError,
                                          "Failed to resolve secret for " + key + ": "
                                              + resolved.error().message));
            }
            value = resolved.value();
        }
        return Ok();
    }

    bool is_sensitive_secret_key(const std::string_view key) {
        const std::string normalized = normalize_key(key);
        constexpr std::string_view sensitive_parts[] = {
            "api_key",
            "secret",
            "token",
            "password",
            "passphrase",
            "authorization"
        };
        return std::ranges::any_of(sensitive_parts, [&normalized](const std::string_view part) {
            return normalized.find(part) != std::string::npos;
        });
    }

    void register_sensitive_value(const std::string_view value) {
        if (value.empty()) {
            return;
        }
        std::lock_guard<std::mutex> lock(g_sensitive_mutex);
        const std::string secret(value);
        if (std::ranges::find(g_sensitive_values, secret) == g_sensitive_values.end()) {
            g_sensitive_values.emplace_back(secret);
            std::ranges::sort(g_sensitive_values, std::greater<>{},
                              [](const std::string& item) { return item.size(); });
        }
    }

    void register_sensitive_config(const std::map<std::string, std::string>& values) {
        for (const auto& [key, value] : values) {
            if (is_sensitive_secret_key(key)) {
                register_sensitive_value(value);
            }
        }
    }

    std::string redact_sensitive_values(const std::string_view message) {
        std::string sanitized(message);
        std::lock_guard<std::mutex> lock(g_sensitive_mutex);
        for (const auto& secret : g_sensitive_values) {
            if (secret.empty()) {
                continue;
            }
            std::string::size_type pos = 0;
            while ((pos = sanitized.find(secret, pos)) != std::string::npos) {
                sanitized.replace(pos, secret.size(), "***");
                pos += 3;
            }
        }
        return sanitized;
    }

    void reset_sensitive_values_for_tests() {
        std::lock_guard<std::mutex> lock(g_sensitive_mutex);
        g_sensitive_values.clear();
    }
}  // namespace regimeflow::live
