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

class ConnectionPool {
public:
    ConnectionPool(std::string connection_string, int size);
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    void* acquire();
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

class PostgresDbClient final : public DbClient {
public:
    struct Config {
        std::string connection_string;
        std::string bars_table = "market_bars";
        std::string ticks_table = "market_ticks";
        std::string actions_table = "corporate_actions";
        std::string order_books_table = "order_books";
        int connection_pool_size = 4;
        bool bars_has_bar_type = true;
    };

    explicit PostgresDbClient(Config config);

    std::vector<Bar> query_bars(SymbolId symbol, TimeRange range, BarType bar_type) override;
    std::vector<Tick> query_ticks(SymbolId symbol, TimeRange range) override;
    std::vector<SymbolInfo> list_symbols() override;
    std::vector<SymbolInfo> list_symbols_with_metadata(const std::string& symbols_table) override;
    TimeRange get_available_range(SymbolId symbol) override;
    std::vector<CorporateAction> query_corporate_actions(SymbolId symbol,
                                                          TimeRange range) override;
    std::vector<OrderBook> query_order_books(SymbolId symbol, TimeRange range) override;

private:
    Config config_;
    ConnectionPool pool_;

    Result<void> execute_query(const std::string& sql,
                               const std::vector<std::string>& params,
                               std::function<void(void*)> row_handler);
};

}  // namespace regimeflow::data
