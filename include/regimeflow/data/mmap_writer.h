#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/mmap_reader.h"

#include <string>
#include <vector>

namespace regimeflow::data {

class MmapWriter {
public:
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
