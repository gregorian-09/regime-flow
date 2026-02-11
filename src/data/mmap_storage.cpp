#include "regimeflow/data/mmap_storage.h"

namespace regimeflow::data {

MmapStorage::MmapStorage(std::string path) : path_(std::move(path)) {}

bool MmapStorage::open_read() {
    try {
        file_ = std::make_unique<MemoryMappedDataFile>(path_);
        return true;
    } catch (const std::exception&) {
        file_.reset();
        return false;
    }
}

std::vector<Bar> MmapStorage::read_bars(SymbolId symbol, TimeRange range) const {
    std::vector<Bar> result;
    if (!file_) {
        return result;
    }
    SymbolId stored = file_->symbol_id();
    if (symbol != 0 && stored != 0 && symbol != stored) {
        return result;
    }
    auto [start, end] = file_->find_range(range);
    result.reserve(end - start);
    for (size_t i = start; i < end; ++i) {
        result.push_back((*file_)[i].to_bar());
    }
    return result;
}

}  // namespace regimeflow::data
