#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/data/csv_reader.h"
#include "regimeflow/data/db_source.h"
#include "regimeflow/data/api_data_source.h"
#include "regimeflow/data/memory_data_source.h"
#include "regimeflow/data/mmap_data_source.h"
#include "regimeflow/data/order_book_mmap_data_source.h"
#include "regimeflow/data/tick_mmap_data_source.h"
#include "regimeflow/data/tick_csv_reader.h"

#include <memory>
#include <string>

namespace regimeflow::data {

class DataSourceFactory {
public:
    static std::unique_ptr<DataSource> create(const Config& config);

private:
    static CSVDataSource::Config parse_csv_config(const Config& cfg);
    static CSVTickDataSource::Config parse_tick_csv_config(const Config& cfg);
};

}  // namespace regimeflow::data
