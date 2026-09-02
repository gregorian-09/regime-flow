/**
 * @file order_book.h
 * @brief RegimeFlow regimeflow order book declarations.
 */

#pragma once

#include "regimeflow/common/types.h"

#include <array>
#include <optional>

namespace regimeflow::data
{
    /**
     * @brief Single level in the order book.
     */
    struct BookLevel {
        Price price = 0;
        Quantity quantity = 0;
        int num_orders = 0;
    };

    /**
     * @brief Snapshot of top-of-book depth.
     */
    struct OrderBook {
        Timestamp timestamp;
        SymbolId symbol = 0;
        std::array<BookLevel, 10> bids{};
        std::array<BookLevel, 10> asks{};

        /**
         * @brief Return the usable best bid, if this snapshot has one.
         */
        [[nodiscard]] std::optional<BookLevel> best_bid() const noexcept {
            const auto& level = bids.front();
            if (level.price <= 0.0 || level.quantity <= 0.0) {
                return std::nullopt;
            }
            return level;
        }

        /**
         * @brief Return the usable best ask, if this snapshot has one.
         */
        [[nodiscard]] std::optional<BookLevel> best_ask() const noexcept {
            const auto& level = asks.front();
            if (level.price <= 0.0 || level.quantity <= 0.0) {
                return std::nullopt;
            }
            return level;
        }

        /**
         * @brief True when the book has a non-crossed, usable top of book.
         */
        [[nodiscard]] bool has_usable_top_of_book() const noexcept {
            const auto bid = best_bid();
            const auto ask = best_ask();
            return bid.has_value() && ask.has_value() && bid->price < ask->price;
        }
    };
}  // namespace regimeflow::data
