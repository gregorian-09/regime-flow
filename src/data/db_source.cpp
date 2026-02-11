#include "regimeflow/data/db_source.h"

#include "regimeflow/data/merged_iterator.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/postgres_client.h"
#include "regimeflow/data/symbol_metadata.h"
#include "regimeflow/data/validation_utils.h"

#include <stdexcept>
#include <unordered_set>

namespace regimeflow::data {

DatabaseDataSource::DatabaseDataSource(const Config& config) : config_(config) {
    if (config_.connection_string.empty()) {
        throw std::invalid_argument("Database connection_string is required");
    }
#ifdef REGIMEFLOW_USE_LIBPQ
    PostgresDbClient::Config db;
    db.connection_string = config_.connection_string;
    db.bars_table = config_.bars_table;
    db.ticks_table = config_.ticks_table;
    db.actions_table = config_.actions_table;
    db.order_books_table = config_.order_books_table;
    db.connection_pool_size = config_.connection_pool_size;
    db.bars_has_bar_type = config_.bars_has_bar_type;
    client_ = std::make_shared<PostgresDbClient>(db);
#endif
}

void DatabaseDataSource::set_client(std::shared_ptr<DbClient> client) {
    client_ = std::move(client);
}

void DatabaseDataSource::set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions) {
    adjuster_.add_actions(symbol, std::move(actions));
}
std::vector<SymbolInfo> DatabaseDataSource::get_available_symbols() const {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    auto symbols = client_->list_symbols();
    if (!config_.symbols_table.empty()) {
        auto db_meta = client_->list_symbols_with_metadata(config_.symbols_table);
        apply_symbol_metadata(symbols, metadata_from_symbols(db_meta));
    }
    std::unordered_set<SymbolId> seen;
    std::vector<SymbolInfo> expanded;
    expanded.reserve(symbols.size());
    for (const auto& info : symbols) {
        auto aliases = adjuster_.aliases_for(info.id);
        for (auto alias : aliases) {
            if (!seen.insert(alias).second) {
                continue;
            }
            SymbolInfo entry = info;
            entry.id = alias;
            entry.ticker = SymbolRegistry::instance().lookup(alias);
            expanded.push_back(std::move(entry));
        }
    }
    return expanded;
}

TimeRange DatabaseDataSource::get_available_range(SymbolId symbol) const {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    auto resolved = adjuster_.resolve_symbol(symbol);
    return client_->get_available_range(resolved);
}

std::vector<Bar> DatabaseDataSource::get_bars(SymbolId symbol, TimeRange range, BarType bar_type) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    auto resolved = adjuster_.resolve_symbol(symbol, range.start);
    auto bars = client_->query_bars(resolved, range, bar_type);
    for (auto& bar : bars) {
        bar = adjuster_.adjust_bar(resolved, bar);
    }
    last_report_ = ValidationReport();
    return validate_bars(std::move(bars), bar_type, config_.validation, config_.fill_missing_bars,
                         config_.collect_validation_report, &last_report_);
}

std::vector<Tick> DatabaseDataSource::get_ticks(SymbolId symbol, TimeRange range) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    auto resolved = adjuster_.resolve_symbol(symbol, range.start);
    last_report_ = ValidationReport();
    auto ticks = client_->query_ticks(resolved, range);
    return validate_ticks(std::move(ticks), config_.validation, config_.collect_validation_report,
                          &last_report_);
}

std::vector<OrderBook> DatabaseDataSource::get_order_books(SymbolId symbol, TimeRange range) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    auto resolved = adjuster_.resolve_symbol(symbol, range.start);
    return client_->query_order_books(resolved, range);
}

std::unique_ptr<DataIterator> DatabaseDataSource::create_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range, BarType bar_type) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    std::vector<std::unique_ptr<DataIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto resolved = adjuster_.resolve_symbol(symbol, range.start);
        auto bars = client_->query_bars(resolved, range, bar_type);
        iterators.push_back(std::make_unique<VectorBarIterator>(std::move(bars)));
    }
    return std::make_unique<MergedBarIterator>(std::move(iterators));
}

std::unique_ptr<TickIterator> DatabaseDataSource::create_tick_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    std::vector<std::unique_ptr<TickIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto resolved = adjuster_.resolve_symbol(symbol, range.start);
        auto ticks = client_->query_ticks(resolved, range);
        iterators.push_back(std::make_unique<VectorTickIterator>(std::move(ticks)));
    }
    return std::make_unique<MergedTickIterator>(std::move(iterators));
}

std::unique_ptr<OrderBookIterator> DatabaseDataSource::create_book_iterator(
    const std::vector<SymbolId>& symbols, TimeRange range) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    std::vector<std::unique_ptr<OrderBookIterator>> iterators;
    iterators.reserve(symbols.size());
    for (auto symbol : symbols) {
        auto resolved = adjuster_.resolve_symbol(symbol, range.start);
        auto books = client_->query_order_books(resolved, range);
        iterators.push_back(std::make_unique<VectorOrderBookIterator>(std::move(books)));
    }
    return std::make_unique<MergedOrderBookIterator>(std::move(iterators));
}

std::vector<CorporateAction> DatabaseDataSource::get_corporate_actions(SymbolId symbol,
                                                                       TimeRange range) {
    if (!client_) {
        throw std::runtime_error("Database client not configured");
    }
    return client_->query_corporate_actions(symbol, range);
}

}  // namespace regimeflow::data
