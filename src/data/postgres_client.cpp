#include "regimeflow/data/postgres_client.h"

#include "regimeflow/common/types.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#ifdef REGIMEFLOW_USE_LIBPQ
#include <libpq-fe.h>
#endif

namespace regimeflow::data {

ConnectionPool::ConnectionPool(std::string connection_string, int size)
    : connection_string_(std::move(connection_string)), size_(size) {}

ConnectionPool::~ConnectionPool() {
    while (!pool_.empty()) {
        destroy_connection(pool_.front());
        pool_.pop();
    }
}

void* ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!pool_.empty()) {
        auto* conn = pool_.front();
        pool_.pop();
        return conn;
    }
    if (total_ < size_) {
        ++total_;
        lock.unlock();
        return create_connection();
    }
    cv_.wait(lock, [this] { return !pool_.empty(); });
    auto* conn = pool_.front();
    pool_.pop();
    return conn;
}

void ConnectionPool::release(void* connection) {
    if (!connection) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(connection);
    }
    cv_.notify_one();
}

void* ConnectionPool::create_connection() const {
#ifdef REGIMEFLOW_USE_LIBPQ
    PGconn* conn = PQconnectdb(connection_string_.c_str());
    if (!conn || PQstatus(conn) != CONNECTION_OK) {
        if (conn) {
            PQfinish(conn);
        }
        throw std::runtime_error("Failed to connect to Postgres");
    }
    return conn;
#else
    throw std::runtime_error("Postgres support not enabled");
#endif
}

void ConnectionPool::destroy_connection(void* connection) const {
#ifdef REGIMEFLOW_USE_LIBPQ
    if (connection) {
        PQfinish(static_cast<PGconn*>(connection));
    }
#else
    (void)connection;
#endif
}

PostgresDbClient::PostgresDbClient(Config config)
    : config_(std::move(config)), pool_(config_.connection_string, config_.connection_pool_size) {
    if (config_.connection_string.empty()) {
        throw std::invalid_argument("Postgres connection_string is required");
    }
}

std::vector<Bar> PostgresDbClient::query_bars(SymbolId symbol, TimeRange range, BarType bar_type) {
    std::vector<Bar> out;
#ifdef REGIMEFLOW_USE_LIBPQ
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return out;
    }

    std::string sql = "SELECT timestamp, open, high, low, close, volume, trade_count, vwap "
                      "FROM " + config_.bars_table + " WHERE symbol = $1 "
                      "AND timestamp >= $2 AND timestamp <= $3";
    std::vector<std::string> params = {
        symbol_name,
        std::to_string(range.start.microseconds()),
        std::to_string(range.end.microseconds())
    };
    if (config_.bars_has_bar_type) {
        sql += " AND bar_type = $4";
        params.push_back(std::to_string(static_cast<int>(bar_type)));
    }
    sql += " ORDER BY timestamp ASC";

    auto result = execute_query(sql, params, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        int rows = PQntuples(res);
        out.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            Bar bar;
            bar.timestamp = Timestamp(std::stoll(PQgetvalue(res, i, 0)));
            bar.symbol = symbol;
            bar.open = std::stod(PQgetvalue(res, i, 1));
            bar.high = std::stod(PQgetvalue(res, i, 2));
            bar.low = std::stod(PQgetvalue(res, i, 3));
            bar.close = std::stod(PQgetvalue(res, i, 4));
            bar.volume = static_cast<uint64_t>(std::stoull(PQgetvalue(res, i, 5)));
            if (!PQgetisnull(res, i, 6)) {
                bar.trade_count = static_cast<uint64_t>(std::stoull(PQgetvalue(res, i, 6)));
            }
            if (!PQgetisnull(res, i, 7)) {
                bar.vwap = std::stod(PQgetvalue(res, i, 7));
            }
            out.push_back(bar);
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#else
    (void)symbol;
    (void)range;
    (void)bar_type;
#endif
    return out;
}

std::vector<Tick> PostgresDbClient::query_ticks(SymbolId symbol, TimeRange range) {
    std::vector<Tick> out;
#ifdef REGIMEFLOW_USE_LIBPQ
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return out;
    }

    std::string sql = "SELECT timestamp, price, quantity, flags FROM " + config_.ticks_table
                      + " WHERE symbol = $1 AND timestamp >= $2 AND timestamp <= $3 "
                        "ORDER BY timestamp ASC";
    std::vector<std::string> params = {
        symbol_name,
        std::to_string(range.start.microseconds()),
        std::to_string(range.end.microseconds())
    };

    auto result = execute_query(sql, params, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        int rows = PQntuples(res);
        out.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            Tick tick;
            tick.timestamp = Timestamp(std::stoll(PQgetvalue(res, i, 0)));
            tick.symbol = symbol;
            tick.price = std::stod(PQgetvalue(res, i, 1));
            tick.quantity = std::stod(PQgetvalue(res, i, 2));
            if (!PQgetisnull(res, i, 3)) {
                tick.flags = static_cast<uint8_t>(std::stoul(PQgetvalue(res, i, 3)));
            }
            out.push_back(tick);
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#else
    (void)symbol;
    (void)range;
#endif
    return out;
}

