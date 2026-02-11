#include "regimeflow/data/tick_csv_reader.h"

#include "regimeflow/common/time.h"
#include "regimeflow/data/merged_iterator.h"
#include "regimeflow/data/memory_data_source.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace regimeflow::data {

namespace {

std::vector<std::string> split_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string token;
    std::istringstream stream(line);
    while (std::getline(stream, token, delimiter)) {
        fields.push_back(token);
    }
    return fields;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string normalize_header(std::string value) {
    value = trim(std::move(value));
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::map<std::string, std::string> default_aliases() {
    return {
        {"datetime", "timestamp"},
        {"date", "timestamp"},
        {"time", "timestamp"},
        {"ts", "timestamp"},
        {"qty", "quantity"},
        {"size", "quantity"},
        {"volume", "quantity"}
    };
}

std::string apply_alias(std::string name, const std::map<std::string, std::string>& aliases) {
    auto it = aliases.find(name);
    if (it != aliases.end()) {
        return it->second;
    }
    return name;
}

struct RunningStats {
    size_t count = 0;
    double mean = 0.0;
    double m2 = 0.0;

    void push(double value) {
        ++count;
        double delta = value - mean;
        mean += delta / static_cast<double>(count);
        double delta2 = value - mean;
        m2 += delta * delta2;
    }

    double variance() const {
        return count > 1 ? m2 / static_cast<double>(count - 1) : 0.0;
    }

    double stddev() const {
        return std::sqrt(variance());
    }
};

bool within_trading_hours(const Timestamp& ts, int start_seconds, int end_seconds) {
    std::time_t seconds = static_cast<std::time_t>(ts.microseconds() / 1'000'000);
    std::tm tm = {};
#if defined(_WIN32)
    gmtime_s(&tm, &seconds);
#else
    gmtime_r(&seconds, &tm);
#endif
    int secs = tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
    if (start_seconds <= end_seconds) {
        return secs >= start_seconds && secs <= end_seconds;
    }
    return secs >= start_seconds || secs <= end_seconds;
}

void ensure_required_columns(const std::map<std::string, int>& mapping) {
    const char* required[] = {"timestamp", "price", "quantity"};
    for (const auto* name : required) {
        if (!mapping.count(name)) {
            throw std::runtime_error(std::string("CSV config error: missing column mapping for ") +
                                     name);
        }
    }
}

Timestamp parse_timestamp(const std::string& value,
                          const std::string& datetime_format,
                          size_t line_number) {
    try {
        return Timestamp::from_string(value, datetime_format);
    } catch (const std::exception&) {
        throw std::runtime_error("CSV parse error: invalid timestamp at line " +
                                 std::to_string(line_number));
    }
}

}  // namespace

CSVTickDataSource::CSVTickDataSource(const Config& config) : config_(config) {
    scan_directory();
}

std::vector<SymbolInfo> CSVTickDataSource::get_available_symbols() const {
    std::vector<SymbolInfo> info;
    for (const auto& [id, path] : symbol_to_path_) {
        SymbolInfo symbol;
        symbol.id = id;
        symbol.ticker = SymbolRegistry::instance().lookup(id);
        info.push_back(symbol);
    }
    return info;
}

TimeRange CSVTickDataSource::get_available_range(SymbolId symbol) const {
    TimeRange range;
    const std::string path = resolve_path(symbol);
    if (path.empty()) {
        return range;
    }
    auto ticks = parse_ticks(symbol, path, range);
    if (!ticks.empty()) {
        range.start = ticks.front().timestamp;
        range.end = ticks.back().timestamp;
    }
    return range;
}

std::vector<Bar> CSVTickDataSource::get_bars(SymbolId, TimeRange, BarType) {
    return {};
}

std::vector<Tick> CSVTickDataSource::get_ticks(SymbolId symbol, TimeRange range) {
    const std::string path = resolve_path(symbol);
    if (path.empty()) {
        return {};
    }
    return parse_ticks(symbol, path, range);
}

std::unique_ptr<DataIterator> CSVTickDataSource::create_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range, BarType) {
    std::vector<Bar> bars;
    for (auto symbol : symbols) {
        auto ticks = get_ticks(symbol, range);
        bars.reserve(bars.size() + ticks.size());
        for (const auto& tick : ticks) {
            Bar bar;
            bar.symbol = tick.symbol;
            bar.timestamp = tick.timestamp;
            bar.open = tick.price;
            bar.high = tick.price;
            bar.low = tick.price;
            bar.close = tick.price;
            bar.volume = static_cast<Volume>(tick.quantity);
            bars.push_back(bar);
        }
    }
    return std::make_unique<VectorBarIterator>(std::move(bars));
}

