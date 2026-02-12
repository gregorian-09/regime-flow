/**
 * @file mmap_writer.h
 * @brief RegimeFlow regimeflow mmap writer declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/mmap_reader.h"

#include <string>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Writer for memory-mapped bar files.
 */
class MmapWriter {
public:
    /**
     * @brief Write bars to a memory-mapped file.
     * @param path Output file path.
     * @param symbol Symbol string.
     * @param bar_type Bar type.
     * @param bars Bars to write.
     * @return Ok on success, error otherwise.
     */
    Result<void> write_bars(const std::string& path,
                            const std::string& symbol,
                            BarType bar_type,
                            std::vector<Bar> bars);

private:
    Result<void> validate_bars(const std::vector<Bar>& bars) const;
    static uint32_t bar_size_ms(BarType type);
    static std::vector<DateIndex> build_date_index(const std::vector<Bar>& bars);
};

}  // namespace regimeflow::data
