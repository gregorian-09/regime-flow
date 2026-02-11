#include "regimeflow/data/csv_reader.h"

#include "regimeflow/common/time.h"
#include "regimeflow/data/merged_iterator.h"
#include "regimeflow/data/validation_utils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace regimeflow::data {

namespace {

std::vector<std::string> split_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string token;
    for (char ch : line) {
        if (ch == delimiter) {
            fields.push_back(token);
            token.clear();
        } else {
            token.push_back(ch);
        }
    }
    fields.push_back(token);
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
        {"open_price", "open"},
        {"high_price", "high"},
        {"low_price", "low"},
        {"close_price", "close"},
        {"adj_close", "close"},
        {"adjclose", "close"},
        {"volume_traded", "volume"},
        {"vol", "volume"}
    };
}

std::map<std::string, std::string> action_aliases() {
    return {
        {"type", "action_type"},
        {"action", "action_type"},
        {"event_type", "action_type"},
        {"ex_date", "effective_date"},
        {"effective", "effective_date"},
        {"split_factor", "factor"},
        {"ratio", "factor"},
        {"dividend", "amount"},
        {"cash_dividend", "amount"},
        {"symbol", "new_symbol"}
    };
}

std::string apply_alias(std::string name, const std::map<std::string, std::string>& aliases) {
    auto it = aliases.find(name);
    if (it != aliases.end()) {
        return it->second;
    }
    return name;
}

Timestamp parse_timestamp(const std::string& value,
                          const std::string& datetime_format,
                          const std::string& date_format,
                          size_t line_number) {
    try {
        return Timestamp::from_string(value, datetime_format);
    } catch (const std::exception&) {
        try {
            return Timestamp::from_string(value, date_format);
        } catch (const std::exception&) {
            throw std::runtime_error("CSV parse error: invalid timestamp at line " +
                                     std::to_string(line_number));
        }
    }
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
    const char* required[] = {"timestamp", "open", "high", "low", "close", "volume"};
    for (const auto* name : required) {
        if (!mapping.count(name)) {
            throw std::runtime_error(std::string("CSV config error: missing column mapping for ") +
                                     name);
        }
    }
}

[[maybe_unused]] std::string resolve_action_path(const CSVDataSource::Config& config,
                                                [[maybe_unused]] SymbolId symbol) {
    std::string directory = config.actions_directory.empty()
        ? config.data_directory
        : config.actions_directory;
    if (directory.empty()) {
        return {};
    }
    auto symbol_str = SymbolRegistry::instance().lookup(symbol);
    std::string filename = config.actions_file_pattern;
    auto pos = filename.find("{symbol}");
    if (pos != std::string::npos) {
        filename.replace(pos, 8, symbol_str);
    }
    std::filesystem::path path = std::filesystem::path(directory) / filename;
    std::ifstream probe(path);
    if (probe) {
        return path.string();
    }
    std::filesystem::path fallback = std::filesystem::path(directory)
        / (symbol_str + "_actions.csv");
    std::ifstream fallback_probe(fallback);
    if (fallback_probe) {
        return fallback.string();
    }
    std::filesystem::path dir_path(directory);
    if (std::filesystem::exists(dir_path) && std::filesystem::is_directory(dir_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (entry.path().filename() == fallback.filename()) {
                return entry.path().string();
            }
        }
    }
    return {};
}

CorporateActionType parse_action_type(const std::string& raw) {
    std::string value = trim(raw);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "split") {
        return CorporateActionType::Split;
    }
    if (value == "dividend") {
        return CorporateActionType::Dividend;
    }
    if (value == "symbol_change" || value == "symbolchange" || value == "rename") {
        return CorporateActionType::SymbolChange;
    }
    try {
        int code = std::stoi(value);
        return static_cast<CorporateActionType>(code);
    } catch (...) {
        return CorporateActionType::Split;
    }
}

std::map<std::string, int> resolve_action_mapping(const std::string& header,
                                                  char delimiter) {
    std::map<std::string, int> mapping;
    auto aliases = action_aliases();
    auto fields = split_line(header, delimiter);
    for (size_t i = 0; i < fields.size(); ++i) {
        auto normalized = normalize_header(fields[i]);
        auto canonical = apply_alias(normalized, aliases);
        mapping[canonical] = static_cast<int>(i);
    }
    return mapping;
}

