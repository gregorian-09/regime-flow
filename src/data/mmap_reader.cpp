#include "regimeflow/data/mmap_reader.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>

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

constexpr char kMagic[8] = {'R', 'G', 'M', 'F', 'L', 'O', 'W', '1'};
constexpr uint32_t kFileVersion = 1;

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

}  // namespace

MemoryMappedDataFile::MemoryMappedDataFile(const std::string& path) {
    try {
        map_file(path);
    } catch (...) {
        unmap_file();
        throw;
    }
}

MemoryMappedDataFile::~MemoryMappedDataFile() {
    unmap_file();
}

MemoryMappedDataFile::MemoryMappedDataFile(MemoryMappedDataFile&& other) noexcept {
    *this = std::move(other);
}

MemoryMappedDataFile& MemoryMappedDataFile::operator=(MemoryMappedDataFile&& other) noexcept {
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
    opens_ = other.opens_;
    highs_ = other.highs_;
    lows_ = other.lows_;
    closes_ = other.closes_;
    volumes_ = other.volumes_;
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
    other.opens_ = nullptr;
    other.highs_ = nullptr;
    other.lows_ = nullptr;
    other.closes_ = nullptr;
    other.volumes_ = nullptr;
    other.date_index_ = nullptr;
    other.symbol_id_ = 0;
    return *this;
}

const FileHeader& MemoryMappedDataFile::header() const {
    if (!header_) {
        throw std::runtime_error("MemoryMappedDataFile: header not available");
    }
    return *header_;
}

std::string MemoryMappedDataFile::symbol() const {
    return symbol_;
}

SymbolId MemoryMappedDataFile::symbol_id() const {
    return symbol_id_;
}

TimeRange MemoryMappedDataFile::time_range() const {
    TimeRange range;
    if (header_) {
        range.start = Timestamp(header_->start_timestamp);
        range.end = Timestamp(header_->end_timestamp);
    }
    return range;
}

size_t MemoryMappedDataFile::bar_count() const {
    return header_ ? static_cast<size_t>(header_->bar_count) : 0;
}

MemoryMappedDataFile::BarView::BarView(const MemoryMappedDataFile* file, size_t index)
    : file_(file), index_(index) {}

Timestamp MemoryMappedDataFile::BarView::timestamp() const {
    return Timestamp(file_->timestamps_[index_]);
}

double MemoryMappedDataFile::BarView::open() const {
    return file_->opens_[index_];
}

double MemoryMappedDataFile::BarView::high() const {
    return file_->highs_[index_];
}

double MemoryMappedDataFile::BarView::low() const {
    return file_->lows_[index_];
}

double MemoryMappedDataFile::BarView::close() const {
    return file_->closes_[index_];
}

uint64_t MemoryMappedDataFile::BarView::volume() const {
    return file_->volumes_[index_];
}

Bar MemoryMappedDataFile::BarView::to_bar() const {
    Bar bar;
    bar.timestamp = timestamp();
    bar.symbol = file_->symbol_id_;
    bar.open = open();
    bar.high = high();
    bar.low = low();
    bar.close = close();
    bar.volume = volume();
    return bar;
}

MemoryMappedDataFile::BarView MemoryMappedDataFile::operator[](size_t index) const {
    return BarView(this, index);
}

MemoryMappedDataFile::BarView MemoryMappedDataFile::at(size_t index) const {
    if (!header_ || index >= header_->bar_count) {
        throw std::out_of_range("MemoryMappedDataFile: index out of range");
    }
    return BarView(this, index);
}

MemoryMappedDataFile::Iterator::Iterator(const MemoryMappedDataFile* file, size_t index)
    : file_(file), index_(index) {}

MemoryMappedDataFile::BarView MemoryMappedDataFile::Iterator::operator*() const {
    return MemoryMappedDataFile::BarView(file_, index_);
}

MemoryMappedDataFile::Iterator& MemoryMappedDataFile::Iterator::operator++() {
    ++index_;
    return *this;
}

bool MemoryMappedDataFile::Iterator::operator!=(const Iterator& other) const {
    return index_ != other.index_ || file_ != other.file_;
}

MemoryMappedDataFile::Iterator MemoryMappedDataFile::begin() const {
    return Iterator(this, 0);
}

MemoryMappedDataFile::Iterator MemoryMappedDataFile::end() const {
    return Iterator(this, bar_count());
}

std::pair<size_t, size_t> MemoryMappedDataFile::find_range(TimeRange range) const {
    size_t count = bar_count();
    if (count == 0) {
        return {0, 0};
    }
    if (range.start.microseconds() == 0 && range.end.microseconds() == 0) {
        return {0, count};
    }
    int64_t start = range.start.microseconds();
    int64_t end = range.end.microseconds();
    auto span = timestamps();
    auto begin_it = std::lower_bound(span.begin(), span.end(), start);
    auto end_it = std::upper_bound(span.begin(), span.end(), end);
    return {static_cast<size_t>(begin_it - span.begin()),
            static_cast<size_t>(end_it - span.begin())};
}

std::span<const int64_t> MemoryMappedDataFile::timestamps() const {
    return {timestamps_, bar_count()};
}

std::span<const double> MemoryMappedDataFile::opens() const {
    return {opens_, bar_count()};
}

std::span<const double> MemoryMappedDataFile::highs() const {
    return {highs_, bar_count()};
}

std::span<const double> MemoryMappedDataFile::lows() const {
    return {lows_, bar_count()};
}

std::span<const double> MemoryMappedDataFile::closes() const {
    return {closes_, bar_count()};
}

std::span<const uint64_t> MemoryMappedDataFile::volumes() const {
    return {volumes_, bar_count()};
}

