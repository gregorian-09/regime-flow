/**
 * @file secret_hygiene.h
 * @brief Secret loading and redaction helpers for live tooling.
 */

#pragma once

#include "regimeflow/common/result.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace regimeflow::live
{
    /**
     * @brief Read a secret from `KEY` or `KEY_FILE`.
     *
     * File-backed values support mounted secrets from container and secret-manager runtimes.
     */
    [[nodiscard]] std::optional<std::string> read_secret_env(const char* key);

    /**
     * @brief Return true when a value uses a supported secret-manager reference scheme.
     */
    [[nodiscard]] bool is_secret_reference(std::string_view value);

    /**
     * @brief Resolve a secret-manager reference to its secret value.
     *
     * Supported schemes:
     * - `vault://`
     * - `aws-sm://`
     * - `gcp-sm://`
     * - `azure-kv://`
     */
    [[nodiscard]] Result<std::string> resolve_secret_reference(std::string_view value);

    /**
     * @brief Resolve secret-manager references in-place inside a config map.
     */
    [[nodiscard]] Result<void> resolve_secret_config(std::map<std::string, std::string>& values);

    /**
     * @brief Return true when a broker config key should be treated as sensitive.
     */
    [[nodiscard]] bool is_sensitive_secret_key(std::string_view key);

    /**
     * @brief Register a single sensitive value for later redaction.
     */
    void register_sensitive_value(std::string_view value);

    /**
     * @brief Register sensitive values from a key/value config map.
     */
    void register_sensitive_config(const std::map<std::string, std::string>& values);

    /**
     * @brief Replace registered secret values in a message with `***`.
     */
    [[nodiscard]] std::string redact_sensitive_values(std::string_view message);

    /**
     * @brief Clear registered secrets. Intended for tests.
     */
    void reset_sensitive_values_for_tests();
}  // namespace regimeflow::live
