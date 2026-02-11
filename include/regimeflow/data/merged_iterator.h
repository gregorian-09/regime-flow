#pragma once

#include "regimeflow/data/data_source.h"

#include <memory>
#include <queue>
#include <vector>

namespace regimeflow::data {

class MergedBarIterator final : public DataIterator {
public:
    explicit MergedBarIterator(std::vector<std::unique_ptr<DataIterator>> iterators);

    bool has_next() const override;
    Bar next() override;
    void reset() override;

private:
    struct HeapEntry {
        Bar bar;
        size_t iterator_index = 0;
    };

    struct HeapCompare {
        bool operator()(const HeapEntry& lhs, const HeapEntry& rhs) const {
            if (lhs.bar.timestamp != rhs.bar.timestamp) {
                return lhs.bar.timestamp > rhs.bar.timestamp;
            }
            return lhs.bar.symbol > rhs.bar.symbol;
        }
    };

    void initialize_heap();

    std::vector<std::unique_ptr<DataIterator>> iterators_;
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, HeapCompare> heap_;
};

class MergedTickIterator final : public TickIterator {
public:
    explicit MergedTickIterator(std::vector<std::unique_ptr<TickIterator>> iterators);

    bool has_next() const override;
    Tick next() override;
    void reset() override;

private:
    struct HeapEntry {
        Tick tick;
        size_t iterator_index = 0;
    };

    struct HeapCompare {
        bool operator()(const HeapEntry& lhs, const HeapEntry& rhs) const {
            if (lhs.tick.timestamp != rhs.tick.timestamp) {
                return lhs.tick.timestamp > rhs.tick.timestamp;
            }
            return lhs.tick.symbol > rhs.tick.symbol;
        }
    };

    void initialize_heap();

    std::vector<std::unique_ptr<TickIterator>> iterators_;
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, HeapCompare> heap_;
};

class MergedOrderBookIterator final : public OrderBookIterator {
public:
    explicit MergedOrderBookIterator(std::vector<std::unique_ptr<OrderBookIterator>> iterators);

    bool has_next() const override;
    OrderBook next() override;
    void reset() override;

private:
    struct HeapEntry {
        OrderBook book;
        size_t iterator_index = 0;
    };

    struct HeapCompare {
        bool operator()(const HeapEntry& lhs, const HeapEntry& rhs) const {
            if (lhs.book.timestamp != rhs.book.timestamp) {
                return lhs.book.timestamp > rhs.book.timestamp;
            }
            return lhs.book.symbol > rhs.book.symbol;
        }
    };

    void initialize_heap();

    std::vector<std::unique_ptr<OrderBookIterator>> iterators_;
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, HeapCompare> heap_;
};

}  // namespace regimeflow::data
