/**
 * @file mmap_storage.h
 * @brief RegimeFlow regimeflow mmap storage declarations.
 */

#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/mmap_reader.h"

#include <memory>
#include <string>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Simple wrapper for reading bars from a mmap data file.
 */
class MmapStorage {
public:
    /**
     * @brief Construct with a file path.
     * @param path Path to the mmap file.
     */
    explicit MmapStorage(std::string path);

    /**
     * @brief Open the file for read access.
     * @return True on success.
     */
    bool open_read();
    /**
     * @brief Read bars for a symbol and time range.
     * @param symbol Symbol ID.
     * @param range Time range.
     * @return Vector of bars.
     */
    std::vector<Bar> read_bars(SymbolId symbol, TimeRange range) const;

private:
    std::string path_;
    std::unique_ptr<MemoryMappedDataFile> file_;
};

}  // namespace regimeflow::data
