#include "regimeflow/data/order_book_mmap.h"

#include "regimeflow/common/sha256.h"
#include "regimeflow/common/types.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace regimeflow::data {

namespace {

constexpr char kMagic[8] = {'R', 'G', 'M', 'B', 'O', 'O', 'K', '1'};
constexpr uint32_t kFileVersion = 1;
constexpr uint32_t kLevels = 10;

bool checked_mul(size_t a, size_t b, size_t& out) {
    if (a == 0 || b == 0) {
        out = 0;
        return true;
    }
    if (a > std::numeric_limits<size_t>::max() / b) {
        return false;
    }
    out = a * b;
    return true;
}

bool checked_add(size_t a, size_t b, size_t& out) {
    if (a > std::numeric_limits<size_t>::max() - b) {
        return false;
    }
    out = a + b;
    return true;
}

std::string_view trim_nulls(const char* data, size_t size) {
    size_t len = 0;
    for (; len < size; ++len) {
        if (data[len] == '\0') {
            break;
        }
    }
    return std::string_view(data, len);
}

void write_bytes(std::ofstream& out, const void* data, size_t len, Sha256* sha) {
    out.write(static_cast<const char*>(data), static_cast<std::streamsize>(len));
    if (sha) {
        sha->update(data, len);
    }
}

int32_t yyyymmdd_from_timestamp(const Timestamp& ts) {
    auto text = ts.to_string("%Y%m%d");
    return static_cast<int32_t>(std::stoi(text));
}

}  // namespace

OrderBookMmapFile::OrderBookMmapFile(const std::string& path) {
    map_file(path);
}

OrderBookMmapFile::~OrderBookMmapFile() {
    unmap_file();
}

OrderBookMmapFile::OrderBookMmapFile(OrderBookMmapFile&& other) noexcept {
    *this = std::move(other);
}

OrderBookMmapFile& OrderBookMmapFile::operator=(OrderBookMmapFile&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    unmap_file();
    mapping_ = other.mapping_;
    file_size_ = other.file_size_;
#if defined(_WIN32)
    file_handle_ = other.file_handle_;
    mapping_handle_ = other.mapping_handle_;
#endif
    fd_ = other.fd_;
    header_ = other.header_;
    timestamps_ = other.timestamps_;
    bid_prices_ = other.bid_prices_;
    bid_qty_ = other.bid_qty_;
    bid_orders_ = other.bid_orders_;
    ask_prices_ = other.ask_prices_;
    ask_qty_ = other.ask_qty_;
    ask_orders_ = other.ask_orders_;
    date_index_ = other.date_index_;
    symbol_ = std::move(other.symbol_);
    symbol_id_ = other.symbol_id_;

    other.mapping_ = nullptr;
    other.file_size_ = 0;
#if defined(_WIN32)
    other.file_handle_ = nullptr;
    other.mapping_handle_ = nullptr;
#endif
    other.fd_ = -1;
    other.header_ = nullptr;
    other.timestamps_ = nullptr;
    other.bid_prices_ = nullptr;
    other.bid_qty_ = nullptr;
    other.bid_orders_ = nullptr;
    other.ask_prices_ = nullptr;
    other.ask_qty_ = nullptr;
    other.ask_orders_ = nullptr;
    other.date_index_ = nullptr;
    other.symbol_id_ = 0;
    return *this;
}

const BookFileHeader& OrderBookMmapFile::header() const {
    if (!header_) {
        throw std::runtime_error("OrderBookMmapFile: header not available");
    }
    return *header_;
}

std::string OrderBookMmapFile::symbol() const {
    return symbol_;
}

SymbolId OrderBookMmapFile::symbol_id() const {
    return symbol_id_;
}

TimeRange OrderBookMmapFile::time_range() const {
    TimeRange range;
    if (header_) {
        range.start = Timestamp(header_->start_timestamp);
        range.end = Timestamp(header_->end_timestamp);
    }
    return range;
}

size_t OrderBookMmapFile::book_count() const {
    return header_ ? static_cast<size_t>(header_->book_count) : 0;
}

