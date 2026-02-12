/**
 * @file lru_cache.h
 * @brief RegimeFlow regimeflow lru cache declarations.
 */

#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

namespace regimeflow {

/**
 * @brief Least-recently-used cache with fixed capacity.
 * @tparam Key Key type (must be hashable).
 * @tparam Value Value type.
 */
template <typename Key, typename Value>
class LRUCache {
public:
    /**
     * @brief Construct an LRU cache with a capacity.
     * @param capacity Max number of items to retain.
     */
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Update the cache capacity.
     * @param capacity New capacity.
     */
    void set_capacity(size_t capacity) {
        capacity_ = capacity;
        evict_if_needed();
    }

    /**
     * @brief Current capacity.
     * @return Maximum number of items.
     */
    size_t capacity() const { return capacity_; }
    /**
     * @brief Current number of items.
     * @return Size.
     */
    size_t size() const { return items_.size(); }

    /**
     * @brief Fetch a value and mark it as most-recently-used.
     * @param key Key to lookup.
     * @return Optional value, empty if missing.
     */
    std::optional<Value> get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    /**
     * @brief Insert or update a value, evicting if needed.
     * @param key Key to insert.
     * @param value Value to store.
     */
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

    /**
     * @brief Clear all entries from the cache.
     */
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
