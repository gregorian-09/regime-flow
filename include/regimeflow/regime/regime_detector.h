#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/regime/types.h"
#include "regimeflow/regime/features.h"

namespace regimeflow::regime {

class RegimeDetector {
public:
    virtual ~RegimeDetector() = default;

    virtual RegimeState on_bar(const data::Bar& bar) = 0;
    virtual RegimeState on_tick(const data::Tick& tick) = 0;
    virtual RegimeState on_book(const data::OrderBook& book) {
        data::Bar bar{};
        bar.timestamp = book.timestamp;
        bar.symbol = book.symbol;
        double bid = book.bids[0].price;
        double ask = book.asks[0].price;
        double mid = (bid + ask) / 2.0;
        bar.open = mid;
        bar.high = mid;
        bar.low = mid;
        bar.close = mid;
        bar.volume = 0;
        return on_bar(bar);
    }
    virtual void train(const std::vector<FeatureVector>&) {}
    virtual void save(const std::string& path) const { (void)path; }
    virtual void load(const std::string& path) { (void)path; }
    virtual void configure(const Config& config) { (void)config; }
    virtual int num_states() const { return 0; }
    virtual std::vector<std::string> state_names() const { return {}; }
};

}  // namespace regimeflow::regime