OrderBook OrderBookMmapFile::at(size_t index) const {
    if (!header_ || index >= header_->book_count) {
        throw std::out_of_range("OrderBookMmapFile: index out of range");
    }
    OrderBook book;
    book.timestamp = Timestamp(timestamps_[index]);
    book.symbol = symbol_id_;
    for (size_t level = 0; level < kLevels; ++level) {
        size_t offset = level * header_->book_count + index;
        book.bids[level].price = bid_prices_[offset];
        book.bids[level].quantity = bid_qty_[offset];
        book.bids[level].num_orders = static_cast<int>(bid_orders_[offset]);
        book.asks[level].price = ask_prices_[offset];
        book.asks[level].quantity = ask_qty_[offset];
        book.asks[level].num_orders = static_cast<int>(ask_orders_[offset]);
    }
    return book;
}

std::pair<size_t, size_t> OrderBookMmapFile::find_range(TimeRange range) const {
    size_t count = book_count();
    if (count == 0) {
        return {0, 0};
    }
    if (range.start.microseconds() == 0 && range.end.microseconds() == 0) {
        return {0, count};
    }
    int64_t start = range.start.microseconds();
    int64_t end = range.end.microseconds();
    auto begin_it = std::lower_bound(timestamps_, timestamps_ + count, start);
    auto end_it = std::upper_bound(timestamps_, timestamps_ + count, end);
    return {static_cast<size_t>(begin_it - timestamps_),
            static_cast<size_t>(end_it - timestamps_)};
}

void OrderBookMmapFile::map_file(const std::string& path) {
#if defined(_WIN32)
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("OrderBookMmapFile: open failed");
    }
    LARGE_INTEGER size {};
    if (!GetFileSizeEx(file, &size)) {
        CloseHandle(file);
        throw std::runtime_error("OrderBookMmapFile: stat failed");
    }
    if (size.QuadPart < static_cast<LONGLONG>(sizeof(BookFileHeader))) {
        CloseHandle(file);
        throw std::runtime_error("OrderBookMmapFile: file too small");
    }
    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping) {
        CloseHandle(file);
        throw std::runtime_error("OrderBookMmapFile: mmap failed");
    }
    void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        CloseHandle(mapping);
        CloseHandle(file);
        throw std::runtime_error("OrderBookMmapFile: mmap failed");
    }
    file_handle_ = file;
    mapping_handle_ = mapping;
    mapping_ = view;
    file_size_ = static_cast<size_t>(size.QuadPart);
#else
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        throw std::runtime_error("OrderBookMmapFile: open failed");
    }
    struct stat st {};
    if (::fstat(fd_, &st) != 0) {
        int err = errno;
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("OrderBookMmapFile: stat failed: " + std::string(std::strerror(err)));
    }
    if (st.st_size < static_cast<off_t>(sizeof(BookFileHeader))) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("OrderBookMmapFile: file too small");
    }
    file_size_ = static_cast<size_t>(st.st_size);
    mapping_ = ::mmap(nullptr, file_size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (mapping_ == MAP_FAILED) {
        mapping_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("OrderBookMmapFile: mmap failed");
    }
#endif
    header_ = static_cast<const BookFileHeader*>(mapping_);
    if (std::memcmp(header_->magic, kMagic, sizeof(kMagic)) != 0) {
        throw std::runtime_error("OrderBookMmapFile: invalid magic");
    }
    if (header_->version != kFileVersion) {
        throw std::runtime_error("OrderBookMmapFile: unsupported version");
    }
    if (header_->level_count != kLevels) {
        throw std::runtime_error("OrderBookMmapFile: unexpected level count");
    }
    symbol_ = std::string(trim_nulls(header_->symbol, sizeof(header_->symbol)));
    if (!symbol_.empty()) {
        symbol_id_ = SymbolRegistry::instance().intern(symbol_);
    }
    setup_column_pointers();
}

