#include "regimeflow/data/tick_mmap.h"

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

constexpr char kMagic[8] = {'R', 'G', 'M', 'T', 'I', 'C', 'K', '1'};
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

TickMmapFile::TickMmapFile(const std::string& path) {
    map_file(path);
}

TickMmapFile::~TickMmapFile() {
    unmap_file();
}

TickMmapFile::TickMmapFile(TickMmapFile&& other) noexcept {
    *this = std::move(other);
}

TickMmapFile& TickMmapFile::operator=(TickMmapFile&& other) noexcept {
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
    prices_ = other.prices_;
    quantities_ = other.quantities_;
    flags_ = other.flags_;
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
    other.prices_ = nullptr;
    other.quantities_ = nullptr;
    other.flags_ = nullptr;
    other.date_index_ = nullptr;
    other.symbol_id_ = 0;
    return *this;
}

const TickFileHeader& TickMmapFile::header() const {
    if (!header_) {
        throw std::runtime_error("TickMmapFile: header not available");
    }
    return *header_;
}

std::string TickMmapFile::symbol() const {
    return symbol_;
}

SymbolId TickMmapFile::symbol_id() const {
    return symbol_id_;
}

TimeRange TickMmapFile::time_range() const {
    TimeRange range;
    if (header_) {
        range.start = Timestamp(header_->start_timestamp);
        range.end = Timestamp(header_->end_timestamp);
    }
    return range;
}

size_t TickMmapFile::tick_count() const {
    return header_ ? static_cast<size_t>(header_->tick_count) : 0;
}

TickMmapFile::TickView::TickView(const TickMmapFile* file, size_t index)
    : file_(file), index_(index) {}

Timestamp TickMmapFile::TickView::timestamp() const {
    return Timestamp(file_->timestamps_[index_]);
}

double TickMmapFile::TickView::price() const {
    return file_->prices_[index_];
}

double TickMmapFile::TickView::quantity() const {
    return file_->quantities_[index_];
}

uint8_t TickMmapFile::TickView::flags() const {
    return static_cast<uint8_t>(file_->flags_[index_]);
}

Tick TickMmapFile::TickView::to_tick() const {
    Tick tick;
    tick.timestamp = timestamp();
    tick.symbol = file_->symbol_id_;
    tick.price = price();
    tick.quantity = quantity();
    tick.flags = flags();
    return tick;
}

TickMmapFile::TickView TickMmapFile::operator[](size_t index) const {
    return TickView(this, index);
}

TickMmapFile::TickView TickMmapFile::at(size_t index) const {
    if (!header_ || index >= header_->tick_count) {
        throw std::out_of_range("TickMmapFile: index out of range");
    }
    return TickView(this, index);
}

std::pair<size_t, size_t> TickMmapFile::find_range(TimeRange range) const {
    size_t count = tick_count();
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

std::span<const int64_t> TickMmapFile::timestamps() const {
    return {timestamps_, tick_count()};
}

std::span<const double> TickMmapFile::prices() const {
    return {prices_, tick_count()};
}

std::span<const double> TickMmapFile::quantities() const {
    return {quantities_, tick_count()};
}

std::span<const uint32_t> TickMmapFile::flags() const {
    return {flags_, tick_count()};
}

void TickMmapFile::map_file(const std::string& path) {
#if defined(_WIN32)
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("TickMmapFile: open failed");
    }
    LARGE_INTEGER size {};
    if (!GetFileSizeEx(file, &size)) {
        CloseHandle(file);
        throw std::runtime_error("TickMmapFile: stat failed");
    }
    if (size.QuadPart < static_cast<LONGLONG>(sizeof(TickFileHeader))) {
        CloseHandle(file);
        throw std::runtime_error("TickMmapFile: file too small");
    }
    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping) {
        CloseHandle(file);
        throw std::runtime_error("TickMmapFile: mmap failed");
    }
    void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        CloseHandle(mapping);
        CloseHandle(file);
        throw std::runtime_error("TickMmapFile: mmap failed");
    }
    file_handle_ = file;
    mapping_handle_ = mapping;
    mapping_ = view;
    file_size_ = static_cast<size_t>(size.QuadPart);
#else
    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        throw std::runtime_error("TickMmapFile: open failed");
    }
    struct stat st {};
    if (::fstat(fd_, &st) != 0) {
        int err = errno;
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("TickMmapFile: stat failed: " + std::string(std::strerror(err)));
    }
    if (st.st_size < static_cast<off_t>(sizeof(TickFileHeader))) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("TickMmapFile: file too small");
    }
    file_size_ = static_cast<size_t>(st.st_size);
    mapping_ = ::mmap(nullptr, file_size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (mapping_ == MAP_FAILED) {
        mapping_ = nullptr;
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error("TickMmapFile: mmap failed");
    }
#endif
    header_ = static_cast<const TickFileHeader*>(mapping_);
    if (std::memcmp(header_->magic, kMagic, sizeof(kMagic)) != 0) {
        throw std::runtime_error("TickMmapFile: invalid magic");
    }
    if (header_->version != kFileVersion) {
        throw std::runtime_error("TickMmapFile: unsupported version");
    }
    symbol_ = std::string(trim_nulls(header_->symbol, sizeof(header_->symbol)));
    if (!symbol_.empty()) {
        symbol_id_ = SymbolRegistry::instance().intern(symbol_);
    }
    setup_column_pointers();
}

void TickMmapFile::unmap_file() {
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
    prices_ = nullptr;
    quantities_ = nullptr;
    flags_ = nullptr;
    date_index_ = nullptr;
    symbol_.clear();
    symbol_id_ = 0;
}

