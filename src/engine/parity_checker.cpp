#include "regimeflow/engine/parity_checker.h"

#include "regimeflow/common/yaml_config.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <sstream>

namespace regimeflow::engine
{
    namespace {

        std::optional<std::string> get_string(const Config& cfg, const std::string& path) {
            if (auto value = cfg.get_as<std::string>(path)) {
                return *value;
            }
            if (auto value = cfg.get_as<int64_t>(path)) {
                return std::to_string(*value);
            }
            if (auto value = cfg.get_as<double>(path)) {
                std::ostringstream out;
                out << *value;
                return out.str();
            }
            if (auto value = cfg.get_as<bool>(path)) {
                return *value ? "true" : "false";
            }
            return std::nullopt;
        }

        std::optional<std::string> first_string(const Config& cfg,
                                                const std::vector<std::string>& paths) {
            for (const auto& path : paths | std::views::all) {
                if (auto value = get_string(cfg, path)) {
                    return value;
                }
            }
            return std::nullopt;
        }

        std::optional<ConfigValue::Object> get_object(const Config& cfg,
                                                      const std::vector<std::string>& paths) {
            for (const auto& path : paths | std::views::all) {
                if (auto obj = cfg.get_as<ConfigValue::Object>(path)) {
                    return *obj;
                }
            }
            return std::nullopt;
        }

        std::unordered_map<std::string, std::string> object_to_string_map(
            const ConfigValue::Object& obj) {
            std::unordered_map<std::string, std::string> out;
            for (const auto& [key, value] : obj) {
                if (const auto* s = value.get_if<std::string>()) {
                    out[key] = *s;
                } else if (const auto* b = value.get_if<bool>()) {
                    out[key] = *b ? "true" : "false";
                } else if (const auto* i = value.get_if<int64_t>()) {
                    out[key] = std::to_string(*i);
                } else if (const auto* d = value.get_if<double>()) {
                    std::ostringstream out_val;
                    out_val << *d;
                    out[key] = out_val.str();
                }
            }
            return out;
        }

        void compare_string_field(ParityReport& report,
                                  const std::string& label,
                                  const std::optional<std::string>& backtest_value,
                                  const std::optional<std::string>& live_value,
                                  bool required_live,
                                  bool strict_match) {
            if (backtest_value) {
                report.backtest_values[label] = *backtest_value;
            }
            if (live_value) {
                report.live_values[label] = *live_value;
            }
            if (required_live && !live_value) {
                report.hard_errors.emplace_back("Missing live config: " + label);
                return;
            }
            if (backtest_value && live_value && strict_match && *backtest_value != *live_value) {
                report.hard_errors.emplace_back("Mismatch for " + label + ": backtest=" + *backtest_value +
                                                ", live=" + *live_value);
                return;
            }
            if (!backtest_value && live_value && strict_match) {
                report.warnings.emplace_back("Backtest config missing: " + label);
            }
        }

        std::string status_from_issues(const ParityReport& report, const std::string& current) {
            if (!report.hard_errors.empty()) {
                return "fail";
            }
            if (!report.warnings.empty()) {
                return "warn";
            }
            return current.empty() ? "pass" : current;
        }

    }  // namespace

    ParityReport ParityChecker::check(const Config& backtest, const Config& live) {
        ParityReport report;

        const auto live_strategy = first_string(live, {"strategy.name"});
        const auto backtest_strategy = first_string(backtest, {"strategy.name"});
        compare_string_field(report, "strategy.name", backtest_strategy, live_strategy, true, true);
        report.compat_matrix["strategy"] = status_from_issues(report, "pass");

        const auto backtest_slippage = first_string(backtest,
            {"execution.slippage.type", "slippage_model"});
        const auto live_slippage = first_string(live,
            {"execution.slippage.type", "slippage_model"});
        compare_string_field(report, "execution.slippage.type", backtest_slippage, live_slippage, false, false);

        const auto backtest_commission = first_string(backtest,
            {"execution.commission.type", "commission_model"});
        const auto live_commission = first_string(live,
            {"execution.commission.type", "commission_model"});
        compare_string_field(report, "execution.commission.type", backtest_commission, live_commission, false, false);

        if (!backtest_slippage && !live_slippage && !backtest_commission && !live_commission) {
            report.compat_matrix["execution"] = "unknown";
        } else if ((backtest_slippage && live_slippage && backtest_slippage != live_slippage) ||
                   (backtest_commission && live_commission && backtest_commission != live_commission)) {
            report.warnings.emplace_back("Execution model differs between backtest and live configs");
            report.compat_matrix["execution"] = "warn";
        } else {
            report.compat_matrix["execution"] = "pass";
        }

        const auto backtest_risk = get_object(backtest, {"risk.limits", "risk_params.limits"});
        const auto live_risk = get_object(live, {"live.risk.limits", "risk.limits"});
        if (backtest_risk) {
            auto map = object_to_string_map(*backtest_risk);
            for (const auto& [key, value] : map) {
                report.backtest_values["risk.limits." + key] = value;
            }
        }
        if (live_risk) {
            auto map = object_to_string_map(*live_risk);
            for (const auto& [key, value] : map) {
                report.live_values["risk.limits." + key] = value;
            }
        }

        if (backtest_risk && live_risk) {
            auto back_map = object_to_string_map(*backtest_risk);
            auto live_map = object_to_string_map(*live_risk);
            bool mismatch = false;
            for (const auto& [key, value] : back_map) {
                if (!live_map.contains(key)) {
                    mismatch = true;
                    report.warnings.emplace_back("Live risk limits missing key: " + key);
                    continue;
                }
                if (live_map[key] != value) {
                    mismatch = true;
                    report.warnings.emplace_back("Risk limit differs for " + key + ": backtest=" + value +
                                                 ", live=" + live_map[key]);
                }
            }
            for (const auto& [key, value] : live_map) {
                if (!back_map.contains(key)) {
                    mismatch = true;
                    report.warnings.emplace_back("Backtest risk limits missing key: " + key);
                }
            }
            report.compat_matrix["risk"] = mismatch ? "warn" : "pass";
        } else {
            report.compat_matrix["risk"] = "unknown";
            report.warnings.emplace_back("Risk limits not defined in both configs");
        }

        const auto backtest_data = first_string(backtest, {"data.type", "data_source"});
        const auto live_data = first_string(live, {"data.type", "data_source"});
        if (backtest_data) {
            report.backtest_values["data.type"] = *backtest_data;
        }
        if (live_data) {
            report.live_values["data.type"] = *live_data;
        }
        if (backtest_data && live_data && backtest_data != live_data) {
            report.warnings.emplace_back("Data source differs between backtest and live configs");
            report.compat_matrix["data"] = "warn";
        } else if (backtest_data || live_data) {
            report.compat_matrix["data"] = "pass";
        } else {
            report.compat_matrix["data"] = "unknown";
        }

        if (!report.hard_errors.empty()) {
            report.status = ParityStatus::Fail;
        } else if (!report.warnings.empty()) {
            report.status = ParityStatus::Warn;
        } else {
            report.status = ParityStatus::Pass;
        }

        return report;
    }

    ParityReport ParityChecker::check_files(const std::string& backtest_path,
                                            const std::string& live_path) {
        Config backtest = YamlConfigLoader::load_file(backtest_path);
        Config live = YamlConfigLoader::load_file(live_path);
        return check(backtest, live);
    }
}  // namespace regimeflow::engine