void OrderBookMmapFile::unmap_file() {
    if (mapping_) {
#if defined(_WIN32)
        UnmapViewOfFile(mapping_);
#else
        ::munmap(mapping_, file_size_);
#endif
        mapping_ = nullptr;
    }
#if defined(_WIN32)
    if (mapping_handle_) {
        CloseHandle(static_cast<HANDLE>(mapping_handle_));
        mapping_handle_ = nullptr;
    }
    if (file_handle_) {
        CloseHandle(static_cast<HANDLE>(file_handle_));
        file_handle_ = nullptr;
    }
#else
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    file_size_ = 0;
    header_ = nullptr;
    timestamps_ = nullptr;
    bid_prices_ = nullptr;
    bid_qty_ = nullptr;
    bid_orders_ = nullptr;
    ask_prices_ = nullptr;
    ask_qty_ = nullptr;
    ask_orders_ = nullptr;
    date_index_ = nullptr;
    symbol_.clear();
    symbol_id_ = 0;
}

void OrderBookMmapFile::setup_column_pointers() {
    size_t count = static_cast<size_t>(header_->book_count);
    size_t data_bytes = 0;
    size_t column_bytes = 0;

    if (!checked_mul(count, sizeof(int64_t), column_bytes)) {
        throw std::runtime_error("OrderBookMmapFile: timestamps size overflow");
    }
    data_bytes = column_bytes;

    size_t price_cols = kLevels * 2;
    size_t qty_cols = kLevels * 2;
    size_t order_cols = kLevels * 2;

    if (!checked_mul(count * price_cols, sizeof(double), column_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes)) {
        throw std::runtime_error("OrderBookMmapFile: price size overflow");
    }
    if (!checked_mul(count * qty_cols, sizeof(double), column_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes)) {
        throw std::runtime_error("OrderBookMmapFile: qty size overflow");
    }
    if (!checked_mul(count * order_cols, sizeof(int64_t), column_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes)) {
        throw std::runtime_error("OrderBookMmapFile: orders size overflow");
    }

    size_t end_offset = 0;
    if (!checked_add(static_cast<size_t>(header_->data_offset), data_bytes, end_offset) ||
        end_offset > file_size_) {
        throw std::runtime_error("OrderBookMmapFile: data section out of range");
    }

    const auto* base = static_cast<const std::byte*>(mapping_);
    const auto* data_ptr = base + header_->data_offset;

    timestamps_ = reinterpret_cast<const int64_t*>(data_ptr);
    bid_prices_ = reinterpret_cast<const double*>(timestamps_ + count);
    bid_qty_ = bid_prices_ + (count * kLevels);
    bid_orders_ = reinterpret_cast<const int64_t*>(bid_qty_ + (count * kLevels));
    ask_prices_ = reinterpret_cast<const double*>(bid_orders_ + (count * kLevels));
    ask_qty_ = ask_prices_ + (count * kLevels);
    ask_orders_ = reinterpret_cast<const int64_t*>(ask_qty_ + (count * kLevels));

    if (header_->index_offset > 0 && header_->index_offset < file_size_) {
        date_index_ = reinterpret_cast<const BookDateIndex*>(base + header_->index_offset);
    }
}

