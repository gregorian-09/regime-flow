#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace regimeflow {

template <typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    void set_capacity(size_t capacity) {
        capacity_ = capacity;
        evict_if_needed();
    }

    size_t capacity() const { return capacity_; }
    size_t size() const { return items_.size(); }

    std::optional<Value> get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(const Key& key, Value value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = std::move(value);
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        items_.emplace_front(key, std::move(value));
        map_[items_.front().first] = items_.begin();
        evict_if_needed();
    }

    void clear() {
        items_.clear();
        map_.clear();
    }

private:
    using ListIt = typename std::list<std::pair<Key, Value>>::iterator;

    void evict_if_needed() {
        if (capacity_ == 0) {
            clear();
            return;
        }
        while (items_.size() > capacity_) {
            auto last = items_.end();
            --last;
            map_.erase(last->first);
            items_.pop_back();
        }
    }

    size_t capacity_ = 0;
    std::list<std::pair<Key, Value>> items_;
    std::unordered_map<Key, ListIt> map_;
};

}  // namespace regimeflow
