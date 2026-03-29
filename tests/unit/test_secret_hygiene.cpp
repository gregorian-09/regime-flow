#include <gtest/gtest.h>

#include "regimeflow/live/audit_log.h"
#include "regimeflow/live/secret_hygiene.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <string_view>

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace regimeflow::test
{
    namespace {
        class ScopedEnv final {
        public:
            explicit ScopedEnv(const char* key) : key_(key) {
                if (const char* value = std::getenv(key_)) {
                    original_ = std::string(value);
                }
            }

            ~ScopedEnv() {
                if (original_) {
#if defined(_WIN32)
                    _putenv_s(key_, original_->c_str());
#else
                    setenv(key_, original_->c_str(), 1);
#endif
                } else {
#if defined(_WIN32)
                    _putenv_s(key_, "");
#else
                    unsetenv(key_);
#endif
                }
            }

        private:
            const char* key_;
            std::optional<std::string> original_;
        };

        std::filesystem::path write_helper_script(std::string_view stem, std::string_view body) {
            const auto path = std::filesystem::temp_directory_path()
                / (std::string(stem)
                   + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
#if defined(_WIN32)
                   + ".bat"
#else
                   + ".sh"
#endif
                );
            {
                std::ofstream out(path);
#if defined(_WIN32)
                out << "@echo off\r\n" << body << "\r\n";
#else
                out << "#!/bin/sh\n" << body << "\n";
#endif
            }
#if !defined(_WIN32)
            chmod(path.c_str(), 0700);
#endif
            return path;
        }
    }  // namespace

    TEST(SecretHygiene, ReadsSecretFromFileBackedEnvironmentVariable) {
        ScopedEnv key_guard("REGIMEFLOW_TEST_SECRET");
        ScopedEnv key_file_guard("REGIMEFLOW_TEST_SECRET_FILE");
        live::reset_sensitive_values_for_tests();

        const auto temp_path = std::filesystem::temp_directory_path()
            / ("regimeflow_secret_test_"
               + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        {
            std::ofstream out(temp_path);
            out << "top-secret-value\n";
        }

#if defined(_WIN32)
        _putenv_s("REGIMEFLOW_TEST_SECRET", "");
        _putenv_s("REGIMEFLOW_TEST_SECRET_FILE", temp_path.string().c_str());
#else
        unsetenv("REGIMEFLOW_TEST_SECRET");
        setenv("REGIMEFLOW_TEST_SECRET_FILE", temp_path.string().c_str(), 1);
#endif

        const auto value = live::read_secret_env("REGIMEFLOW_TEST_SECRET");
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, "top-secret-value");

        std::filesystem::remove(temp_path);
    }

    TEST(SecretHygiene, RedactsRegisteredSecretsInMessages) {
        live::reset_sensitive_values_for_tests();
        live::register_sensitive_value("alpha-secret");
        live::register_sensitive_value("beta-token");

        const std::string message = "keys alpha-secret and beta-token must not leak";
        const std::string redacted = live::redact_sensitive_values(message);

        EXPECT_EQ(redacted, "keys *** and *** must not leak");
    }

    TEST(SecretHygiene, AuditLoggerRedactsRegisteredSecrets) {
        live::reset_sensitive_values_for_tests();
        live::register_sensitive_value("super-secret-key");

        const auto temp_path = std::filesystem::temp_directory_path()
            / ("regimeflow_audit_secret_test_"
               + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".log");

        live::AuditLogger logger(temp_path.string());
        auto result = logger.log_error("broker rejected super-secret-key during handshake");
        ASSERT_TRUE(result.is_ok());

        std::ifstream input(temp_path);
        ASSERT_TRUE(input.good());
        std::string content((std::istreambuf_iterator<char>(input)),
                            std::istreambuf_iterator<char>());

        EXPECT_EQ(content.find("super-secret-key"), std::string::npos);
        EXPECT_NE(content.find("***"), std::string::npos);

        std::filesystem::remove(temp_path);
    }

    TEST(SecretHygiene, AuditLoggerPreservesStructuredErrorMetadata) {
        live::reset_sensitive_values_for_tests();

        const auto temp_path = std::filesystem::temp_directory_path()
            / ("regimeflow_audit_structured_error_test_"
               + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".log");

        live::AuditLogger logger(temp_path.string());
        Error error(Error::Code::BrokerError, "Broker rejected request");
        error.details = "category=rejection;operation=submit_order;http_status=422";
        ASSERT_TRUE(logger.log_error(error, "binance.rest_post", {{"broker", "binance"}}).is_ok());

        std::ifstream input(temp_path);
        ASSERT_TRUE(input.good());
        std::string content((std::istreambuf_iterator<char>(input)),
                            std::istreambuf_iterator<char>());

        EXPECT_NE(content.find("code=broker"), std::string::npos);
        EXPECT_NE(content.find("source=binance.rest_post"), std::string::npos);
        EXPECT_NE(content.find("broker=binance"), std::string::npos);
        EXPECT_NE(content.find("details=category=rejection;operation=submit_order;http_status=422"),
                  std::string::npos);

        std::filesystem::remove(temp_path);
    }

    TEST(SecretHygiene, ResolvesVaultReferenceThroughHelperCommand) {
        ScopedEnv vault_bin_guard("REGIMEFLOW_VAULT_BIN");
        live::reset_sensitive_values_for_tests();

        const auto helper = write_helper_script("regimeflow_vault_helper_", "printf 'resolved-from-vault'");
#if defined(_WIN32)
        _putenv_s("REGIMEFLOW_VAULT_BIN", helper.string().c_str());
#else
        setenv("REGIMEFLOW_VAULT_BIN", helper.string().c_str(), 1);
#endif

        const auto result = live::resolve_secret_reference("vault://secret/trading#api_key");
        ASSERT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), "resolved-from-vault");

        std::filesystem::remove(helper);
    }

    TEST(SecretHygiene, ResolvesJsonFieldsFromCloudSecretManagers) {
        ScopedEnv aws_bin_guard("REGIMEFLOW_AWS_BIN");
        ScopedEnv gcloud_bin_guard("REGIMEFLOW_GCLOUD_BIN");
        ScopedEnv az_bin_guard("REGIMEFLOW_AZ_BIN");
        live::reset_sensitive_values_for_tests();

        const auto aws_helper = write_helper_script("regimeflow_aws_helper_",
                                                    "printf '{\"api_key\":\"aws-key\",\"secret_key\":\"aws-secret\"}'");
        const auto gcloud_helper = write_helper_script("regimeflow_gcloud_helper_",
                                                       "printf '{\"token\":\"gcp-token\"}'");
        const auto az_helper = write_helper_script("regimeflow_az_helper_",
                                                   "printf '{\"password\":\"azure-password\"}'");
#if defined(_WIN32)
        _putenv_s("REGIMEFLOW_AWS_BIN", aws_helper.string().c_str());
        _putenv_s("REGIMEFLOW_GCLOUD_BIN", gcloud_helper.string().c_str());
        _putenv_s("REGIMEFLOW_AZ_BIN", az_helper.string().c_str());
#else
        setenv("REGIMEFLOW_AWS_BIN", aws_helper.string().c_str(), 1);
        setenv("REGIMEFLOW_GCLOUD_BIN", gcloud_helper.string().c_str(), 1);
        setenv("REGIMEFLOW_AZ_BIN", az_helper.string().c_str(), 1);
#endif

        const auto aws = live::resolve_secret_reference("aws-sm://prod/regimeflow/alpaca#api_key");
        const auto gcp = live::resolve_secret_reference("gcp-sm://quant-prod/regimeflow-binance#token");
        const auto azure = live::resolve_secret_reference("azure-kv://vault-main/ib-credentials#password");

        ASSERT_TRUE(aws.is_ok());
        ASSERT_TRUE(gcp.is_ok());
        ASSERT_TRUE(azure.is_ok());
        EXPECT_EQ(aws.value(), "aws-key");
        EXPECT_EQ(gcp.value(), "gcp-token");
        EXPECT_EQ(azure.value(), "azure-password");

        std::filesystem::remove(aws_helper);
        std::filesystem::remove(gcloud_helper);
        std::filesystem::remove(az_helper);
    }

    TEST(SecretHygiene, ResolvesSecretReferencesInsideConfigMap) {
        ScopedEnv aws_bin_guard("REGIMEFLOW_AWS_BIN");
        live::reset_sensitive_values_for_tests();

        const auto aws_helper = write_helper_script("regimeflow_aws_config_helper_",
                                                    "printf '{\"api_key\":\"resolved-api-key\"}'");
#if defined(_WIN32)
        _putenv_s("REGIMEFLOW_AWS_BIN", aws_helper.string().c_str());
#else
        setenv("REGIMEFLOW_AWS_BIN", aws_helper.string().c_str(), 1);
#endif

        std::map<std::string, std::string> values{
            {"api_key", "aws-sm://prod/regimeflow/alpaca#api_key"},
            {"base_url", "https://paper-api.alpaca.markets"}
        };

        const auto result = live::resolve_secret_config(values);
        ASSERT_TRUE(result.is_ok());
        EXPECT_EQ(values["api_key"], "resolved-api-key");
        EXPECT_EQ(values["base_url"], "https://paper-api.alpaca.markets");

        std::filesystem::remove(aws_helper);
    }
}  // namespace regimeflow::test
