/**
 * @file memory_data_source.h
 * @brief RegimeFlow regimeflow memory data source declarations.
 */

#pragma once

#include "regimeflow/data/data_source.h"
#include "regimeflow/data/corporate_actions.h"

#include <algorithm>
#include <map>
#include <utility>

namespace regimeflow::data {

/**
 * @brief Bar iterator over an in-memory vector.
 */
class VectorBarIterator final : public DataIterator {
public:
    /**
     * @brief Construct from a vector of bars.
     * @param bars Bar list.
     */
    explicit VectorBarIterator(std::vector<Bar> bars);

    /**
     * @brief True if more bars exist.
     */
    bool has_next() const override;
    /**
     * @brief Get next bar.
     */
    Bar next() override;
    /**
     * @brief Reset iterator to beginning.
     */
    void reset() override;

private:
    std::vector<Bar> bars_;
    size_t index_ = 0;
};

/**
 * @brief Order book iterator over an in-memory vector.
 */
class VectorOrderBookIterator final : public OrderBookIterator {
public:
    /**
     * @brief Construct from a vector of order books.
     * @param books Order book snapshots.
     */
    explicit VectorOrderBookIterator(std::vector<OrderBook> books);

    /**
     * @brief True if more order books exist.
     */
    bool has_next() const override;
    /**
     * @brief Get next order book.
     */
    OrderBook next() override;
    /**
     * @brief Reset iterator to beginning.
     */
    void reset() override;

private:
    std::vector<OrderBook> books_;
    size_t index_ = 0;
};

/**
 * @brief Tick iterator over an in-memory vector.
 */
class VectorTickIterator final : public TickIterator {
public:
    /**
     * @brief Construct from a vector of ticks.
     * @param ticks Tick list.
     */
    explicit VectorTickIterator(std::vector<Tick> ticks);

    /**
     * @brief True if more ticks exist.
     */
    bool has_next() const override;
    /**
     * @brief Get next tick.
     */
    Tick next() override;
    /**
     * @brief Reset iterator to beginning.
     */
    void reset() override;

private:
    std::vector<Tick> ticks_;
    size_t index_ = 0;
};

/**
 * @brief In-memory data source for tests or ad-hoc data.
 */
class MemoryDataSource final : public DataSource {
public:
    /**
     * @brief Add bars for a symbol.
     */
    void add_bars(SymbolId symbol, std::vector<Bar> bars);
    /**
     * @brief Add ticks for a symbol.
     */
    void add_ticks(SymbolId symbol, std::vector<Tick> ticks);
    /**
     * @brief Add order books for a symbol.
     */
    void add_order_books(SymbolId symbol, std::vector<OrderBook> books);
    /**
     * @brief Add symbol metadata.
     */
    void add_symbol_info(SymbolInfo info);
    /**
     * @brief Add corporate actions.
     */
    void set_corporate_actions(SymbolId symbol, std::vector<CorporateAction> actions);

    /**
     * @brief List available symbols.
     */
    std::vector<SymbolInfo> get_available_symbols() const override;
    /**
     * @brief Get available range for a symbol.
     */
    TimeRange get_available_range(SymbolId symbol) const override;

    /**
     * @brief Get bars for a symbol and range.
     */
    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    /**
     * @brief Get ticks for a symbol and range.
     */
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;
    /**
     * @brief Get order books for a symbol and range.
     */
    std::vector<OrderBook> get_order_books(SymbolId symbol, TimeRange range) override;

    /**
     * @brief Create a bar iterator for multiple symbols.
     */
    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;
    /**
     * @brief Create a tick iterator.
     */
    std::unique_ptr<TickIterator> create_tick_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;
    /**
     * @brief Create an order book iterator.
     */
    std::unique_ptr<OrderBookIterator> create_book_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range) override;

    /**
     * @brief Get corporate actions for a symbol and range.
     */
    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;

private:
    static bool in_range(const Timestamp& ts, TimeRange range);

    std::map<SymbolId, std::vector<Bar>> bars_;
    std::map<SymbolId, std::vector<Tick>> ticks_;
    std::map<SymbolId, std::vector<OrderBook>> books_;
    std::map<SymbolId, SymbolInfo> symbols_;
    CorporateActionAdjuster adjuster_;
};

}  // namespace regimeflow::data
