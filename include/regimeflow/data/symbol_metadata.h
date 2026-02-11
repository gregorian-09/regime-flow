#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/data/data_source.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace regimeflow::data {

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

using SymbolMetadataMap = std::unordered_map<std::string, SymbolMetadata>;

SymbolMetadataMap load_symbol_metadata_csv(const std::string& path,
                                           char delimiter = ',',
                                           bool has_header = true);
SymbolMetadataMap load_symbol_metadata_config(const Config& config,
                                              const std::string& key = "symbol_metadata");
SymbolMetadataMap metadata_from_symbols(const std::vector<SymbolInfo>& symbols);

void apply_symbol_metadata(std::vector<SymbolInfo>& symbols,
                           const SymbolMetadataMap& metadata,
                           bool overwrite = true);

}  // namespace regimeflow::data