void TickMmapFile::setup_column_pointers() {
    size_t count = static_cast<size_t>(header_->tick_count);
    size_t data_bytes = 0;
    size_t column_bytes = 0;

    if (!checked_mul(count, sizeof(int64_t), column_bytes)) {
        throw std::runtime_error("TickMmapFile: timestamps size overflow");
    }
    data_bytes = column_bytes;
    if (!checked_mul(count, sizeof(double), column_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes)) {
        throw std::runtime_error("TickMmapFile: data size overflow");
    }
    if (!checked_mul(count, sizeof(uint32_t), column_bytes) ||
        !checked_add(data_bytes, column_bytes, data_bytes)) {
        throw std::runtime_error("TickMmapFile: flags size overflow");
    }

    size_t end_offset = 0;
    if (!checked_add(static_cast<size_t>(header_->data_offset), data_bytes, end_offset) ||
        end_offset > file_size_) {
        throw std::runtime_error("TickMmapFile: data section out of range");
    }

    const auto* base = static_cast<const std::byte*>(mapping_);
    const auto* data_ptr = base + header_->data_offset;
    timestamps_ = reinterpret_cast<const int64_t*>(data_ptr);
    prices_ = reinterpret_cast<const double*>(timestamps_ + count);
    quantities_ = prices_ + count;
    flags_ = reinterpret_cast<const uint32_t*>(quantities_ + count);

    if (header_->index_offset > 0 && header_->index_offset < file_size_) {
        date_index_ = reinterpret_cast<const TickDateIndex*>(base + header_->index_offset);
    }
}

Result<void> TickMmapWriter::write_ticks(const std::string& path,
                                         const std::string& symbol,
                                         std::vector<Tick> ticks) {
    std::sort(ticks.begin(), ticks.end(), [](const Tick& a, const Tick& b) {
        return a.timestamp < b.timestamp;
    });
    if (auto validation = validate_ticks(ticks); validation.is_err()) {
        return validation;
    }

    const size_t count = ticks.size();
    std::vector<int64_t> timestamps(count);
    std::vector<double> prices(count);
    std::vector<double> quantities(count);
    std::vector<uint32_t> flags(count);

    for (size_t i = 0; i < count; ++i) {
        timestamps[i] = ticks[i].timestamp.microseconds();
        prices[i] = ticks[i].price;
        quantities[i] = ticks[i].quantity;
        flags[i] = ticks[i].flags;
    }

    std::vector<TickDateIndex> index = build_date_index(ticks);

    TickFileHeader header{};
    std::memcpy(header.magic, kMagic, sizeof(kMagic));
    header.version = kFileVersion;
    header.flags = 0;
    std::memset(header.symbol, 0, sizeof(header.symbol));
    std::strncpy(header.symbol, symbol.c_str(), sizeof(header.symbol) - 1);
    if (!ticks.empty()) {
        header.start_timestamp = ticks.front().timestamp.microseconds();
        header.end_timestamp = ticks.back().timestamp.microseconds();
    }
    header.tick_count = static_cast<uint64_t>(count);
    header.data_offset = sizeof(TickFileHeader);
    size_t data_bytes = count * (sizeof(int64_t) + 2 * sizeof(double) + sizeof(uint32_t));
    header.index_offset = header.data_offset + static_cast<uint64_t>(data_bytes);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return Error(Error::Code::IoError, "Unable to open tick mmap output file");
    }

    write_bytes(out, &header, sizeof(header), nullptr);

    Sha256 sha;
    write_bytes(out, timestamps.data(), timestamps.size() * sizeof(int64_t), &sha);
    write_bytes(out, prices.data(), prices.size() * sizeof(double), &sha);
    write_bytes(out, quantities.data(), quantities.size() * sizeof(double), &sha);
    write_bytes(out, flags.data(), flags.size() * sizeof(uint32_t), &sha);

    if (!index.empty()) {
        write_bytes(out, index.data(), index.size() * sizeof(TickDateIndex), nullptr);
    }

    auto checksum = sha.digest();
    std::memcpy(header.checksum, checksum.data(), checksum.size());
    out.seekp(0);
    write_bytes(out, &header, sizeof(header), nullptr);
    return Ok();
}

Result<void> TickMmapWriter::validate_ticks(const std::vector<Tick>& ticks) const {
    if (ticks.empty()) {
        return Ok();
    }
    Timestamp last = ticks.front().timestamp;
    for (size_t i = 0; i < ticks.size(); ++i) {
        const auto& tick = ticks[i];
        if (tick.timestamp.microseconds() <= 0) {
            return Error(Error::Code::InvalidArgument, "Tick timestamp must be positive");
        }
        if (i > 0 && tick.timestamp < last) {
            return Error(Error::Code::InvalidArgument, "Ticks must be sorted by timestamp");
        }
        if (!std::isfinite(tick.price) || tick.price <= 0) {
            return Error(Error::Code::InvalidArgument, "Tick price must be positive");
        }
        if (!std::isfinite(tick.quantity) || tick.quantity <= 0) {
            return Error(Error::Code::InvalidArgument, "Tick quantity must be positive");
        }
        last = tick.timestamp;
    }
    return Ok();
}

std::vector<TickDateIndex> TickMmapWriter::build_date_index(const std::vector<Tick>& ticks) {
    std::vector<TickDateIndex> index;
    int32_t last_date = 0;
    for (size_t i = 0; i < ticks.size(); ++i) {
        int32_t date = yyyymmdd_from_timestamp(ticks[i].timestamp);
        if (i == 0 || date != last_date) {
            index.push_back(TickDateIndex{date, static_cast<uint64_t>(i)});
            last_date = date;
        }
    }
    return index;
}

}  // namespace regimeflow::data
