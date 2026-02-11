#include "regimeflow/data/merged_iterator.h"

#include <stdexcept>

namespace regimeflow::data {

MergedBarIterator::MergedBarIterator(std::vector<std::unique_ptr<DataIterator>> iterators)
    : iterators_(std::move(iterators)) {
    initialize_heap();
}

bool MergedBarIterator::has_next() const {
    return !heap_.empty();
}

Bar MergedBarIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more bars");
    }
    auto entry = heap_.top();
    heap_.pop();

    auto& iterator = iterators_[entry.iterator_index];
    if (iterator && iterator->has_next()) {
        heap_.push({iterator->next(), entry.iterator_index});
    }
    return entry.bar;
}

void MergedBarIterator::reset() {
    for (auto& iterator : iterators_) {
        if (iterator) {
            iterator->reset();
        }
    }
    while (!heap_.empty()) {
        heap_.pop();
    }
    initialize_heap();
}

void MergedBarIterator::initialize_heap() {
    for (size_t i = 0; i < iterators_.size(); ++i) {
        auto& iterator = iterators_[i];
        if (iterator && iterator->has_next()) {
            heap_.push({iterator->next(), i});
        }
    }
}

MergedTickIterator::MergedTickIterator(std::vector<std::unique_ptr<TickIterator>> iterators)
    : iterators_(std::move(iterators)) {
    initialize_heap();
}

bool MergedTickIterator::has_next() const {
    return !heap_.empty();
}

Tick MergedTickIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more ticks");
    }
    auto entry = heap_.top();
    heap_.pop();

    auto& iterator = iterators_[entry.iterator_index];
    if (iterator && iterator->has_next()) {
        heap_.push({iterator->next(), entry.iterator_index});
    }
    return entry.tick;
}

void MergedTickIterator::reset() {
    for (auto& iterator : iterators_) {
        if (iterator) {
            iterator->reset();
        }
    }
    while (!heap_.empty()) {
        heap_.pop();
    }
    initialize_heap();
}

void MergedTickIterator::initialize_heap() {
    for (size_t i = 0; i < iterators_.size(); ++i) {
        auto& iterator = iterators_[i];
        if (iterator && iterator->has_next()) {
            heap_.push({iterator->next(), i});
        }
    }
}

MergedOrderBookIterator::MergedOrderBookIterator(
    std::vector<std::unique_ptr<OrderBookIterator>> iterators)
    : iterators_(std::move(iterators)) {
    initialize_heap();
}

bool MergedOrderBookIterator::has_next() const {
    return !heap_.empty();
}

OrderBook MergedOrderBookIterator::next() {
    if (!has_next()) {
        throw std::out_of_range("No more order books");
    }
    auto entry = heap_.top();
    heap_.pop();

    auto& iterator = iterators_[entry.iterator_index];
    if (iterator && iterator->has_next()) {
        heap_.push({iterator->next(), entry.iterator_index});
    }
    return entry.book;
}

void MergedOrderBookIterator::reset() {
    for (auto& iterator : iterators_) {
        if (iterator) {
            iterator->reset();
        }
    }
    while (!heap_.empty()) {
        heap_.pop();
    }
    initialize_heap();
}

void MergedOrderBookIterator::initialize_heap() {
    for (size_t i = 0; i < iterators_.size(); ++i) {
        auto& iterator = iterators_[i];
        if (iterator && iterator->has_next()) {
            heap_.push({iterator->next(), i});
        }
    }
}

}  // namespace regimeflow::data