std::vector<CorporateAction> parse_actions_file(const CSVDataSource::Config& config,
                                                [[maybe_unused]] SymbolId symbol,
                                                const std::string& path) {
    if (path.empty()) {
        return {};
    }
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    std::string line;
    std::map<std::string, int> col;
    if (config.has_header) {
        if (!std::getline(file, line)) {
            return {};
        }
        col = resolve_action_mapping(line, config.delimiter);
    }

    int type_col = col.count("action_type") ? col["action_type"] : 0;
    int date_col = col.count("effective_date") ? col["effective_date"] : 1;
    int factor_col = col.count("factor") ? col["factor"] : -1;
    int amount_col = col.count("amount") ? col["amount"] : -1;
    int new_symbol_col = col.count("new_symbol") ? col["new_symbol"] : -1;

    std::vector<CorporateAction> actions;
    size_t line_number = config.has_header ? 2 : 1;
    while (std::getline(file, line)) {
        auto fields = split_line(line, config.delimiter);
        int max_col = std::max(type_col, date_col);
        if (factor_col >= 0) max_col = std::max(max_col, factor_col);
        if (amount_col >= 0) max_col = std::max(max_col, amount_col);
        if (new_symbol_col >= 0) max_col = std::max(max_col, new_symbol_col);
        if (static_cast<int>(fields.size()) <= max_col) {
            ++line_number;
            continue;
        }

        CorporateAction action;
        action.type = parse_action_type(fields[type_col]);
        action.effective_date = parse_timestamp(fields[date_col],
                                                config.actions_datetime_format,
                                                config.actions_date_format,
                                                line_number);
        if (factor_col >= 0 && !trim(fields[factor_col]).empty()) {
            action.factor = std::stod(fields[factor_col]);
        }
        if (amount_col >= 0 && !trim(fields[amount_col]).empty()) {
            action.amount = std::stod(fields[amount_col]);
        }
        if (new_symbol_col >= 0) {
            action.new_symbol = trim(fields[new_symbol_col]);
        }
        actions.push_back(std::move(action));
        ++line_number;
    }

    std::sort(actions.begin(), actions.end(),
              [](const CorporateAction& a, const CorporateAction& b) {
                  return a.effective_date < b.effective_date;
              });
    return actions;
}

std::vector<CorporateAction> parse_actions_fallback(const CSVDataSource::Config& config,
                                                    const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }
    std::string line;
    if (config.has_header) {
        std::getline(file, line);
    }
    std::vector<CorporateAction> actions;
    size_t line_number = config.has_header ? 2 : 1;
    while (std::getline(file, line)) {
        auto fields = split_line(line, config.delimiter);
        if (fields.size() < 2) {
            ++line_number;
            continue;
        }
        CorporateAction action;
        action.type = parse_action_type(fields[0]);
        action.effective_date = parse_timestamp(fields[1],
                                                config.actions_datetime_format,
                                                config.actions_date_format,
                                                line_number);
        if (fields.size() > 2 && !trim(fields[2]).empty()) {
            action.factor = std::stod(fields[2]);
        }
        if (fields.size() > 3 && !trim(fields[3]).empty()) {
            action.amount = std::stod(fields[3]);
        }
        if (fields.size() > 4) {
            action.new_symbol = trim(fields[4]);
        }
        actions.push_back(std::move(action));
        ++line_number;
    }
    std::sort(actions.begin(), actions.end(),
              [](const CorporateAction& a, const CorporateAction& b) {
                  return a.effective_date < b.effective_date;
              });
    return actions;
}

}  // namespace

CSVDataSource::CSVDataSource(const Config& config) : config_(config) {
    scan_directory();
}

std::vector<SymbolInfo> CSVDataSource::get_available_symbols() const {
    std::vector<SymbolInfo> info;
    std::unordered_set<SymbolId> seen;
    for (const auto& [id, path] : symbol_to_path_) {
        for (auto alias : adjuster_.aliases_for(id)) {
            if (!seen.insert(alias).second) {
                continue;
            }
            SymbolInfo symbol;
            symbol.id = alias;
            symbol.ticker = SymbolRegistry::instance().lookup(alias);
            info.push_back(symbol);
        }
    }
    return info;
}

TimeRange CSVDataSource::get_available_range(SymbolId symbol) const {
    TimeRange range;
    const std::string path = resolve_path(symbol);
    if (path.empty()) {
        return range;
    }
    auto resolved = adjuster_.resolve_symbol(symbol);
    auto bars = parse_bars(resolved, path, range, BarType::Time_1Day);
    if (!bars.empty()) {
        range.start = bars.front().timestamp;
        range.end = bars.back().timestamp;
    }
    return range;
}

std::vector<Bar> CSVDataSource::get_bars(SymbolId symbol, TimeRange range, BarType bar_type) {
    const std::string path = resolve_path(symbol);
    if (path.empty()) {
        return {};
    }
    auto resolved = adjuster_.resolve_symbol(symbol, range.start);
    ensure_actions_loaded(resolved);
    return parse_bars(resolved, path, range, bar_type);
}

std::vector<Tick> CSVDataSource::get_ticks(SymbolId, TimeRange) {
    return {};
}

