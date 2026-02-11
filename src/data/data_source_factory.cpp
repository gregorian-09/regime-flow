#include "regimeflow/data/data_source_factory.h"
#include "regimeflow/data/metadata_data_source.h"
#include "regimeflow/data/symbol_metadata.h"
#include "regimeflow/plugins/interfaces.h"
#include "regimeflow/plugins/registry.h"

#include <sstream>

namespace regimeflow::data {

namespace {

ValidationAction parse_action(const std::string& value) {
    if (value == "skip") {
        return ValidationAction::Skip;
    }
    if (value == "fill") {
        return ValidationAction::Fill;
    }
    if (value == "continue") {
        return ValidationAction::Continue;
    }
    return ValidationAction::Fail;
}

void parse_validation_config(const Config& cfg, ValidationConfig& out) {
    if (auto v = cfg.get_as<bool>("validation.require_monotonic_timestamps")) {
        out.require_monotonic_timestamps = *v;
    }
    if (auto v = cfg.get_as<bool>("validation.check_price_bounds")) {
        out.check_price_bounds = *v;
    }
    if (auto v = cfg.get_as<bool>("validation.check_gap")) {
        out.check_gap = *v;
    }
    if (auto v = cfg.get_as<int64_t>("validation.max_gap_seconds")) {
        out.max_gap = Duration::seconds(*v);
    }
    if (auto v = cfg.get_as<bool>("validation.check_price_jump")) {
        out.check_price_jump = *v;
    }
    if (auto v = cfg.get_as<double>("validation.max_jump_pct")) {
        out.max_jump_pct = *v;
    }
    if (auto v = cfg.get_as<bool>("validation.check_future_timestamps")) {
        out.check_future_timestamps = *v;
    }
    if (auto v = cfg.get_as<int64_t>("validation.max_future_skew_seconds")) {
        out.max_future_skew = Duration::seconds(*v);
    }
    if (auto v = cfg.get_as<bool>("validation.check_trading_hours")) {
        out.check_trading_hours = *v;
    }
    if (auto v = cfg.get_as<int64_t>("validation.trading_start_seconds")) {
        out.trading_start_seconds = static_cast<int>(*v);
    }
    if (auto v = cfg.get_as<int64_t>("validation.trading_end_seconds")) {
        out.trading_end_seconds = static_cast<int>(*v);
    }
    if (auto v = cfg.get_as<bool>("validation.check_volume_bounds")) {
        out.check_volume_bounds = *v;
    }
    if (auto v = cfg.get_as<int64_t>("validation.max_volume")) {
        if (*v > 0) {
            out.max_volume = static_cast<uint64_t>(*v);
        }
    }
    if (auto v = cfg.get_as<double>("validation.max_price")) {
        out.max_price = *v;
    }
    if (auto v = cfg.get_as<bool>("validation.check_outliers")) {
        out.check_outliers = *v;
    }
    if (auto v = cfg.get_as<double>("validation.outlier_zscore")) {
        out.outlier_zscore = *v;
    }
    if (auto v = cfg.get_as<int64_t>("validation.outlier_warmup")) {
        if (*v > 0) {
            out.outlier_warmup = static_cast<size_t>(*v);
        }
    }
    if (auto v = cfg.get_as<std::string>("validation.on_error")) {
        out.on_error = parse_action(*v);
    }
    if (auto v = cfg.get_as<std::string>("validation.on_gap")) {
        out.on_gap = parse_action(*v);
    }
    if (auto v = cfg.get_as<std::string>("validation.on_warning")) {
        out.on_warning = parse_action(*v);
    }
}

}  // namespace

std::unique_ptr<DataSource> DataSourceFactory::create(const Config& config) {
    auto type = config.get_as<std::string>("type").value_or("csv");
    std::unique_ptr<DataSource> source;
    if (type == "csv") {
        source = std::make_unique<CSVDataSource>(parse_csv_config(config));
    } else if (type == "tick_csv") {
        source = std::make_unique<CSVTickDataSource>(parse_tick_csv_config(config));
    } else if (type == "memory") {
        source = std::make_unique<MemoryDataSource>();
    } else if (type == "mmap") {
        MemoryMappedDataSource::Config mmap_cfg;
        if (auto v = config.get_as<std::string>("data_directory")) mmap_cfg.data_directory = *v;
        if (auto v = config.get_as<bool>("preload_index")) mmap_cfg.preload_index = *v;
        if (auto v = config.get_as<int64_t>("max_cached_files")) {
            if (*v > 0) {
                mmap_cfg.max_cached_files = static_cast<size_t>(*v);
            }
        }
        if (auto v = config.get_as<int64_t>("max_cached_ranges")) {
            if (*v > 0) {
                mmap_cfg.max_cached_ranges = static_cast<size_t>(*v);
            }
        }
        source = std::make_unique<MemoryMappedDataSource>(mmap_cfg);
    } else if (type == "mmap_ticks") {
        TickMmapDataSource::Config tick_cfg;
        if (auto v = config.get_as<std::string>("data_directory")) tick_cfg.data_directory = *v;
        if (auto v = config.get_as<int64_t>("max_cached_files")) {
            if (*v > 0) {
                tick_cfg.max_cached_files = static_cast<size_t>(*v);
            }
        }
        if (auto v = config.get_as<int64_t>("max_cached_ranges")) {
            if (*v > 0) {
                tick_cfg.max_cached_ranges = static_cast<size_t>(*v);
            }
        }
        source = std::make_unique<TickMmapDataSource>(tick_cfg);
    } else if (type == "mmap_books") {
        OrderBookMmapDataSource::Config book_cfg;
        if (auto v = config.get_as<std::string>("data_directory")) book_cfg.data_directory = *v;
        if (auto v = config.get_as<int64_t>("max_cached_files")) {
            if (*v > 0) {
                book_cfg.max_cached_files = static_cast<size_t>(*v);
            }
        }
        if (auto v = config.get_as<int64_t>("max_cached_ranges")) {
            if (*v > 0) {
                book_cfg.max_cached_ranges = static_cast<size_t>(*v);
            }
        }
        source = std::make_unique<OrderBookMmapDataSource>(book_cfg);
    } else if (type == "api") {
        ApiDataSource::Config api;
        if (auto v = config.get_as<std::string>("base_url")) api.base_url = *v;
        if (auto v = config.get_as<std::string>("bars_endpoint")) api.bars_endpoint = *v;
        if (auto v = config.get_as<std::string>("ticks_endpoint")) api.ticks_endpoint = *v;
        if (auto v = config.get_as<std::string>("api_key")) api.api_key = *v;
        if (auto v = config.get_as<std::string>("api_key_header")) api.api_key_header = *v;
        if (auto v = config.get_as<std::string>("format")) api.format = *v;
        if (auto v = config.get_as<std::string>("time_format")) api.time_format = *v;
        if (auto v = config.get_as<int64_t>("timeout_seconds")) {
            api.timeout_seconds = static_cast<int>(*v);
        }
        if (auto v = config.get_as<bool>("collect_validation_report")) {
            api.collect_validation_report = *v;
        }
        if (auto v = config.get_as<bool>("fill_missing_bars")) api.fill_missing_bars = *v;
        parse_validation_config(config, api.validation);
        if (auto v = config.get_as<std::string>("symbols")) {
            std::string token;
            std::istringstream stream(*v);
            while (std::getline(stream, token, ',')) {
                if (!token.empty()) {
                    api.symbols.push_back(token);
                }
            }
        }
        source = std::make_unique<ApiDataSource>(std::move(api));
    } else if (type == "database" || type == "db") {
        DatabaseDataSource::Config db;
        if (auto v = config.get_as<std::string>("connection_string")) db.connection_string = *v;
        if (auto v = config.get_as<std::string>("bars_table")) db.bars_table = *v;
        if (auto v = config.get_as<std::string>("ticks_table")) db.ticks_table = *v;
        if (auto v = config.get_as<std::string>("actions_table")) db.actions_table = *v;
        if (auto v = config.get_as<std::string>("order_books_table")) db.order_books_table = *v;
        if (auto v = config.get_as<std::string>("symbols_table")) db.symbols_table = *v;
        if (auto v = config.get_as<int64_t>("connection_pool_size")) {
            db.connection_pool_size = static_cast<int>(*v);
        }
        if (auto v = config.get_as<bool>("bars_has_bar_type")) db.bars_has_bar_type = *v;
        if (auto v = config.get_as<bool>("collect_validation_report")) {
            db.collect_validation_report = *v;
        }
        if (auto v = config.get_as<bool>("fill_missing_bars")) db.fill_missing_bars = *v;
        parse_validation_config(config, db.validation);
        source = std::make_unique<DatabaseDataSource>(db);
    }
    if (!source) {
        Config plugin_cfg = config;
        if (auto params = config.get_as<ConfigValue::Object>("params")) {
            plugin_cfg = Config(*params);
        }
        auto plugin = plugins::PluginRegistry::instance()
            .create<plugins::DataSourcePlugin>("data_source", type, plugin_cfg);
        if (plugin) {
            source = plugin->create_data_source();
        }
    }
    if (!source) {
        return nullptr;
    }

    auto csv_path = config.get_as<std::string>("symbol_metadata_csv");
    auto csv_delim = config.get_as<std::string>("symbol_metadata_delimiter");
    auto csv_header = config.get_as<bool>("symbol_metadata_has_header");
    char delimiter = ',';
    if (csv_delim && !csv_delim->empty()) {
        delimiter = (*csv_delim)[0];
    }
    SymbolMetadataMap csv_metadata;
    if (csv_path && !csv_path->empty()) {
        csv_metadata = load_symbol_metadata_csv(*csv_path, delimiter,
                                                csv_header.value_or(true));
    }
    SymbolMetadataMap config_metadata = load_symbol_metadata_config(config);
    if (!csv_metadata.empty() || !config_metadata.empty()) {
        return std::make_unique<MetadataOverlayDataSource>(std::move(source),
                                                           std::move(csv_metadata),
                                                           std::move(config_metadata));
    }
    return source;
}

CSVDataSource::Config DataSourceFactory::parse_csv_config(const Config& cfg) {
    CSVDataSource::Config out;
    if (auto v = cfg.get_as<std::string>("data_directory")) out.data_directory = *v;
    if (auto v = cfg.get_as<std::string>("file_pattern")) out.file_pattern = *v;
    if (auto v = cfg.get_as<std::string>("actions_directory")) out.actions_directory = *v;
    if (auto v = cfg.get_as<std::string>("actions_file_pattern")) {
        out.actions_file_pattern = *v;
    }
    if (auto v = cfg.get_as<std::string>("date_format")) out.date_format = *v;
    if (auto v = cfg.get_as<std::string>("datetime_format")) out.datetime_format = *v;
    if (auto v = cfg.get_as<std::string>("actions_date_format")) {
        out.actions_date_format = *v;
    }
    if (auto v = cfg.get_as<std::string>("actions_datetime_format")) {
        out.actions_datetime_format = *v;
    }
    if (auto v = cfg.get_as<std::string>("delimiter")) out.delimiter = !v->empty() ? (*v)[0] : ',';
    if (auto v = cfg.get_as<bool>("has_header")) out.has_header = *v;
    if (auto v = cfg.get_as<bool>("collect_validation_report")) out.collect_validation_report = *v;
    if (auto v = cfg.get_as<bool>("allow_symbol_column")) out.allow_symbol_column = *v;
    if (auto v = cfg.get_as<std::string>("symbol_column")) out.symbol_column = *v;
    if (auto v = cfg.get_as<int64_t>("utc_offset_seconds")) {
        out.utc_offset_seconds = static_cast<int>(*v);
    }
    if (auto v = cfg.get_as<bool>("fill_missing_bars")) out.fill_missing_bars = *v;

    parse_validation_config(cfg, out.validation);

    return out;
}

CSVTickDataSource::Config DataSourceFactory::parse_tick_csv_config(const Config& cfg) {
    CSVTickDataSource::Config out;
    if (auto v = cfg.get_as<std::string>("data_directory")) out.data_directory = *v;
    if (auto v = cfg.get_as<std::string>("file_pattern")) out.file_pattern = *v;
    if (auto v = cfg.get_as<std::string>("datetime_format")) out.datetime_format = *v;
    if (auto v = cfg.get_as<std::string>("delimiter")) out.delimiter = !v->empty() ? (*v)[0] : ',';
    if (auto v = cfg.get_as<bool>("has_header")) out.has_header = *v;
    if (auto v = cfg.get_as<bool>("collect_validation_report")) out.collect_validation_report = *v;
    if (auto v = cfg.get_as<int64_t>("utc_offset_seconds")) {
        out.utc_offset_seconds = static_cast<int>(*v);
    }

    parse_validation_config(cfg, out.validation);

    return out;
}

}  // namespace regimeflow::data
