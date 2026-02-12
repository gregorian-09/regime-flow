/**
 * @file postgres_client.h
 * @brief RegimeFlow regimeflow postgres client declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/db_client.h"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace regimeflow::data {

/**
 * @brief Simple connection pool for PostgreSQL connections.
 */
class ConnectionPool {
public:
    /**
     * @brief Construct a connection pool.
     * @param connection_string Database connection string.
     * @param size Maximum pool size.
     */
    ConnectionPool(std::string connection_string, int size);
    /**
     * @brief Destroy the pool and release connections.
     */
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    /**
     * @brief Acquire a connection (blocks if none available).
     * @return Opaque connection pointer.
     */
    void* acquire();
    /**
     * @brief Return a connection to the pool.
     * @param connection Connection pointer.
     */
    void release(void* connection);

private:
    std::string connection_string_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<void*> pool_;
    int size_ = 0;
    int total_ = 0;

    void* create_connection() const;
    void destroy_connection(void* connection) const;
};

/**
 * @brief PostgreSQL-backed DbClient implementation.
 */
class PostgresDbClient final : public DbClient {
public:
    /**
     * @brief Configuration for the PostgreSQL client.
     */
    struct Config {
        /**
         * @brief Connection string.
         */
        std::string connection_string;
        /**
         * @brief Bars table name.
         */
        std::string bars_table = "market_bars";
        /**
         * @brief Ticks table name.
         */
        std::string ticks_table = "market_ticks";
        /**
         * @brief Corporate actions table name.
         */
        std::string actions_table = "corporate_actions";
        /**
         * @brief Order books table name.
         */
        std::string order_books_table = "order_books";
        /**
         * @brief Connection pool size.
         */
        int connection_pool_size = 4;
        /**
         * @brief Whether bars table includes bar_type column.
         */
        bool bars_has_bar_type = true;
    };

    /**
     * @brief Construct a Postgres client.
     * @param config Configuration.
     */
    explicit PostgresDbClient(Config config);

    /**
     * @brief Query bars for a symbol and range.
     */
    std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;
    /**
     * @brief Query ticks for a symbol and range.
     */
    std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;
    /**
     * @brief List symbols.
     */
    std::vector<SymbolInfo> list_symbols() override;
    /**
     * @brief List symbols with metadata.
     */
    std::vector<SymbolInfo> list_symbols_with_metadata(const std::string& symbols_table) override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) override;
    /**
     * @brief Query corporate actions for a symbol.
     */
    std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                          TimeRange range) override;
    /**
     * @brief Query order books for a symbol.
     */
    std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;

private:
    Config config_;
    ConnectionPool pool_;

    Result<void> execute_query(const std::string& sql,
                               const std::vector<std::string>& params,
                               std::function<void(void*)> row_handler);
};

}  // namespace regimeflow::data