std::unique_ptr<DataIterator> CSVDataSource::create_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) {
    std::vector<std::unique_ptr<DataIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto resolved = adjuster_.resolve_symbol(symbol, range.start);
        auto bars = parse_bars(resolved, resolve_path(symbol), range, bar_type);
        iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars)));
    }
    return std::make_unique<MergedBarIterator>(std::move(iterators));
}

std::vector<CorporateAction> CSVDataSource::get_corporate_actions(SymbolId symbol, TimeRange range) {
    symbol = adjuster_.resolve_symbol(symbol);
    ensure_actions_loaded(symbol);
    auto cached = action_cache_.find(symbol);
    if (cached == action_cache_.end()) {
        return {};
    }
    std::vector<CorporateAction> filtered;
    for (const auto& action : cached->second) {
        if ((range.start.microseconds() == 0 && range.end.microseconds() == 0) ||
            range.contains(action.effective_date)) {
            filtered.push_back(action);
        }
    }
    return filtered;
}

void CSVDataSource::set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions) {
    adjuster_.add_actions(symbol, std::move(actions));
}

void CSVDataSource::ensure_actions_loaded(SymbolId symbol) const {
    if (action_cache_.find(symbol) != action_cache_.end()) {
        return;
    }
    std::vector<CorporateAction> actions;
    std::string directory = config_.actions_directory.empty()
        ? config_.data_directory
        : config_.actions_directory;
    if (!directory.empty()) {
        auto symbol_str = SymbolRegistry::instance().lookup(symbol);
        std::string filename = config_.actions_file_pattern;
        auto pos = filename.find("{symbol}");
        if (pos != std::string::npos) {
            filename.replace(pos, 8, symbol_str);
        }
        std::filesystem::path resolved = std::filesystem::path(directory) / filename;
        actions = parse_actions_file(config_, symbol, resolved.string());
        if (actions.empty()) {
            std::filesystem::path fallback = std::filesystem::path(directory)
                / (symbol_str + "_actions.csv");
            actions = parse_actions_fallback(config_, fallback.string());
        }
    }
    if (!actions.empty()) {
        adjuster_.add_actions(symbol, actions);
    }
    action_cache_.emplace(symbol, std::move(actions));
}

