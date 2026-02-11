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

struct BookDateIndex {
    int32_t date_yyyymmdd = 0;
    uint64_t offset = 0;
};

class OrderBookMmapFile {
public:
    explicit OrderBookMmapFile(const std::string& path);
    ~OrderBookMmapFile();

    OrderBookMmapFile(const OrderBookMmapFile&) = delete;
    OrderBookMmapFile& operator=(const OrderBookMmapFile&) = delete;

    OrderBookMmapFile(OrderBookMmapFile&& other) noexcept;
    OrderBookMmapFile& operator=(OrderBookMmapFile&& other) noexcept;

    const BookFileHeader& header() const;
    std::string symbol() const;
    SymbolId symbol_id() const;
    TimeRange time_range() const;
    size_t book_count() const;

    OrderBook at(size_t index) const;

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

class OrderBookMmapWriter {
public:
    Result<void> write_books(const std::string& path,
                             const std::string& symbol,
                             std::vector<OrderBook> books);

private:
    Result<void> validate_books(const std::vector<OrderBook>& books) const;
    static std::vector<BookDateIndex> build_date_index(const std::vector<OrderBook>& books);
};

}  // namespace regimeflow::data