std::vector<SymbolInfo> PostgresDbClient::list_symbols() {
    std::vector<SymbolInfo> out;
#ifdef REGIMEFLOW_USE_LIBPQ
    std::string sql = "SELECT DISTINCT symbol FROM " + config_.bars_table + " ORDER BY symbol";
    auto result = execute_query(sql, {}, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        int rows = PQntuples(res);
        out.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            std::string symbol = PQgetvalue(res, i, 0);
            SymbolInfo info;
            info.id = SymbolRegistry::instance().intern(symbol);
            info.ticker = symbol;
            out.push_back(info);
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#endif
    return out;
}

std::vector<SymbolInfo> PostgresDbClient::list_symbols_with_metadata(
    const std::string& symbols_table) {
    if (symbols_table.empty()) {
        return list_symbols();
    }
    std::vector<SymbolInfo> out;
#ifdef REGIMEFLOW_USE_LIBPQ
    std::string sql = "SELECT symbol, exchange, asset_class, currency, tick_size, lot_size, "
                      "multiplier, sector, industry FROM " + symbols_table + " ORDER BY symbol";
    auto result = execute_query(sql, {}, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        int rows = PQntuples(res);
        out.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            SymbolInfo info;
            std::string symbol = PQgetvalue(res, i, 0);
            info.id = SymbolRegistry::instance().intern(symbol);
            info.ticker = symbol;
            if (!PQgetisnull(res, i, 1)) {
                info.exchange = PQgetvalue(res, i, 1);
            }
            if (!PQgetisnull(res, i, 2)) {
                std::string asset = PQgetvalue(res, i, 2);
                std::transform(asset.begin(), asset.end(), asset.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (asset == "equity" || asset == "stock") info.asset_class = AssetClass::Equity;
                else if (asset == "futures" || asset == "future") info.asset_class = AssetClass::Futures;
                else if (asset == "forex" || asset == "fx") info.asset_class = AssetClass::Forex;
                else if (asset == "crypto" || asset == "cryptocurrency") info.asset_class = AssetClass::Crypto;
                else if (asset == "options" || asset == "option") info.asset_class = AssetClass::Options;
                else info.asset_class = AssetClass::Other;
            }
            if (!PQgetisnull(res, i, 3)) {
                info.currency = PQgetvalue(res, i, 3);
            }
            if (!PQgetisnull(res, i, 4)) {
                info.tick_size = std::stod(PQgetvalue(res, i, 4));
            }
            if (!PQgetisnull(res, i, 5)) {
                info.lot_size = std::stod(PQgetvalue(res, i, 5));
            }
            if (!PQgetisnull(res, i, 6)) {
                info.multiplier = std::stod(PQgetvalue(res, i, 6));
            }
            if (!PQgetisnull(res, i, 7)) {
                info.sector = PQgetvalue(res, i, 7);
            }
            if (!PQgetisnull(res, i, 8)) {
                info.industry = PQgetvalue(res, i, 8);
            }
            out.push_back(std::move(info));
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#else
    (void)symbols_table;
#endif
    return out;
}

TimeRange PostgresDbClient::get_available_range(SymbolId symbol) {
    TimeRange range;
#ifdef REGIMEFLOW_USE_LIBPQ
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return range;
    }
    std::string sql = "SELECT MIN(timestamp), MAX(timestamp) FROM " + config_.bars_table
                      + " WHERE symbol = $1";
    std::vector<std::string> params = {symbol_name};
    auto result = execute_query(sql, params, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        if (PQntuples(res) == 0) {
            return;
        }
        if (!PQgetisnull(res, 0, 0)) {
            range.start = Timestamp(std::stoll(PQgetvalue(res, 0, 0)));
        }
        if (!PQgetisnull(res, 0, 1)) {
            range.end = Timestamp(std::stoll(PQgetvalue(res, 0, 1)));
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#else
    (void)symbol;
#endif
    return range;
}

std::vector<CorporateAction> PostgresDbClient::query_corporate_actions(SymbolId symbol,
                                                                       TimeRange range) {
    std::vector<CorporateAction> out;
#ifdef REGIMEFLOW_USE_LIBPQ
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return out;
    }

    std::string sql = "SELECT action_type, effective_date, factor, amount, new_symbol FROM "
                      + config_.actions_table + " WHERE symbol = $1 "
                      "AND effective_date >= $2 AND effective_date <= $3 "
                      "ORDER BY effective_date ASC";
    std::vector<std::string> params = {
        symbol_name,
        std::to_string(range.start.microseconds()),
        std::to_string(range.end.microseconds())
    };

    auto result = execute_query(sql, params, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        int rows = PQntuples(res);
        out.reserve(rows);
        for (int i = 0; i < rows; ++i) {
            CorporateAction action;
            action.type = static_cast<CorporateActionType>(
                std::stoi(PQgetvalue(res, i, 0)));
            action.effective_date = Timestamp(std::stoll(PQgetvalue(res, i, 1)));
            if (!PQgetisnull(res, i, 2)) {
                action.factor = std::stod(PQgetvalue(res, i, 2));
            }
            if (!PQgetisnull(res, i, 3)) {
                action.amount = std::stod(PQgetvalue(res, i, 3));
            }
            if (!PQgetisnull(res, i, 4)) {
                action.new_symbol = PQgetvalue(res, i, 4);
            }
            out.push_back(std::move(action));
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#else
    (void)symbol;
    (void)range;
#endif
    return out;
}

std::vector<OrderBook> PostgresDbClient::query_order_books(SymbolId symbol, TimeRange range) {
    std::vector<OrderBook> out;
#ifdef REGIMEFLOW_USE_LIBPQ
    const auto& symbol_name = SymbolRegistry::instance().lookup(symbol);
    if (symbol_name.empty()) {
        return out;
    }

    std::string sql = "SELECT timestamp, level, bid_price, bid_qty, bid_orders, "
                      "ask_price, ask_qty, ask_orders FROM " + config_.order_books_table
                      + " WHERE symbol = $1 AND timestamp >= $2 AND timestamp <= $3 "
                        "ORDER BY timestamp ASC, level ASC";
    std::vector<std::string> params = {
        symbol_name,
        std::to_string(range.start.microseconds()),
        std::to_string(range.end.microseconds())
    };

    auto result = execute_query(sql, params, [&](void* row) {
        auto* res = static_cast<PGresult*>(row);
        int rows = PQntuples(res);
        out.reserve(rows / 10 + 1);
        int64_t current_ts = 0;
        OrderBook current{};
        bool has_current = false;

        for (int i = 0; i < rows; ++i) {
            int64_t ts = std::stoll(PQgetvalue(res, i, 0));
            int level = std::stoi(PQgetvalue(res, i, 1));
            if (level < 0 || level >= static_cast<int>(current.bids.size())) {
                continue;
            }
            if (!has_current || ts != current_ts) {
                if (has_current) {
                    out.push_back(current);
                }
                current = OrderBook{};
                current_ts = ts;
                current.timestamp = Timestamp(ts);
                current.symbol = symbol;
                has_current = true;
            }

            if (!PQgetisnull(res, i, 2)) {
                current.bids[level].price = std::stod(PQgetvalue(res, i, 2));
            }
            if (!PQgetisnull(res, i, 3)) {
                current.bids[level].quantity = std::stod(PQgetvalue(res, i, 3));
            }
            if (!PQgetisnull(res, i, 4)) {
                current.bids[level].num_orders = std::stoi(PQgetvalue(res, i, 4));
            }
            if (!PQgetisnull(res, i, 5)) {
                current.asks[level].price = std::stod(PQgetvalue(res, i, 5));
            }
            if (!PQgetisnull(res, i, 6)) {
                current.asks[level].quantity = std::stod(PQgetvalue(res, i, 6));
            }
            if (!PQgetisnull(res, i, 7)) {
                current.asks[level].num_orders = std::stoi(PQgetvalue(res, i, 7));
            }
        }

        if (has_current) {
            out.push_back(current);
        }
    });
    if (result.is_err()) {
        throw std::runtime_error(result.error().message);
    }
#else
    (void)symbol;
    (void)range;
#endif
    return out;
}

Result<void> PostgresDbClient::execute_query(const std::string& sql,
                                             const std::vector<std::string>& params,
                                             std::function<void(void*)> row_handler) {
#ifdef REGIMEFLOW_USE_LIBPQ
    auto* conn = static_cast<PGconn*>(pool_.acquire());
    if (!conn) {
        return Error(Error::Code::IoError, "Postgres connection unavailable");
    }

    std::vector<const char*> values;
    values.reserve(params.size());
    for (const auto& p : params) {
        values.push_back(p.c_str());
    }

    PGresult* res = PQexecParams(conn, sql.c_str(), static_cast<int>(values.size()),
                                 nullptr, values.data(), nullptr, nullptr, 0);

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::string message = res ? PQresultErrorMessage(res) : "Query failed";
        if (res) {
            PQclear(res);
        }
        pool_.release(conn);
        return Error(Error::Code::IoError, message);
    }

    row_handler(res);
    PQclear(res);
    pool_.release(conn);
    return Ok();
#else
    (void)sql;
    (void)params;
    (void)row_handler;
    return Error(Error::Code::IoError, "Postgres support not enabled");
#endif
}

}  // namespace regimeflow::data
