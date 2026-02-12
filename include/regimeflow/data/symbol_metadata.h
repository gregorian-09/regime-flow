/**
 * @file symbol_metadata.h
 * @brief RegimeFlow regimeflow symbol metadata declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/data/data_source.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::data {

/**
 * @brief Optional metadata fields for symbol enrichment.
 */
struct SymbolMetadata {
    std::string ticker;
    std::optional<std::string> exchange;
    std::optional<AssetClass> asset_class;
    std::optional<std::string> currency;
    std::optional<double> tick_size;
    std::optional<double> lot_size;
    std::optional<double> multiplier;
    std::optional<std::string> sector;
    std::optional<std::string> industry;
};

/**
 * @brief Map of symbol string to metadata.
 */
using SymbolMetadataMap = std::unordered_map<std::string, SymbolMetadata>;

/**
 * @brief Load metadata from a CSV file.
 * @param path CSV path.
 * @param delimiter Column delimiter.
 * @param has_header Whether the CSV has a header.
 * @return Metadata map.
 */
SymbolMetadataMap load_symbol_metadata_csv(const std::string& path,
                                           char delimiter = ',',
                                           bool has_header = true);
/**
 * @brief Load metadata from config.
 * @param config Root configuration.
 * @param key Config key for metadata block.
 * @return Metadata map.
 */
SymbolMetadataMap load_symbol_metadata_config(const Config& config,
                                              const std::string& key = "symbol_metadata");
/**
 * @brief Convert symbol info into metadata map.
 * @param symbols Symbol list.
 * @return Metadata map.
 */
SymbolMetadataMap metadata_from_symbols(const std::vector<SymbolInfo>& symbols);

/**
 * @brief Apply metadata to a list of symbols.
 * @param symbols Symbol list to mutate.
 * @param metadata Metadata map.
 * @param overwrite Whether to overwrite existing fields.
 */
void apply_symbol_metadata(std::vector<SymbolInfo>& symbols,
                           const SymbolMetadataMap& metadata,
                           bool overwrite = true);

}  // namespace regimeflow::data
