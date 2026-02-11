#pragma once

#include "regimeflow/common/time.h"
#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"
#include "regimeflow/data/tick.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace regimeflow::data {

#pragma pack(push, 1)
struct TickFileHeader {
    char magic[8];
    uint32_t version;
    uint32_t flags;
    char symbol[32];
    int64_t start_timestamp;
    int64_t end_timestamp;
    uint64_t tick_count;
    uint64_t data_offset;
    uint64_t index_offset;
    unsigned char checksum[32];
    unsigned char reserved[136];
};
#pragma pack(pop)

static_assert(sizeof(TickFileHeader) == 256, "TickFileHeader must be 256 bytes");

struct TickDateIndex {
    int32_t date_yyyymmdd = 0;
    uint64_t offset = 0;
};

class TickMmapFile {
public:
    explicit TickMmapFile(const std::string& path);
    ~TickMmapFile();

    TickMmapFile(const TickMmapFile&) = delete;
    TickMmapFile& operator=(const TickMmapFile&) = delete;

    TickMmapFile(TickMmapFile&& other) noexcept;
    TickMmapFile& operator=(TickMmapFile&& other) noexcept;

    const TickFileHeader& header() const;
    std::string symbol() const;
    SymbolId symbol_id() const;
    TimeRange time_range() const;
    size_t tick_count() const;

    class TickView {
    public:
        TickView(const TickMmapFile* file, size_t index);

        Timestamp timestamp() const;
        double price() const;
        double quantity() const;
        uint8_t flags() const;

        Tick to_tick() const;

    private:
        const TickMmapFile* file_ = nullptr;
        size_t index_ = 0;
    };

    TickView operator[](size_t index) const;
    TickView at(size_t index) const;

    std::pair<size_t, size_t> find_range(TimeRange range) const;

    std::span<const int64_t> timestamps() const;
    std::span<const double> prices() const;
    std::span<const double> quantities() const;
    std::span<const uint32_t> flags() const;

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

    const TickFileHeader* header_ = nullptr;
    const int64_t* timestamps_ = nullptr;
    const double* prices_ = nullptr;
    const double* quantities_ = nullptr;
    const uint32_t* flags_ = nullptr;
    const TickDateIndex* date_index_ = nullptr;
    std::string symbol_;
    SymbolId symbol_id_ = 0;
};

class TickMmapWriter {
public:
    Result<void> write_ticks(const std::string& path,
                             const std::string& symbol,
                             std::vector<Tick> ticks);

private:
    Result<void> validate_ticks(const std::vector<Tick>& ticks) const;
    static std::vector<TickDateIndex> build_date_index(const std::vector<Tick>& ticks);
};

}  // namespace regimeflow::data