void MemoryMappedDataFile::map_file(const std::string& path) {
#if defined(_WIN32)
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("MemoryMappedDataFile: open failed");
    }
    LARGE_INTEGER size {};
    if (!GetFileSizeEx(file, &size)) {
        CloseHandle(file);
        throw std::runtime_error("MemoryMappedDataFile: stat failed");
    }
    if (size.QuadPart < static_cast<LONGLONG>(sizeof(FileHeader))) {
        CloseHandle(file);
        throw std::runtime_error("MemoryMappedDataFile: file too small");
    }
    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping) {
        CloseHandle(file);
        throw std::runtime_error("MemoryMappedDataFile: mmap failed");
    }
    void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        CloseHandle(mapping);
        CloseHandle(file);
        throw std::runtime_error("MemoryMappedDataFile: mmap failed");
    }
    file_handle_ = file;
    mapping_handle_ = mapping;
    mapping_ = view;
    file_size_ = static_cast<size_t>(size.QuadPart);
#else
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        throw std::runtime_error("MemoryMappedDataFile: open failed: " +
                                 std::string(std::strerror(errno)));
    }

    struct stat st {};
    if (::fstat(fd_, &st) != 0) {
        int err = errno;
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("MemoryMappedDataFile: stat failed: " +
                                 std::string(std::strerror(err)));
    }

    if (st.st_size < static_cast<off_t>(sizeof(FileHeader))) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("MemoryMappedDataFile: file too small");
    }

    file_size_ = static_cast<size_t>(st.st_size);
    mapping_ = ::mmap(nullptr, file_size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (mapping_ == MAP_FAILED) {
        mapping_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("MemoryMappedDataFile: mmap failed");
    }
#endif

    header_ = static_cast<const FileHeader*>(mapping_);
    if (std::memcmp(header_->magic, kMagic, sizeof(kMagic)) != 0) {
        throw std::runtime_error("MemoryMappedDataFile: invalid magic");
    }
    if (header_->version != kFileVersion) {
        throw std::runtime_error("MemoryMappedDataFile: unsupported version");
    }
    if (header_->data_offset < sizeof(FileHeader)) {
        throw std::runtime_error("MemoryMappedDataFile: invalid data offset");
    }
    if (header_->data_offset >= file_size_) {
        throw std::runtime_error("MemoryMappedDataFile: data offset out of range");
    }

    symbol_ = std::string(trim_nulls(header_->symbol, sizeof(header_->symbol)));
    if (!symbol_.empty()) {
        symbol_id_ = SymbolRegistry::instance().intern(symbol_);
    }

    setup_column_pointers();
}

void MemoryMappedDataFile::unmap_file() {
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
    opens_ = nullptr;
    highs_ = nullptr;
    lows_ = nullptr;
    closes_ = nullptr;
    volumes_ = nullptr;
    date_index_ = nullptr;
    index_count_ = 0;
    symbol_.clear();
    symbol_id_ = 0;
}

void MemoryMappedDataFile::setup_column_pointers() {
    size_t count = static_cast<size_t>(header_->bar_count);
    if (count != header_->bar_count) {
        throw std::runtime_error("MemoryMappedDataFile: bar count overflow");
    }

    size_t column_bytes = 0;
    size_t total_bytes = 0;
    size_t data_bytes = 0;

    if (!checked_mul(count, sizeof(int64_t), column_bytes)) {
        throw std::runtime_error("MemoryMappedDataFile: timestamps size overflow");
    }
    data_bytes = column_bytes;

    if (!checked_mul(count, sizeof(double), column_bytes)) {
        throw std::runtime_error("MemoryMappedDataFile: price size overflow");
    }

    if (!checked_add(data_bytes, column_bytes, data_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes)) {
        throw std::runtime_error("MemoryMappedDataFile: price section overflow");
    }

    if (!checked_mul(count, sizeof(uint64_t), column_bytes) ||
        !checked_add(data_bytes, column_bytes, total_bytes)) {
        throw std::runtime_error("MemoryMappedDataFile: volume size overflow");
    }

    size_t end_offset = 0;
    if (!checked_add(static_cast<size_t>(header_->data_offset), total_bytes, end_offset) ||
        end_offset > file_size_) {
        throw std::runtime_error("MemoryMappedDataFile: data section out of range");
    }

    const auto* base = static_cast<const std::byte*>(mapping_);
    const auto* data_ptr = base + header_->data_offset;

    timestamps_ = reinterpret_cast<const int64_t*>(data_ptr);
    opens_ = reinterpret_cast<const double*>(timestamps_ + count);
    highs_ = opens_ + count;
    lows_ = highs_ + count;
    closes_ = lows_ + count;
    volumes_ = reinterpret_cast<const uint64_t*>(closes_ + count);

    if (header_->index_offset > 0 && header_->index_offset < file_size_) {
        date_index_ = reinterpret_cast<const DateIndex*>(base + header_->index_offset);
        size_t index_bytes = file_size_ - static_cast<size_t>(header_->index_offset);
        index_count_ = index_bytes / sizeof(DateIndex);
    }
}

void MemoryMappedDataFile::preload_index() const {
    if (!date_index_ || index_count_ == 0) {
        return;
    }
    volatile int32_t sink = 0;
    size_t stride = 64;
    for (size_t i = 0; i < index_count_; i += stride) {
        sink ^= date_index_[i].date_yyyymmdd;
    }
    if (index_count_ > 0) {
        sink ^= date_index_[index_count_ - 1].date_yyyymmdd;
    }
    (void)sink;
}

}  // namespace regimeflow::data