Result<void> OrderBookMmapWriter::write_books(const std::string& path,
                                              const std::string& symbol,
                                              std::vector<OrderBook> books) {
    std::sort(books.begin(), books.end(), [](const OrderBook& a, const OrderBook& b) {
        return a.timestamp < b.timestamp;
    });
    if (auto validation = validate_books(books); validation.is_err()) {
        return validation;
    }

    const size_t count = books.size();
    std::vector<int64_t> timestamps(count);
    std::vector<double> bid_prices(count * kLevels);
    std::vector<double> bid_qty(count * kLevels);
    std::vector<int64_t> bid_orders(count * kLevels);
    std::vector<double> ask_prices(count * kLevels);
    std::vector<double> ask_qty(count * kLevels);
    std::vector<int64_t> ask_orders(count * kLevels);

    for (size_t i = 0; i < count; ++i) {
        timestamps[i] = books[i].timestamp.microseconds();
        for (size_t level = 0; level < kLevels; ++level) {
            size_t offset = level * count + i;
            bid_prices[offset] = books[i].bids[level].price;
            bid_qty[offset] = books[i].bids[level].quantity;
            bid_orders[offset] = books[i].bids[level].num_orders;
            ask_prices[offset] = books[i].asks[level].price;
            ask_qty[offset] = books[i].asks[level].quantity;
            ask_orders[offset] = books[i].asks[level].num_orders;
        }
    }

    std::vector<BookDateIndex> index = build_date_index(books);

    BookFileHeader header{};
    std::memcpy(header.magic, kMagic, sizeof(kMagic));
    header.version = kFileVersion;
    header.flags = 0;
    std::memset(header.symbol, 0, sizeof(header.symbol));
    #if defined(_WIN32)
    strncpy_s(header.symbol, sizeof(header.symbol), symbol.c_str(), _TRUNCATE);
    #else
    std::strncpy(header.symbol, symbol.c_str(), sizeof(header.symbol) - 1);
    #endif
    header.level_count = kLevels;
    if (!books.empty()) {
        header.start_timestamp = books.front().timestamp.microseconds();
        header.end_timestamp = books.back().timestamp.microseconds();
    }
    header.book_count = static_cast<uint64_t>(count);
    header.data_offset = sizeof(BookFileHeader);
    size_t data_bytes = count * (sizeof(int64_t) +
                                 (kLevels * 2) * sizeof(double) +
                                 (kLevels * 2) * sizeof(double) +
                                 (kLevels * 2) * sizeof(int64_t));
    header.index_offset = header.data_offset + static_cast<uint64_t>(data_bytes);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return Error(Error::Code::IoError, "Unable to open order book mmap output file");
    }

    write_bytes(out, &header, sizeof(header), nullptr);

    Sha256 sha;
    write_bytes(out, timestamps.data(), timestamps.size() * sizeof(int64_t), &sha);
    write_bytes(out, bid_prices.data(), bid_prices.size() * sizeof(double), &sha);
    write_bytes(out, bid_qty.data(), bid_qty.size() * sizeof(double), &sha);
    write_bytes(out, bid_orders.data(), bid_orders.size() * sizeof(int64_t), &sha);
    write_bytes(out, ask_prices.data(), ask_prices.size() * sizeof(double), &sha);
    write_bytes(out, ask_qty.data(), ask_qty.size() * sizeof(double), &sha);
    write_bytes(out, ask_orders.data(), ask_orders.size() * sizeof(int64_t), &sha);

    if (!index.empty()) {
        write_bytes(out, index.data(), index.size() * sizeof(BookDateIndex), nullptr);
    }

    auto checksum = sha.digest();
    std::memcpy(header.checksum, checksum.data(), checksum.size());
    out.seekp(0);
    write_bytes(out, &header, sizeof(header), nullptr);
    return Ok();
}

Result<void> OrderBookMmapWriter::validate_books(const std::vector<OrderBook>& books) const {
    if (books.empty()) {
        return Ok();
    }
    Timestamp last = books.front().timestamp;
    for (size_t i = 0; i < books.size(); ++i) {
        const auto& book = books[i];
        if (book.timestamp.microseconds() <= 0) {
            return Error(Error::Code::InvalidArgument, "Book timestamp must be positive");
        }
        if (i > 0 && book.timestamp < last) {
            return Error(Error::Code::InvalidArgument, "Books must be sorted by timestamp");
        }
        last = book.timestamp;
    }
    return Ok();
}

std::vector<BookDateIndex> OrderBookMmapWriter::build_date_index(
    const std::vector<OrderBook>& books) {
    std::vector<BookDateIndex> index;
    int32_t last_date = 0;
    for (size_t i = 0; i < books.size(); ++i) {
        int32_t date = yyyymmdd_from_timestamp(books[i].timestamp);
        if (i == 0 || date != last_date) {
            index.push_back(BookDateIndex{date, static_cast<uint64_t>(i)});
            last_date = date;
        }
    }
    return index;
}

}  // namespace regimeflow::data