std::string CSVDataSource::resolve_path(SymbolId symbol) const {
    SymbolId resolved = adjuster_.resolve_symbol(symbol);
    symbol = resolved;
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

void CSVDataSource::scan_directory() {
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

std::map<std::string, int> CSVDataSource::resolve_mapping(const std::string& header) const {
    std::map<std::string, int> mapping;
    auto aliases = config_.column_aliases.empty() ? default_aliases() : config_.column_aliases;
    auto fields = split_line(header, config_.delimiter);
    for (size_t i = 0; i < fields.size(); ++i) {
        auto normalized = normalize_header(fields[i]);
        auto canonical = apply_alias(normalized, aliases);
        mapping[canonical] = static_cast<int>(i);
    }
    return mapping;
}

std::vector<Bar> CSVDataSource::parse_bars(SymbolId symbol, const std::string& path,
                                           TimeRange range, BarType bar_type) const {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    last_report_ = ValidationReport();
    std::string line;
    std::map<std::string, int> col = config_.column_mapping;
    auto aliases = config_.column_aliases.empty() ? default_aliases() : config_.column_aliases;
    if (config_.has_header) {
        if (!std::getline(file, line)) {
            return {};
        }
        if (col.empty()) {
            col = resolve_mapping(line);
        }
    }
    ensure_required_columns(col);
    int ts_col = col.count("timestamp") ? col["timestamp"] : 0;
    int open_col = col.count("open") ? col["open"] : 1;
    int high_col = col.count("high") ? col["high"] : 2;
    int low_col = col.count("low") ? col["low"] : 3;
    int close_col = col.count("close") ? col["close"] : 4;
    int vol_col = col.count("volume") ? col["volume"] : 5;
    int symbol_col = col.count(config_.symbol_column) ? col[config_.symbol_column] : -1;

    std::vector<Bar> bars;
    Timestamp last_ts;
    bool has_last_ts = false;
    double last_close = 0.0;
    bool has_last_close = false;
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
    bool fill_on_gap = false;
    auto error_action = [&]() {
        auto action = config_.validation.on_error;
        if (action == ValidationAction::Continue || action == ValidationAction::Fill) {
            action = ValidationAction::Skip;
        }
        return action;
    };

    while (std::getline(file, line)) {
        auto fields = split_line(line, config_.delimiter);
        int max_col = std::max({ts_col, open_col, high_col, low_col, close_col, vol_col});
        if (config_.allow_symbol_column && symbol_col >= 0) {
            max_col = std::max(max_col, symbol_col);
        }
        if (static_cast<int>(fields.size()) <= max_col) {
            if (!handle_issue(ValidationSeverity::Error, error_action(), "Missing columns")) {
                ++line_number;
                continue;
            }
        }

        Bar bar;
        try {
            bar.timestamp = parse_timestamp(fields[ts_col], config_.datetime_format,
                                            config_.date_format, line_number);
            if (config_.utc_offset_seconds != 0) {
                bar.timestamp = bar.timestamp - Duration::seconds(config_.utc_offset_seconds);
            }
            bar.open = std::stod(fields[open_col]);
            bar.high = std::stod(fields[high_col]);
            bar.low = std::stod(fields[low_col]);
            bar.close = std::stod(fields[close_col]);
            bar.volume = static_cast<Volume>(std::stoull(fields[vol_col]));
        } catch (const std::exception& ex) {
            if (!handle_issue(ValidationSeverity::Error, error_action(), ex.what())) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.check_price_bounds) {
            if (!std::isfinite(bar.open) || !std::isfinite(bar.high) ||
                !std::isfinite(bar.low) || !std::isfinite(bar.close) ||
                bar.open <= 0 || bar.high <= 0 || bar.low <= 0 || bar.close <= 0 ||
                (config_.validation.max_price > 0.0 &&
                 (bar.open > config_.validation.max_price ||
                  bar.high > config_.validation.max_price ||
                  bar.low > config_.validation.max_price ||
                  bar.close > config_.validation.max_price)) ||
                (bar.high < bar.low || bar.open < bar.low || bar.open > bar.high ||
                 bar.close < bar.low || bar.close > bar.high)) {
                if (!handle_issue(ValidationSeverity::Error, error_action(),
                                  "OHLC out of range")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_volume_bounds && config_.validation.max_volume > 0 &&
            bar.volume > config_.validation.max_volume) {
            if (!handle_issue(ValidationSeverity::Error, error_action(),
                              "Volume exceeds max_volume")) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.check_future_timestamps) {
            if (bar.timestamp > now + config_.validation.max_future_skew) {
                if (!handle_issue(ValidationSeverity::Error, error_action(),
                                  "Timestamp is in the future")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_trading_hours &&
            !within_trading_hours(bar.timestamp,
                                  config_.validation.trading_start_seconds,
                                  config_.validation.trading_end_seconds)) {
            if (!handle_issue(ValidationSeverity::Error, error_action(),
                              "Timestamp outside trading hours")) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.require_monotonic_timestamps && has_last_ts &&
            bar.timestamp < last_ts) {
            if (!handle_issue(ValidationSeverity::Error, error_action(),
                              "Non-monotonic timestamp")) {
                ++line_number;
                continue;
            }
        }

        if (config_.validation.check_gap && has_last_ts) {
            Duration gap = bar.timestamp - last_ts;
            if (gap.total_microseconds() > config_.validation.max_gap.total_microseconds()) {
                auto action = config_.validation.on_gap;
                if (action == ValidationAction::Fill) {
                    fill_on_gap = true;
                }
                if (!handle_issue(ValidationSeverity::Warning, action,
                                  "Timestamp gap exceeds max_gap")) {
                    ++line_number;
                    continue;
                }
            }
        }

        if (config_.validation.check_price_jump && has_last_close && last_close != 0.0) {
            double jump = std::abs(bar.close - last_close) / std::abs(last_close);
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
                double zscore = std::abs(bar.close - price_stats.mean) / stddev;
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
                double zscore = std::abs(static_cast<double>(bar.volume) - volume_stats.mean) /
                                stddev;
                if (zscore > config_.validation.outlier_zscore) {
                    handle_issue(ValidationSeverity::Warning, config_.validation.on_warning,
                                 "Volume outlier detected");
                }
            }
        }

        if (config_.allow_symbol_column && symbol_col >= 0) {
            auto sym = trim(fields[symbol_col]);
            if (sym.empty()) {
                if (!handle_issue(ValidationSeverity::Error, error_action(), "Empty symbol")) {
                    ++line_number;
                    continue;
                }
            }
            bar.symbol = SymbolRegistry::instance().intern(sym);
        } else {
            bar.symbol = symbol;
        }
        last_ts = bar.timestamp;
        has_last_ts = true;
        last_close = bar.close;
        has_last_close = true;
        if (config_.validation.check_outliers) {
            price_stats.push(bar.close);
            volume_stats.push(static_cast<double>(bar.volume));
        }

        if (range.start.microseconds() != 0 || range.end.microseconds() != 0) {
            if (!range.contains(bar.timestamp)) {
                ++line_number;
                continue;
            }
        }

        bars.push_back(adjuster_.adjust_bar(bar.symbol, bar));
        ++line_number;
    }

    if (config_.fill_missing_bars || fill_on_gap) {
        auto interval = bar_interval_for(bar_type);
        if (interval.has_value()) {
            return fill_missing_time_bars(bars, *interval);
        }
    }
    return bars;
}

}  // namespace regimeflow::data
