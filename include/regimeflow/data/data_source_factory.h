/**
 * @file data_source_factory.h
 * @brief RegimeFlow regimeflow data source factory declarations.
 */

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

/**
 * @brief Factory for creating data sources from configuration.
 */
class DataSourceFactory {
public:
    /**
     * @brief Create a data source based on config.
     * @param config Data source configuration.
     * @return Data source instance.
     */
    static std::unique_ptr<DataSource> create(const Config& config);

private:
    static CSVDataSource::Config parse_csv_config(const Config& cfg);
    static CSVTickDataSource::Config parse_tick_csv_config(const Config& cfg);
};

}  // namespace regimeflow::data
