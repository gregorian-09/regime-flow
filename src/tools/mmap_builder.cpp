#include "regimeflow/common/config.h"
#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"
#include "regimeflow/data/bar_builder.h"
#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/data/mmap_writer.h"
#include "regimeflow/data/tick_mmap.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace regimeflow::data {

namespace {

struct Args {
    std::string source = "csv";
    std::string mode = "bars";
    std::string data_dir;
    std::string output_dir;
    std::string connection_string;
    std::string bars_table;
    std::string ticks_table;
    std::string actions_table;
    std::string symbols;
    std::string bar_type = "1d";
    std::string start;
    std::string end;
    uint64_t volume_threshold = 0;
    uint64_t tick_threshold = 0;
    double dollar_threshold = 0.0;
};

void usage() {
    std::cout << "Usage: regimeflow_mmap_builder --source csv|db --data-dir PATH --output-dir PATH \n"
                 "       [--mode bars|ticks] [--connection-string STR] [--symbols AAPL,MSFT] [--bar-type 1d] \n"
                 "       [--start YYYY-MM-DD] [--end YYYY-MM-DD] \n"
                 "       [--volume-threshold N] [--tick-threshold N] [--dollar-threshold N]" << std::endl;
}

std::optional<std::string> arg_value(const std::string& arg, const std::string& key) {
    if (arg.rfind(key + "=", 0) == 0) {
        return arg.substr(key.size() + 1);
    }
    return std::nullopt;
}

Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            usage();
            std::exit(0);
        }
        if (arg == "--source" && i + 1 < argc) {
            args.source = argv[++i];
        } else if (auto source_value = arg_value(arg, "--source")) {
            args.source = *source_value;
        } else if (arg == "--mode" && i + 1 < argc) {
            args.mode = argv[++i];
        } else if (auto mode_value = arg_value(arg, "--mode")) {
            args.mode = *mode_value;
        } else if (arg == "--data-dir" && i + 1 < argc) {
            args.data_dir = argv[++i];
        } else if (auto data_dir_value = arg_value(arg, "--data-dir")) {
            args.data_dir = *data_dir_value;
        } else if (arg == "--output-dir" && i + 1 < argc) {
            args.output_dir = argv[++i];
        } else if (auto output_dir_value = arg_value(arg, "--output-dir")) {
            args.output_dir = *output_dir_value;
        } else if (arg == "--connection-string" && i + 1 < argc) {
            args.connection_string = argv[++i];
        } else if (auto connection_value = arg_value(arg, "--connection-string")) {
            args.connection_string = *connection_value;
        } else if (arg == "--bars-table" && i + 1 < argc) {
            args.bars_table = argv[++i];
        } else if (auto bars_table_value = arg_value(arg, "--bars-table")) {
            args.bars_table = *bars_table_value;
        } else if (arg == "--ticks-table" && i + 1 < argc) {
            args.ticks_table = argv[++i];
        } else if (auto ticks_table_value = arg_value(arg, "--ticks-table")) {
            args.ticks_table = *ticks_table_value;
        } else if (arg == "--actions-table" && i + 1 < argc) {
            args.actions_table = argv[++i];
        } else if (auto actions_table_value = arg_value(arg, "--actions-table")) {
            args.actions_table = *actions_table_value;
        } else if (arg == "--symbols" && i + 1 < argc) {
            args.symbols = argv[++i];
        } else if (auto symbols_value = arg_value(arg, "--symbols")) {
            args.symbols = *symbols_value;
        } else if (arg == "--bar-type" && i + 1 < argc) {
            args.bar_type = argv[++i];
        } else if (auto bar_type_value = arg_value(arg, "--bar-type")) {
            args.bar_type = *bar_type_value;
        } else if (arg == "--start" && i + 1 < argc) {
            args.start = argv[++i];
        } else if (auto start_value = arg_value(arg, "--start")) {
            args.start = *start_value;
        } else if (arg == "--end" && i + 1 < argc) {
            args.end = argv[++i];
        } else if (auto end_value = arg_value(arg, "--end")) {
            args.end = *end_value;
        } else if (arg == "--volume-threshold" && i + 1 < argc) {
            args.volume_threshold = static_cast<uint64_t>(std::stoull(argv[++i]));
        } else if (auto volume_value = arg_value(arg, "--volume-threshold")) {
            args.volume_threshold = static_cast<uint64_t>(std::stoull(*volume_value));
        } else if (arg == "--tick-threshold" && i + 1 < argc) {
            args.tick_threshold = static_cast<uint64_t>(std::stoull(argv[++i]));
        } else if (auto tick_value = arg_value(arg, "--tick-threshold")) {
            args.tick_threshold = static_cast<uint64_t>(std::stoull(*tick_value));
        } else if (arg == "--dollar-threshold" && i + 1 < argc) {
            args.dollar_threshold = std::stod(argv[++i]);
        } else if (auto dollar_value = arg_value(arg, "--dollar-threshold")) {
            args.dollar_threshold = std::stod(*dollar_value);
        }
    }
    return args;
}

