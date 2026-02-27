#include "regimeflow/engine/parity_checker.h"

#include <iostream>
#include <string>

namespace {

    void print_usage() {
        std::cout << "Usage: regimeflow_parity_check --backtest-config <path> --live-config <path> [--json]\n";
    }

    std::string json_escape(const std::string& value) {
        std::string out;
        out.reserve(value.size());
        for (char c : value) {
            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
            }
        }
        return out;
    }

    void print_json(const regimeflow::engine::ParityReport& report) {
        std::cout << "{\n";
        std::cout << "  \"status\": \"" << report.status_name() << "\",\n";
        std::cout << "  \"hard_errors\": [";
        for (size_t i = 0; i < report.hard_errors.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << "\"" << json_escape(report.hard_errors[i]) << "\"";
        }
        std::cout << "],\n";
        std::cout << "  \"warnings\": [";
        for (size_t i = 0; i < report.warnings.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << "\"" << json_escape(report.warnings[i]) << "\"";
        }
        std::cout << "],\n";
        std::cout << "  \"compat_matrix\": {";
        bool first = true;
        for (const auto& [key, value] : report.compat_matrix) {
            if (!first) std::cout << ", ";
            first = false;
            std::cout << "\"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
        }
        std::cout << "},\n";
        std::cout << "  \"backtest_values\": {";
        first = true;
        for (const auto& [key, value] : report.backtest_values) {
            if (!first) std::cout << ", ";
            first = false;
            std::cout << "\"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
        }
        std::cout << "},\n";
        std::cout << "  \"live_values\": {";
        first = true;
        for (const auto& [key, value] : report.live_values) {
            if (!first) std::cout << ", ";
            first = false;
            std::cout << "\"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
        }
        std::cout << "}\n";
        std::cout << "}\n";
    }

    void print_text(const regimeflow::engine::ParityReport& report) {
        std::cout << "Parity status: " << report.status_name() << "\n";
        if (!report.hard_errors.empty()) {
            std::cout << "Errors:\n";
            for (const auto& err : report.hard_errors) {
                std::cout << "- " << err << "\n";
            }
        }
        if (!report.warnings.empty()) {
            std::cout << "Warnings:\n";
            for (const auto& warn : report.warnings) {
                std::cout << "- " << warn << "\n";
            }
        }
        if (!report.compat_matrix.empty()) {
            std::cout << "Compatibility:\n";
            for (const auto& [key, value] : report.compat_matrix) {
                std::cout << "- " << key << ": " << value << "\n";
            }
        }
    }

}  // namespace

int main(int argc, char** argv) {
    std::string backtest_path;
    std::string live_path;
    bool json = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--backtest-config" && i + 1 < argc) {
            backtest_path = argv[++i];
        } else if (arg == "--live-config" && i + 1 < argc) {
            live_path = argv[++i];
        } else if (arg == "--json") {
            json = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
    }

    if (backtest_path.empty() || live_path.empty()) {
        print_usage();
        return 1;
    }

    try {
        auto report = regimeflow::engine::ParityChecker::check_files(backtest_path, live_path);
        if (json) {
            print_json(report);
        } else {
            print_text(report);
        }
        return report.hard_errors.empty() ? 0 : 2;
    } catch (const std::exception& exc) {
        std::cerr << "error: " << exc.what() << "\n";
        return 2;
    }
}