std::unique_ptr<TickIterator> CSVTickDataSource::create_tick_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range) {
    std::vector<std::unique_ptr<TickIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto ticks = get_ticks(symbol, range);
        iterators.push_back(std::make_unique<VectorTickIterator>(std::move(ticks)));
    }
    return std::make_unique<MergedTickIterator>(std::move(iterators));
}

std::vector<CorporateAction> CSVTickDataSource::get_corporate_actions(SymbolId, TimeRange) {
    return {};
}

std::string CSVTickDataSource::resolve_path(SymbolId symbol) const {
    auto it = symbol_to_path_.find(symbol);
    if (it != symbol_to_path_.end()) {
        return it->second;
    }
    auto symbol_str = SymbolRegistry::instance().lookup(symbol);
    std::string filename = config_.file_pattern;
    auto pos = filename.find("{symbol}");
    if (pos != std::string::npos) {
        filename.replace(pos, 8, symbol_str);
    }
    std::filesystem::path path = std::filesystem::path(config_.data_directory) / filename;
    if (!std::filesystem::exists(path)) {
        return {};
    }
    return path.string();
}

void CSVTickDataSource::scan_directory() {
    symbol_to_path_.clear();
    if (config_.data_directory.empty()) {
        return;
    }
    std::filesystem::path root(config_.data_directory);
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        return;
    }

    const std::string pattern = config_.file_pattern;
    auto marker = pattern.find("{symbol}");
    std::string prefix = marker == std::string::npos ? "" : pattern.substr(0, marker);
    std::string suffix = marker == std::string::npos ? "" : pattern.substr(marker + 8);

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string filename = entry.path().filename().string();
        if (!prefix.empty() && filename.rfind(prefix, 0) != 0) {
            continue;
        }
        if (!suffix.empty() && (filename.size() < suffix.size() ||
                                filename.substr(filename.size() - suffix.size()) != suffix)) {
            continue;
        }
        std::string symbol = filename.substr(prefix.size(),
                                             filename.size() - prefix.size() - suffix.size());
        if (symbol.empty()) {
            continue;
        }
        SymbolId id = SymbolRegistry::instance().intern(symbol);
        symbol_to_path_[id] = entry.path().string();
    }
}

