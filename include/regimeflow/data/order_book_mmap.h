/**
 * @file order_book_mmap.h
 * @brief RegimeFlow regimeflow order book mmap declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/common/time.h"
#include "regimeflow/common/types.h"
#include "regimeflow/data/order_book.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace regimeflow::data {

#pragma pack(push, 1)
/**
 * @brief Header for memory-mapped order book files.
 */
struct BookFileHeader {
    char magic[8];
    uint32_t version;
    uint32_t flags;
    char symbol[32];
    uint32_t level_count;
    int64_t start_timestamp;
    int64_t end_timestamp;
    uint64_t book_count;
    uint64_t data_offset;
    uint64_t index_offset;
    unsigned char checksum[32];
    unsigned char reserved[132];
};
#pragma pack(pop)

static_assert(sizeof(BookFileHeader) == 256, "BookFileHeader must be 256 bytes");

/**
 * @brief Date index entry for order book files.
 */
struct BookDateIndex {
    int32_t date_yyyymmdd = 0;
    uint64_t offset = 0;
};

/**
 * @brief Memory-mapped access to order book snapshots.
 */
class OrderBookMmapFile {
public:
    /**
     * @brief Map a book file into memory.
     * @param path File path.
     */
    explicit OrderBookMmapFile(const std::string& path);
    /**
     * @brief Unmap and close the file.
     */
    ~OrderBookMmapFile();

    OrderBookMmapFile(const OrderBookMmapFile&) = delete;
    OrderBookMmapFile& operator=(const OrderBookMmapFile&) = delete;

    OrderBookMmapFile(OrderBookMmapFile&& other) noexcept;
    OrderBookMmapFile& operator=(OrderBookMmapFile&& other) noexcept;

    /**
     * @brief File header.
     */
    const BookFileHeader& header() const;
    /**
     * @brief Symbol string from header.
     */
    std::string symbol() const;
    /**
     * @brief Symbol ID derived from registry.
     */
    SymbolId symbol_id() const;
    /**
     * @brief Time range covered by this file.
     */
    TimeRange time_range() const;
    /**
     * @brief Number of order book snapshots.
     */
    size_t book_count() const;

    /**
     * @brief Read an order book at an index.
     * @param index Snapshot index.
     * @return Order book snapshot.
     */
    OrderBook at(size_t index) const;

    /**
     * @brief Find a [start,end) index range for a time range.
     * @param range Requested time range.
     * @return Pair of indices.
     */
    std::pair<size_t, size_t> find_range(TimeRange range) const;

private:
    void map_file(const std::string& path);
    void unmap_file();
    void setup_column_pointers();

    void* mapping_ = nullptr;
    size_t file_size_ = 0;
#if defined(_WIN32)
    void* file_handle_ = nullptr;
    void* mapping_handle_ = nullptr;
#endif
    int fd_ = -1;

    const BookFileHeader* header_ = nullptr;
    const int64_t* timestamps_ = nullptr;
    const double* bid_prices_ = nullptr;
    const double* bid_qty_ = nullptr;
    const int64_t* bid_orders_ = nullptr;
    const double* ask_prices_ = nullptr;
    const double* ask_qty_ = nullptr;
    const int64_t* ask_orders_ = nullptr;
    const BookDateIndex* date_index_ = nullptr;

    std::string symbol_;
    SymbolId symbol_id_ = 0;
};

/**
 * @brief Writer for memory-mapped order book files.
 */
class OrderBookMmapWriter {
public:
    /**
     * @brief Write order book snapshots to a mmap file.
     * @param path Output path.
     * @param symbol Symbol string.
     * @param books Order book snapshots.
     * @return Ok on success, error otherwise.
     */
    Result<void> write_books(const std::string& path,
                             const std::string& symbol,
                             std::vector<OrderBook> books);

private:
    Result<void> validate_books(const std::vector<OrderBook>& books) const;
    static std::vector<BookDateIndex> build_date_index(const std::vector<OrderBook>& books);
};

}  // namespace regimeflow::data
