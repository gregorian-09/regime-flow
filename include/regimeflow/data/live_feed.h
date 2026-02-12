/**
 * @file live_feed.h
 * @brief RegimeFlow regimeflow live feed declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Abstract interface for live market data feeds.
 */
class LiveFeedAdapter {
public:
    virtual ~LiveFeedAdapter() = default;

    /**
     * @brief Establish the live feed connection.
     */
    virtual Result<void> connect() = 0;
    /**
     * @brief Disconnect the live feed.
     */
    virtual void disconnect() = 0;
    /**
     * @brief Check connection status.
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief Subscribe to a list of symbols.
     * @param symbols Symbol strings.
     */
    virtual void subscribe(const std::vector<std::string>& symbols) = 0;
    /**
     * @brief Unsubscribe from a list of symbols.
     * @param symbols Symbol strings.
     */
    virtual void unsubscribe(const std::vector<std::string>& symbols) = 0;

    /**
     * @brief Register a bar callback.
     */
    virtual void on_bar(std::function<void(const Bar&)> cb) = 0;
    /**
     * @brief Register a tick callback.
     */
    virtual void on_tick(std::function<void(const Tick&)> cb) = 0;
    /**
     * @brief Register an order book callback.
     */
    virtual void on_book(std::function<void(const OrderBook&)> cb) = 0;

    /**
     * @brief Poll for new data (for polling feeds).
     */
    virtual void poll() = 0;
};

/**
 * @brief Polling adapter that fetches data from a REST source.
 */
class PollingRestFeed final : public LiveFeedAdapter {
public:
    /**
     * @brief Configuration for polling feed.
     */
    struct Config {
        /**
         * @brief Underlying data source.
         */
        std::shared_ptr<DataSource> source;
        /**
         * @brief Bar type to request.
         */
        BarType bar_type = BarType::Time_1Min;
    };

    /**
     * @brief Construct the polling feed.
     * @param config Feed configuration.
     */
    explicit PollingRestFeed(Config config);

    /**
     * @brief Connect (noop for polling feed).
     */
    Result<void> connect() override;
    /**
     * @brief Disconnect.
     */
    void disconnect() override;
    /**
     * @brief Check connection status.
     */
    bool is_connected() const override;

    /**
     * @brief Subscribe to symbols.
     */
    void subscribe(const std::vector<std::string>& symbols) override;
    /**
     * @brief Unsubscribe from symbols.
     */
    void unsubscribe(const std::vector<std::string>& symbols) override;

    /**
     * @brief Register bar callback.
     */
    void on_bar(std::function<void(const Bar&)> cb) override;
    /**
     * @brief Register tick callback.
     */
    void on_tick(std::function<void(const Tick&)> cb) override;
    /**
     * @brief Register order book callback.
     */
    void on_book(std::function<void(const OrderBook&)> cb) override;

    /**
     * @brief Poll data and emit callbacks.
     */
    void poll() override;

private:
    Config config_;
    bool connected_ = false;
    std::vector<SymbolId> subscribed_;
    std::unordered_map<SymbolId, Timestamp> last_bar_ts_;
    std::unordered_map<SymbolId, Timestamp> last_tick_ts_;

    std::function<void(const Bar&)> bar_cb_;
    std::function<void(const Tick&)> tick_cb_;
    std::function<void(const OrderBook&)> book_cb_;
};

}  // namespace regimeflow::data
