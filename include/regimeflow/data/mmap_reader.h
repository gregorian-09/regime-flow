/**
 * @file mmap_reader.h
 * @brief RegimeFlow regimeflow mmap reader declarations.
 */

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
/**
 * @brief Header layout for memory-mapped data files.
 */
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

/**
 * @brief Date-to-offset index entry.
 */
struct DateIndex {
    int32_t date_yyyymmdd = 0;
    uint64_t offset = 0;
};

/**
 * @brief Memory-mapped access to bar data files.
 */
class MemoryMappedDataFile {
public:
    /**
     * @brief Map a file into memory.
     * @param path Path to the mmap file.
     */
    explicit MemoryMappedDataFile(const std::string& path);
    /**
     * @brief Unmap and close the file.
     */
    ~MemoryMappedDataFile();

    MemoryMappedDataFile(const MemoryMappedDataFile&) = delete;
    MemoryMappedDataFile& operator=(const MemoryMappedDataFile&) = delete;

    MemoryMappedDataFile(MemoryMappedDataFile&& other) noexcept;
    MemoryMappedDataFile& operator=(MemoryMappedDataFile&& other) noexcept;

    /**
     * @brief File header.
     */
    const FileHeader& header() const;
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
     * @brief Number of bars in the file.
     */
    size_t bar_count() const;

    /**
     * @brief Lightweight view of a bar row in the mmap.
     */
    class BarView {
    public:
        BarView(const MemoryMappedDataFile* file, size_t index);

        /**
         * @brief Bar timestamp.
         */
        Timestamp timestamp() const;
        /**
         * @brief Open price.
         */
        double open() const;
        /**
         * @brief High price.
         */
        double high() const;
        /**
         * @brief Low price.
         */
        double low() const;
        /**
         * @brief Close price.
         */
        double close() const;
        /**
         * @brief Volume.
         */
        uint64_t volume() const;

        /**
         * @brief Convert view to a Bar struct.
         */
        Bar to_bar() const;

    private:
        const MemoryMappedDataFile* file_ = nullptr;
        size_t index_ = 0;
    };

    /**
     * @brief Access a bar view by index (unchecked).
     */
    BarView operator[](size_t index) const;
    /**
     * @brief Access a bar view by index (checked).
     */
    BarView at(size_t index) const;

    /**
     * @brief Forward iterator over BarView entries.
     */
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

    /**
     * @brief Begin iterator.
     */
    Iterator begin() const;
    /**
     * @brief End iterator.
     */
    Iterator end() const;

    /**
     * @brief Find a [start,end) index range for a time range.
     * @param range Requested time range.
     * @return Pair of indices.
     */
    std::pair<size_t, size_t> find_range(TimeRange range) const;

    /**
     * @brief Column views for zero-copy access.
     */
    std::span<const int64_t> timestamps() const;
    std::span<const double> opens() const;
    std::span<const double> highs() const;
    std::span<const double> lows() const;
    std::span<const double> closes() const;
    std::span<const uint64_t> volumes() const;
    /**
     * @brief Number of date index entries.
     */
    size_t date_index_count() const { return index_count_; }
    /**
     * @brief Preload the date index into memory.
     */
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
