#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/common/types.h"
#include "regimeflow/data/bar.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

namespace regimeflow::data {

#pragma pack(push, 1)
struct FileHeader {
    char magic[8];
    uint32_t version;
    uint32_t flags;
    char symbol[32];
    uint32_t bar_type;
    uint32_t bar_size_ms;
    int64_t start_timestamp;
    int64_t end_timestamp;
    uint64_t bar_count;
    uint64_t data_offset;
    uint64_t index_offset;
    unsigned char checksum[32];
    unsigned char reserved[128];
};
#pragma pack(pop)

static_assert(sizeof(FileHeader) == 256, "FileHeader must be 256 bytes");

struct DateIndex {
    int32_t date_yyyymmdd = 0;
    uint64_t offset = 0;
};

class MemoryMappedDataFile {
public:
    explicit MemoryMappedDataFile(const std::string& path);
    ~MemoryMappedDataFile();

    MemoryMappedDataFile(const MemoryMappedDataFile&) = delete;
    MemoryMappedDataFile& operator=(const MemoryMappedDataFile&) = delete;

    MemoryMappedDataFile(MemoryMappedDataFile&& other) noexcept;
    MemoryMappedDataFile& operator=(MemoryMappedDataFile&& other) noexcept;

    const FileHeader& header() const;
    std::string symbol() const;
    SymbolId symbol_id() const;
    TimeRange time_range() const;
    size_t bar_count() const;

    class BarView {
    public:
        BarView(const MemoryMappedDataFile* file, size_t index);

        Timestamp timestamp() const;
        double open() const;
        double high() const;
        double low() const;
        double close() const;
        uint64_t volume() const;

        Bar to_bar() const;

    private:
        const MemoryMappedDataFile* file_ = nullptr;
        size_t index_ = 0;
    };

    BarView operator[](size_t index) const;
    BarView at(size_t index) const;

    class Iterator {
    public:
        Iterator(const MemoryMappedDataFile* file, size_t index);
        BarView operator*() const;
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;

    private:
        const MemoryMappedDataFile* file_ = nullptr;
        size_t index_ = 0;
    };

    Iterator begin() const;
    Iterator end() const;

    std::pair<size_t, size_t> find_range(TimeRange range) const;

    std::span<const int64_t> timestamps() const;
    std::span<const double> opens() const;
    std::span<const double> highs() const;
    std::span<const double> lows() const;
    std::span<const double> closes() const;
    std::span<const uint64_t> volumes() const;
    size_t date_index_count() const { return index_count_; }
    void preload_index() const;

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

    const FileHeader* header_ = nullptr;
    const int64_t* timestamps_ = nullptr;
    const double* opens_ = nullptr;
    const double* highs_ = nullptr;
    const double* lows_ = nullptr;
    const double* closes_ = nullptr;
    const uint64_t* volumes_ = nullptr;
    const DateIndex* date_index_ = nullptr;
    size_t index_count_ = 0;
    std::string symbol_;
    SymbolId symbol_id_ = 0;
};

}  // namespace regimeflow::data