std::vector<std::string> split_symbols(const std::string& input) {
    std::vector<std::string> out;
    std::string token;
    std::istringstream stream(input);
    while (std::getline(stream, token, ',')) {
        if (!token.empty()) {
            out.push_back(token);
        }
    }
    return out;
}

std::optional<BarType> parse_bar_type(const std::string& value) {
    if (value == "1m") return BarType::Time_1Min;
    if (value == "5m") return BarType::Time_5Min;
    if (value == "15m") return BarType::Time_15Min;
    if (value == "30m") return BarType::Time_30Min;
    if (value == "1h") return BarType::Time_1Hour;
    if (value == "4h") return BarType::Time_4Hour;
    if (value == "1d") return BarType::Time_1Day;
    if (value == "volume") return BarType::Volume;
    if (value == "tick") return BarType::Tick;
    if (value == "dollar") return BarType::Dollar;
    return std::nullopt;
}

std::string bar_type_suffix(BarType type) {
    switch (type) {
        case BarType::Time_1Min: return "1m";
        case BarType::Time_5Min: return "5m";
        case BarType::Time_15Min: return "15m";
        case BarType::Time_30Min: return "30m";
        case BarType::Time_1Hour: return "1h";
        case BarType::Time_4Hour: return "4h";
        case BarType::Time_1Day: return "1d";
        case BarType::Volume: return "volume";
        case BarType::Tick: return "tick";
        case BarType::Dollar: return "dollar";
    }
    return "1d";
}

TimeRange parse_range(const Args& args, const DataSource& source, SymbolId symbol) {
    TimeRange range;
    if (!args.start.empty()) {
        range.start = Timestamp::from_string(args.start, "%Y-%m-%d");
    }
    if (!args.end.empty()) {
        range.end = Timestamp::from_string(args.end, "%Y-%m-%d");
    }
    if (range.start.microseconds() == 0 || range.end.microseconds() == 0) {
        TimeRange available = source.get_available_range(symbol);
        if (range.start.microseconds() == 0) {
            range.start = available.start;
        }
        if (range.end.microseconds() == 0) {
            range.end = available.end;
        }
    }
    return range;
}

BarBuilder::Config bar_builder_config(BarType bar_type, const Args& args) {
    BarBuilder::Config cfg;
    cfg.type = bar_type;
    cfg.volume_threshold = args.volume_threshold;
    cfg.tick_threshold = args.tick_threshold;
    cfg.dollar_threshold = args.dollar_threshold;
    switch (bar_type) {
        case BarType::Time_1Min: cfg.time_interval_ms = 60'000; break;
        case BarType::Time_5Min: cfg.time_interval_ms = 5 * 60'000; break;
        case BarType::Time_15Min: cfg.time_interval_ms = 15 * 60'000; break;
        case BarType::Time_30Min: cfg.time_interval_ms = 30 * 60'000; break;
        case BarType::Time_1Hour: cfg.time_interval_ms = 60 * 60'000; break;
        case BarType::Time_4Hour: cfg.time_interval_ms = 4 * 60 * 60'000; break;
        case BarType::Time_1Day: cfg.time_interval_ms = 24 * 60 * 60'000; break;
        case BarType::Volume:
        case BarType::Tick:
        case BarType::Dollar:
            break;
    }
    return cfg;
}

}  // namespace

}  // namespace regimeflow::data

