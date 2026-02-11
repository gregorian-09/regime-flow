#pragma once

#include "regimeflow/data/bar.h"
#include "regimeflow/data/mmap_reader.h"

#include <memory>
#include <string>
#include <vector>

namespace regimeflow::data {

class MmapStorage {
public:
    explicit MmapStorage(std::string path);

    bool open_read();
    std::vector<Bar> read_bars(SymbolId symbol, TimeRange range) const;

private:
    std::string path_;
    std::unique_ptr<MemoryMappedDataFile> file_;
};

}  // namespace regimeflow::data