std::vector<Tick> CSVTickDataSource::parse_ticks(SymbolId symbol, const std::string& path,
                                                 TimeRange range) const {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    last_report_ = ValidationReport();
    std::string line;
    std::map<std::string, int> col = config_.column_mapping;
    auto aliases = default_aliases();
    if (config_.has_header) {
        if (!std::getline(file, line)) {
            return {};
        }
        if (col.empty()) {
            auto fields = split_line(line, config_.delimiter);
            for (size_t i = 0; i < fields.size(); ++i) {
                auto key = normalize_header(fields[i]);
                auto canonical = apply_alias(key, aliases);
                col[canonical] = static_cast<int>(i);
            }
        }
    }

    ensure_required_columns(col);
    int ts_col = col.count("timestamp") ? col["timestamp"] : 0;
    int price_col = col.count("price") ? col["price"] : 1;
    int qty_col = col.count("quantity") ? col["quantity"] : 2;

    std::vector<Tick> ticks;
    Timestamp last_ts;
    bool has_last_ts = false;
    double last_price = 0.0;
    bool has_last_price = false;
    size_t line_number = config_.has_header ? 2 : 1;
    Timestamp now;
    if (config_.validation.check_future_timestamps) {
        now = Timestamp::now();
    }
    RunningStats price_stats;
    RunningStats volume_stats;

    auto handle_issue = [&](ValidationSeverity severity,
                            ValidationAction action,
                            const std::string& message) -> bool {
        if (config_.collect_validation_report) {
            last_report_.add_issue({severity, line_number, message});
        }
        if (severity == ValidationSeverity::Error) {
            if (action == ValidationAction::Fail) {
                if (!config_.collect_validation_report) {
                    throw std::runtime_error("CSV validation error: " + message + " at line " +
                                             std::to_string(line_number));
                }
                return false;
            }
            return false;
        }
        if (action == ValidationAction::Fail) {
            if (!config_.collect_validation_report) {
                throw std::runtime_error("CSV validation warning treated as error: " + message +
                                         " at line " + std::to_string(line_number));
            }
            return false;
        }
        if (action == ValidationAction::Skip) {
            return false;
        }
        return true;
    };
    auto error_action = [&]() {
        auto action = config_.validation.on_error;
        if (action == ValidationAction::Continue || action == ValidationAction::Fill) {
            action = ValidationAction::Skip;
        }
        return action;
    };

    while (std::getline(file, line)) {
        auto fields = split_line(line, config_.delimiter);
        if (static_cast<int>(fields.size()) <= std::max({ts_col, price_col, qty_col})) {
            if (!handle_issue(ValidationSeverity::Error, error_action(), "Missing columns")) {
                ++line_number;
                continue;
            }
        }

        Tick tick;
        try {
            tick.timestamp = parse_timestamp(fields[ts_col], config_.datetime_format, line_number);
            if (config_.utc_offset_seconds != 0) {
                tick.timestamp = tick.timestamp - Duration::seconds(config_.utc_offset_seconds);
            }
            tick.price = std::stod(fields[price_col]);
            tick.quantity = std::stod(fields[qty_col]);
        } catch (const std::exception& ex) {
            if (!handle_issue(ValidationSeverity::Error, error_action(), ex.what())) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.check_price_bounds) {
            if (!std::isfinite(tick.price) || tick.price <= 0.0 ||
                (config_.validation.max_price > 0.0 &&
                 tick.price > config_.validation.max_price)) {
                if (!handle_issue(ValidationSeverity::Error, error_action(), "Invalid price")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_volume_bounds && config_.validation.max_volume > 0 &&
            tick.quantity > static_cast<double>(config_.validation.max_volume)) {
            if (!handle_issue(ValidationSeverity::Error, error_action(),
                              "Quantity exceeds max_volume")) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.check_future_timestamps) {
            if (tick.timestamp > now + config_.validation.max_future_skew) {
                if (!handle_issue(ValidationSeverity::Error, error_action(),
                                  "Timestamp is in the future")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_trading_hours &&
            !within_trading_hours(tick.timestamp,
                                  config_.validation.trading_start_seconds,
                                  config_.validation.trading_end_seconds)) {
            if (!handle_issue(ValidationSeverity::Error, error_action(),
                              "Timestamp outside trading hours")) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.require_monotonic_timestamps && has_last_ts &&
            tick.timestamp < last_ts) {
            if (!handle_issue(ValidationSeverity::Error, error_action(),
                              "Non-monotonic timestamp")) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.check_gap && has_last_ts) {
            Duration gap = tick.timestamp - last_ts;
            if (gap.total_microseconds() > config_.validation.max_gap.total_microseconds()) {
                if (!handle_issue(ValidationSeverity::Warning, config_.validation.on_gap,
                                  "Timestamp gap exceeds max_gap")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_price_jump && has_last_price && last_price != 0.0) {
            double jump = std::abs(tick.price - last_price) / std::abs(last_price);
            if (jump > config_.validation.max_jump_pct) {
                if (!handle_issue(ValidationSeverity::Warning, config_.validation.on_warning,
                                  "Price jump exceeds max_jump_pct")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_outliers &&
            price_stats.count >= config_.validation.outlier_warmup) {
            double stddev = price_stats.stddev();
            if (stddev > 0.0) {
                double zscore = std::abs(tick.price - price_stats.mean) / stddev;
                if (zscore > config_.validation.outlier_zscore) {
                    handle_issue(ValidationSeverity::Warning, config_.validation.on_warning,
                                 "Price outlier detected");
                }
            }
        }

        if (config_.validation.check_outliers &&
            volume_stats.count >= config_.validation.outlier_warmup) {
            double stddev = volume_stats.stddev();
            if (stddev > 0.0) {
                double zscore = std::abs(tick.quantity - volume_stats.mean) / stddev;
                if (zscore > config_.validation.outlier_zscore) {
                    handle_issue(ValidationSeverity::Warning, config_.validation.on_warning,
                                 "Quantity outlier detected");
                }
            }
        }

        tick.symbol = symbol;
        last_ts = tick.timestamp;
        has_last_ts = true;
        last_price = tick.price;
        has_last_price = true;
        if (config_.validation.check_outliers) {
            price_stats.push(tick.price);
            volume_stats.push(tick.quantity);
        }

        if (range.start.microseconds() != 0 || range.end.microseconds() != 0) {
            if (!range.contains(tick.timestamp)) {
                ++line_number;
                continue;
            }
        }

        ticks.push_back(tick);
        ++line_number;
    }

    return ticks;
}

}  // namespace regimeflow::data
