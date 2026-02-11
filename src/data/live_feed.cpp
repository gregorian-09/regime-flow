#include "regimeflow/data/live_feed.h"

#include "regimeflow/common/types.h"

#include <algorithm>

namespace regimeflow::data {

PollingRestFeed::PollingRestFeed(Config config) : config_(std::move(config)) {}

Result<void> PollingRestFeed::connect() {
    if (!config_.source) {
        return Error(Error::Code::InvalidState, "Live feed source not configured");
    }
    connected_ = true;
    return Ok();
}

void PollingRestFeed::disconnect() {
    connected_ = false;
}

bool PollingRestFeed::is_connected() const {
    return connected_;
}

void PollingRestFeed::subscribe(const std::vector<std::string>& symbols) {
    subscribed_.clear();
    subscribed_.reserve(symbols.size());
    for (const auto& symbol : symbols) {
        subscribed_.push_back(SymbolRegistry::instance().intern(symbol));
    }
}

void PollingRestFeed::unsubscribe(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        auto id = SymbolRegistry::instance().intern(symbol);
        subscribed_.erase(std::remove(subscribed_.begin(), subscribed_.end(), id),
                          subscribed_.end());
        last_bar_ts_.erase(id);
        last_tick_ts_.erase(id);
    }
}

void PollingRestFeed::on_bar(std::function<void(const Bar&)> cb) {
    bar_cb_ = std::move(cb);
}

void PollingRestFeed::on_tick(std::function<void(const Tick&)> cb) {
    tick_cb_ = std::move(cb);
}

void PollingRestFeed::on_book(std::function<void(const OrderBook&)> cb) {
    book_cb_ = std::move(cb);
}

void PollingRestFeed::poll() {
    if (!connected_ || !config_.source) {
        return;
    }

    for (auto symbol : subscribed_) {
        TimeRange range;
        if (auto it = last_bar_ts_.find(symbol); it != last_bar_ts_.end()) {
            range.start = it->second + Duration::microseconds(1);
        }
        range.end = Timestamp::now();
        auto bars = config_.source->get_bars(symbol, range, config_.bar_type);
        for (const auto& bar : bars) {
            last_bar_ts_[symbol] = bar.timestamp;
            if (bar_cb_) {
                bar_cb_(bar);
            }
        }

        auto ticks = config_.source->get_ticks(symbol, range);
        for (const auto& tick : ticks) {
            last_tick_ts_[symbol] = tick.timestamp;
            if (tick_cb_) {
                tick_cb_(tick);
            }
        }
    }

    (void)book_cb_;
}

}  // namespace regimeflow::data