int main(int argc, char** argv) {
    using namespace regimeflow;
    using namespace regimeflow::data;

    Args args = parse_args(argc, argv);
    if (args.output_dir.empty() || args.source.empty()) {
        usage();
        return 1;
    }
    if (args.source == "csv" && args.data_dir.empty()) {
        std::cerr << "Missing --data-dir for csv source" << std::endl;
        return 1;
    }
    if (args.source == "db" && args.connection_string.empty()) {
        std::cerr << "Missing --connection-string for db source" << std::endl;
        return 1;
    }

    auto bar_type = parse_bar_type(args.bar_type);
    if (!bar_type) {
        std::cerr << "Invalid bar type" << std::endl;
        return 1;
    }
    if (args.mode != "bars" && args.mode != "ticks") {
        std::cerr << "Invalid mode" << std::endl;
        return 1;
    }

    Config cfg;
    cfg.set("type", args.source);
    if (args.source == "csv") {
        cfg.set("data_directory", args.data_dir);
    } else if (args.source == "db") {
        cfg.set("type", "database");
        cfg.set("connection_string", args.connection_string);
        if (!args.bars_table.empty()) cfg.set("bars_table", args.bars_table);
        if (!args.ticks_table.empty()) cfg.set("ticks_table", args.ticks_table);
        if (!args.actions_table.empty()) cfg.set("actions_table", args.actions_table);
    }

    auto source = DataSourceFactory::create(cfg);
    if (!source) {
        std::cerr << "Failed to create data source" << std::endl;
        return 1;
    }

    std::vector<SymbolInfo> symbols;
    if (!args.symbols.empty()) {
        for (const auto& sym : split_symbols(args.symbols)) {
            SymbolInfo info;
            info.id = SymbolRegistry::instance().intern(sym);
            info.ticker = sym;
            symbols.push_back(info);
        }
    } else {
        symbols = source->get_available_symbols();
    }

    if (symbols.empty()) {
        std::cerr << "No symbols found" << std::endl;
        return 1;
    }

    std::filesystem::create_directories(args.output_dir);

    MmapWriter writer;
    TickMmapWriter tick_writer;
    for (const auto& info : symbols) {
        TimeRange range = parse_range(args, *source, info.id);
        if (args.mode == "ticks") {
            auto ticks = source->get_ticks(info.id, range);
            if (ticks.empty()) {
                continue;
            }
            std::filesystem::path out_path = args.output_dir;
            out_path /= info.ticker + ".rft";
            auto result = tick_writer.write_ticks(out_path.string(), info.ticker, std::move(ticks));
            if (result.is_err()) {
                std::cerr << result.error().to_string() << std::endl;
                return 1;
            }
            continue;
        }

        auto bars = source->get_bars(info.id, range, *bar_type);
        if (bars.empty()) {
            auto ticks = source->get_ticks(info.id, range);
            if (!ticks.empty()) {
                BarBuilder builder(bar_builder_config(*bar_type, args));
                for (const auto& tick : ticks) {
                    if (auto bar = builder.process(tick)) {
                        bars.push_back(*bar);
                    }
                }
                if (auto bar = builder.flush()) {
                    bars.push_back(*bar);
                }
            }
        }
        if (bars.empty()) {
            continue;
        }
        std::filesystem::path out_path = args.output_dir;
        out_path /= info.ticker + "_" + bar_type_suffix(*bar_type) + ".rfb";
        auto result = writer.write_bars(out_path.string(), info.ticker, *bar_type, std::move(bars));
        if (result.is_err()) {
            std::cerr << result.error().to_string() << std::endl;
            return 1;
        }
    }

    return 0;
}
