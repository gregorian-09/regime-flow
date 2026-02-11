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

class LiveFeedAdapter {
public:
    virtual ~LiveFeedAdapter() = default;

    virtual Result<void> connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual void subscribe(const std::vector<std::string>& symbols) = 0;
    virtual void unsubscribe(const std::vector<std::string>& symbols) = 0;

    virtual void on_bar(std::function<void(const Bar&)> cb) = 0;
    virtual void on_tick(std::function<void(const Tick&)> cb) = 0;
    virtual void on_book(std::function<void(const OrderBook&)> cb) = 0;

    virtual void poll() = 0;
};

class PollingRestFeed final : public LiveFeedAdapter {
public:
    struct Config {
        std::shared_ptr<DataSource> source;
        BarType bar_type = BarType::Time_1Min;
    };

    explicit PollingRestFeed(Config config);

    Result<void> connect() override;
    void disconnect() override;
    bool is_connected() const override;

    void subscribe(const std::vector<std::string>& symbols) override;
    void unsubscribe(const std::vector<std::string>& symbols) override;

    void on_bar(std::function<void(const Bar&)> cb) override;
    void on_tick(std::function<void(const Tick&)> cb) override;
    void on_book(std::function<void(const OrderBook&)> cb) override;

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
