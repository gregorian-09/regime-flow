/**
 * @file merged_iterator.h
 * @brief RegimeFlow regimeflow merged iterator declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"

#include <memory>
#include <queue>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Merge multiple bar iterators into a time-ordered stream.
 */
class MergedBarIterator final : public DataIterator {
public:
    /**
     * @brief Construct from a list of iterators.
     * @param iterators Bar iterators.
     */
    explicit MergedBarIterator(std::vector<std::unique_ptr<DataIterator>> iterators);

    /**
     * @brief True if more bars exist.
     */
    bool has_next() const override;
    /**
     * @brief Get the next merged bar.
     */
    Bar next() override;
    /**
     * @brief Reset all iterators and heap.
     */
    void reset() override;

private:
    /**
     * @brief Heap entry for bar merge.
     */
    struct HeapEntry {
        Bar bar;
        size_t iterator_index = 0;
    };

    /**
     * @brief Heap comparator for bars.
     */
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

/**
 * @brief Merge multiple tick iterators into a time-ordered stream.
 */
class MergedTickIterator final : public TickIterator {
public:
    /**
     * @brief Construct from a list of tick iterators.
     * @param iterators Tick iterators.
     */
    explicit MergedTickIterator(std::vector<std::unique_ptr<TickIterator>> iterators);

    /**
     * @brief True if more ticks exist.
     */
    bool has_next() const override;
    /**
     * @brief Get the next merged tick.
     */
    Tick next() override;
    /**
     * @brief Reset all iterators and heap.
     */
    void reset() override;

private:
    /**
     * @brief Heap entry for tick merge.
     */
    struct HeapEntry {
        Tick tick;
        size_t iterator_index = 0;
    };

    /**
     * @brief Heap comparator for ticks.
     */
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

/**
 * @brief Merge multiple order book iterators into a time-ordered stream.
 */
class MergedOrderBookIterator final : public OrderBookIterator {
public:
    /**
     * @brief Construct from a list of order book iterators.
     * @param iterators Order book iterators.
     */
    explicit MergedOrderBookIterator(std::vector<std::unique_ptr<OrderBookIterator>> iterators);

    /**
     * @brief True if more order books exist.
     */
    bool has_next() const override;
    /**
     * @brief Get the next merged order book.
     */
    OrderBook next() override;
    /**
     * @brief Reset all iterators and heap.
     */
    void reset() override;

private:
    /**
     * @brief Heap entry for order book merge.
     */
    struct HeapEntry {
        OrderBook book;
        size_t iterator_index = 0;
    };

    /**
     * @brief Heap comparator for order books.
     */
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
